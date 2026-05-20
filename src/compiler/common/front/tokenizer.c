
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "tokenizer.h"

#include "arrays.h"
#include "char_repr.h"
#include "lilycc_malloc.h"
#include "utf8.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>



// Delete a tokenizer context.
// Deletes the token in the buffer but not any tokens consumed.
void tkn_ctx_delete(tokenizer_t *tkn_ctx) {
    for (size_t i = 0; i < tkn_ctx->tkn_buffer_len; i++) {
        tkn_delete(tkn_ctx->tkn_buffer[i]);
    }
    lilycc_free(tkn_ctx->tkn_buffer);
    if (tkn_ctx->cleanup) {
        tkn_ctx->cleanup(tkn_ctx);
    }
    lilycc_free(tkn_ctx);
}


// Consume next token from the tokenizer.
token_t tkn_next(tokenizer_t *tkn_ctx) {
    if (tkn_ctx->tkn_buffer_len) {
        tkn_ctx->tkn_buffer_len--;
        token_t tmp = tkn_ctx->tkn_buffer[0];
        memmove(tkn_ctx->tkn_buffer, tkn_ctx->tkn_buffer + 1, tkn_ctx->tkn_buffer_len * sizeof(token_t));
        return tmp;
    } else {
        return tkn_ctx->next(tkn_ctx);
    }
}

// Peek at (do not consume) next token from the tokenizer.
token_t tkn_peek(tokenizer_t *tkn_ctx) {
    return tkn_peek_n(tkn_ctx, 0);
}

// Peek at (do not consume) next token from the tokenizer.
// Depth 0 is one ahead, depth 1 is two ahead, etc. The buffer grows as needed.
token_t tkn_peek_n(tokenizer_t *tkn_ctx, size_t depth) {
    while (tkn_ctx->tkn_buffer_len <= depth) {
        token_t tmp = tkn_ctx->next(tkn_ctx);
        if (tmp.type == TOKENTYPE_EOF) {
            return tmp;
        }
        array_lencap_insert_strong(
            &tkn_ctx->tkn_buffer,
            sizeof(token_t),
            &tkn_ctx->tkn_buffer_len,
            &tkn_ctx->tkn_buffer_cap,
            &tmp,
            tkn_ctx->tkn_buffer_len
        );
    }
    return tkn_ctx->tkn_buffer[depth];
}

// Opposite of tkn_next; stuff a token back to the front of the buffer.
void tkn_unget(tokenizer_t *tkn_ctx, token_t token) {
    array_lencap_insert_strong(
        &tkn_ctx->tkn_buffer,
        sizeof(token_t),
        &tkn_ctx->tkn_buffer_len,
        &tkn_ctx->tkn_buffer_cap,
        &token,
        0
    );
}


// Next-token callback for `tkn_array_t`.
static token_t tkn_array_next(tokenizer_t *tkn_ctx) {
    tkn_array_t *ctx = (tkn_array_t *)tkn_ctx;
    if (ctx->index >= ctx->tokens_len) {
        return (token_t){.pos = ctx->eof_pos, .type = TOKENTYPE_EOF};
    }
    return tkn_clone(&ctx->tokens[ctx->index++]);
}

// Create an array-backed tokenizer.
tkn_array_t *tkn_array_create(token_t const *tokens, size_t tokens_len, pos_t eof_pos) {
    tkn_array_t *ctx = lilycc_calloc(1, sizeof(tkn_array_t));
    ctx->base.next   = tkn_array_next;
    ctx->base.pos    = eof_pos;
    ctx->tokens      = tokens;
    ctx->tokens_len  = tokens_len;
    ctx->eof_pos     = eof_pos;
    return ctx;
}


// Read a character from a token's `strval` and update offset.
// Returns -1 on end of token.
int tkn_getc(token_t const *tkn, tknoff_t *off) {
    size_t off1 = off->offset;
    int    c    = utf8_decode(tkn->strval, tkn->strval_len, &off1);
    if (c == -1) {
        return -1;
    }
    if (c == '\n') {
        off->col_offset = -tkn->pos.col;
        off->line_offset++;
    } else {
        off->col_offset++;
    }
    off->offset = off1;
    return c;
}


// Delete a token's dynamic memory (`strval` and `params`).
void tkn_delete(token_t token) {
    if (token.strval) {
        lilycc_free(token.strval);
    }
    for (size_t i = 0; i < token.params_len; i++) {
        tkn_delete(token.params[i]);
    }
    if (token.params) {
        lilycc_free(token.params);
    }
}

// Perform a deep copy of a token.
token_t tkn_clone(token_t const *token) {
    token_t out = {
        .pos        = token->pos,
        .type       = token->type,
        .subtype    = token->subtype,
        .ival       = token->ival,
        .ivalh      = token->ivalh,
        .strval_len = token->strval_len,
        .params_len = token->params_len,
    };

    if (token->strval_len) {
        out.strval = lilycc_malloc(token->strval_len + 1);
        memcpy(out.strval, token->strval, token->strval_len);
        out.strval[token->strval_len] = 0;
    }

    if (token->params_len) {
        out.params = lilycc_calloc(token->params_len, sizeof(token_t));
        for (size_t i = 0; i < token->params_len; i++) {
            out.params[i] = tkn_clone(&token->params[i]);
        }
    }

    return out;
}

// Delete an array of tokens and each token within.
void tkn_arr_delete(size_t tokens_len, token_t *tokens) {
    for (size_t i = 0; i < tokens_len; i++) {
        tkn_delete(tokens[i]);
    }
    lilycc_free(tokens);
}


// Tests whether a character is a valid hexadecimal constant character ([0-9a-fA-F]).
bool is_hex_char(int c) {
    if (c >= '0' && c <= '9') {
        return true;
    }
    c |= 0x20;
    return c >= 'a' && c <= 'f';
}



#ifndef NDEBUG
static void pindent(int indent) {
    while (indent-- > 0) fputs("    ", stdout);
}


// Print a token.
static void tkn_debug_print_r(
    token_t token, char const *const keyw[], char const *const ast[], char const *const tkn[], int indent
) {
    if (token.pos.srcfile) {
        pindent(indent);
        printf(
            "pos:        %s:%d:%d (len %lld)\n",
            token.pos.srcfile->path,
            token.pos.line + 1,
            token.pos.col + 1,
            (long long)token.pos.len
        );
    }
    if (token.type == TOKENTYPE_AST) {
        pindent(indent);
        printf("asttype:    %s\n", ast[token.subtype]);
        for (size_t i = 0; i < token.params_len; i++) {
            pindent(indent);
            printf("child %zu/%zu:\n", i + 1, token.params_len);
            tkn_debug_print_r(token.params[i], keyw, ast, tkn, indent + 1);
        }
    } else if (token.type == TOKENTYPE_OTHER) {
        pindent(indent);
        printf("subtype:    %s\n", tkn[token.subtype]);
    } else if (token.type == TOKENTYPE_KEYWORD) {
        pindent(indent);
        printf("keyword:    %s\n", keyw[token.subtype]);
    } else if (token.type == TOKENTYPE_SCONST) {
        pindent(indent);
        printf("strval:     %s\n", token.strval);
    } else if (token.type == TOKENTYPE_IDENT) {
        pindent(indent);
        printf("ident:      %s\n", token.strval);
    } else if (token.type == TOKENTYPE_ICONST) {
        pindent(indent);
        printf("ival:       %" PRId64 "\n", token.ival);
    } else if (token.type == TOKENTYPE_CCONST) {
        pindent(indent);
        printf("character:  ");
        print_char_repr(token.ival, stdout);
        printf("\n");
    }
}

// Print a token.
void tkn_debug_print(token_t token, char const *const keyw[], char const *const ast[], char const *const tkn[]) {
    printf("Token:\n");
    tkn_debug_print_r(token, keyw, ast, tkn, 1);
}


// Build a test case that asserts an exact value for a token.
void tkn_debug_testcase_r(
    token_t           token,
    char const *const keyw[],
    char const *const ast[],
    char const *const tkn[],
    char const       *access,
    int               indent
) {
    pindent(indent);
    printf("EXPECT_INT(%s.pos.line, %d);\n", access, token.pos.line);
    pindent(indent);
    printf("EXPECT_INT(%s.pos.col, %d);\n", access, token.pos.col);
    pindent(indent);
    printf("EXPECT_INT(%s.pos.len, %lld);\n", access, (long long)token.pos.len);
    pindent(indent);
    printf("EXPECT_INT(%s.type, %s);\n", access, tokentype_names[token.type]);
    if (token.type == TOKENTYPE_AST) {
        pindent(indent);
        printf("EXPECT_INT(%s.subtype, %s);\n", access, ast[token.subtype]);
        pindent(indent);
        printf("EXPECT_INT(%s.params_len, %zu);\n", access, token.params_len);
        if (token.params_len) {
            pindent(indent);
            printf("{");
            for (size_t i = 0; i < token.params_len; i++) {
                printf("\n");
                pindent(indent + 1);
                printf("token_t %s_%zu = %s.params[%zu];\n", access, i, access, i);
                char const fmt[] = "%s_%zu";
                size_t     len   = snprintf(NULL, 0, fmt, access, i);
                char      *mem   = lilycc_malloc(len + 1);
                snprintf(mem, len + 1, fmt, access, i);
                tkn_debug_testcase_r(token.params[i], keyw, ast, tkn, mem, indent + 1);
                lilycc_free(mem);
            }
            pindent(indent);
            printf("}\n");
        }
    } else if (token.type == TOKENTYPE_OTHER || token.type == TOKENTYPE_KEYWORD) {
        pindent(indent);
        printf("EXPECT_INT(%s.subtype, %s);\n", access, keyw[token.subtype]);
    } else if (token.type == TOKENTYPE_SCONST || token.type == TOKENTYPE_IDENT) {
        pindent(indent);
        printf("EXPECT_STR_L(%s.strval, %s.strval_len, \"", access, access);
        print_cstr_repr(token.strval, token.strval_len, stdout);
        printf("\", %zu);\n", token.strval_len);
    } else if (token.type == TOKENTYPE_ICONST) {
        pindent(indent);
        printf("EXPECT_INT(%s.ival, %" PRId64 ");\n", access, token.ival);
    } else if (token.type == TOKENTYPE_CCONST) {
        pindent(indent);
        printf("EXPECT_INT(%s.ival, '", access);
        print_char_repr(token.ival, stdout);
        printf("');\n");
    }
}

// Build a test case that asserts an exact value for a token.
void tkn_debug_testcase(token_t token, char const *const keyw[], char const *const ast[], char const *const tkn[]) {
    tkn_debug_testcase_r(token, keyw, ast, tkn, "token", 0);
}
#endif
