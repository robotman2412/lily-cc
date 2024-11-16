
#include "tokenizer.h"

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


// Tests whether a character is a valid hexadecimal constant character ([0-9a-fA-F]).
bool is_hex_char(int c) {
    if (c >= '0' && c <= '9') {
        return true;
    }
    c |= 0x20;
    return c >= 'a' && c <= 'f';
}
