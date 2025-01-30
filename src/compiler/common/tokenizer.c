
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "tokenizer.h"

#include "char_repr.h"
#include "strong_malloc.h"

#include <inttypes.h>
#include <stdlib.h>



// Delete a tokenizer context.
// Deletes the token in the buffer but not any tokens consumed.
void tkn_ctx_delete(tokenizer_t *tkn_ctx) {
    if (tkn_ctx->has_tkn_buffer) {
        tkn_delete(tkn_ctx->tkn_buffer);
    }
    if (tkn_ctx->cleanup) {
        tkn_ctx->cleanup(tkn_ctx);
    }
    free(tkn_ctx);
}


// Consume next token from the tokenizer.
token_t tkn_next(tokenizer_t *tkn_ctx) {
    if (tkn_ctx->has_tkn_buffer) {
        tkn_ctx->has_tkn_buffer = false;
        return tkn_ctx->tkn_buffer;
    } else {
        return tkn_ctx->next(tkn_ctx);
    }
}

// Peek at (do not consume) next token from the tokenizer.
token_t tkn_peek(tokenizer_t *tkn_ctx) {
    if (!tkn_ctx->has_tkn_buffer) {
        tkn_ctx->has_tkn_buffer = true;
        tkn_ctx->tkn_buffer     = tkn_ctx->next(tkn_ctx);
    }
    return tkn_ctx->tkn_buffer;
}

// Opposite of tkn_next; stuff up to one token back into the buffer.
// Will abort if there is already a token there.
void tkn_unget(tokenizer_t *tkn_ctx, token_t token) {
    if (tkn_ctx->has_tkn_buffer) {
        fprintf(stderr, "[BUG] tkn_unget() with token already in buffer\n");
        abort();
    }
    tkn_ctx->has_tkn_buffer = true;
    tkn_ctx->tkn_buffer     = token;
}


// Delete a token's dynamic memory (`strval` and `params`).
void tkn_delete(token_t token) {
    if (token.strval) {
        free(token.strval);
    }
    for (size_t i = 0; i < token.params_len; i++) {
        tkn_delete(token.params[i]);
    }
    if (token.params) {
        free(token.params);
    }
}

// Delete an array of tokens and each token within.
void tkn_arr_delete(size_t tokens_len, token_t *tokens) {
    for (size_t i = 0; i < tokens_len; i++) {
        tkn_delete(tokens[i]);
    }
    free(tokens);
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
            "pos:        %s:%d:%d (len %d)\n",
            token.pos.srcfile->path,
            token.pos.line + 1,
            token.pos.col + 1,
            token.pos.len
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
    printf("EXPECT_INT(%s.pos.len, %d);\n", access, token.pos.len);
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
                char      *mem   = strong_malloc(len + 1);
                snprintf(mem, len + 1, fmt, access, i);
                tkn_debug_testcase_r(token.params[i], keyw, ast, tkn, mem, indent + 1);
                free(mem);
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