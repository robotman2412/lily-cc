
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

static void tkn_debug_print_r(token_t tkn, int indent) {
    if (tkn.pos.srcfile) {
        pindent(indent);
        printf("pos:        %s:%d:%d\n", tkn.pos.srcfile->path, tkn.pos.line + 1, tkn.pos.col + 1);
    }
    if (tkn.type == TOKENTYPE_AST) {
        pindent(indent);
        printf("asttype:    %d\n", tkn.subtype);
        for (size_t i = 0; i < tkn.params_len; i++) {
            pindent(indent);
            printf("child %zu/%zu:\n", i + 1, tkn.params_len);
            tkn_debug_print_r(tkn.params[i], indent + 1);
        }
    } else {
        pindent(indent);
        printf("type:       %d\n", tkn.type);
        pindent(indent);
        printf("subtype:    %d\n", tkn.subtype);
        if (tkn.type == TOKENTYPE_SCONST) {
            pindent(indent);
            printf("strval:     %s\n", tkn.strval);
        } else if (tkn.type == TOKENTYPE_IDENT) {
            pindent(indent);
            printf("ident:      %s\n", tkn.strval);
        }
        if (tkn.type == TOKENTYPE_ICONST || tkn.type == TOKENTYPE_CCONST) {
            pindent(indent);
            printf("ival:       %" PRId64 "\n", tkn.ival);
        }
    }
}

// Print a token.
void tkn_debug_print(token_t token) {
    printf("Token:\n");
    tkn_debug_print_r(token, 1);
}
