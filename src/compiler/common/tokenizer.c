
#include "tokenizer.h"

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


static void pindent(int indent) {
    while (indent-- > 0) fputs("  ", stdout);
}

static void tkn_debug_print_r(
    token_t token, char const *const keyw[], char const *const ast[], char const *const tkn[], int indent
) {
    if (token.pos.srcfile) {
        pindent(indent);
        printf("pos:        %s:%d:%d\n", token.pos.srcfile->path, token.pos.line + 1, token.pos.col + 1);
    }
    if (token.type == TOKENTYPE_AST) {
        pindent(indent);
        printf("asttype:    %s\n", ast[token.subtype]);
        for (size_t i = 0; i < token.params_len; i++) {
            pindent(indent);
            printf("child %zu/%zu:\n", i + 1, token.params_len);
            tkn_debug_print_r(token.params[i], keyw, ast, tkn, indent + 1);
        }
    } else {
        if (token.type == TOKENTYPE_OTHER) {
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
        } else if (token.type == TOKENTYPE_ICONST || token.type == TOKENTYPE_CCONST) {
            pindent(indent);
            printf("ival:       %" PRId64 "\n", token.ival);
            if (token.type == TOKENTYPE_CCONST && token.ival >= 0x20 && token.ival <= 0x7e) {
                printf("character:  %c\n", (char)token.ival);
            }
        }
    }
}

// Print a token.
void tkn_debug_print(token_t token, char const *const keyw[], char const *const ast[], char const *const tkn[]) {
    printf("Token:\n");
    tkn_debug_print_r(token, keyw, ast, tkn, 1);
}
