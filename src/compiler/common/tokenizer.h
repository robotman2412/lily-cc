
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#pragma once

#include "compiler.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>



// Abstract tokenizer handle.
typedef struct tokenizer tokenizer_t;


// Abstract tokenizer handle.
struct tokenizer {
    // Associated frontend context.
    cctx_t    *cctx;
    // Current source file.
    srcfile_t *file;
    // Current file position.
    pos_t      pos;
    // Has a buffered token.
    bool       has_tkn_buffer;
    // Buffered token.
    token_t    tkn_buffer;
    // Function to call to get next token.
    token_t (*next)(tokenizer_t *tkn_ctx);
    // Extra function to call to clean up tokenizer.
    void (*cleanup)(tokenizer_t *tkn_ctx);
};



// Delete a tokenizer context.
// Deletes the token in the buffer but not any tokens consumed.
void tkn_ctx_delete(tokenizer_t *tkn_ctx);

// Consume next token from the tokenizer.
token_t tkn_next(tokenizer_t *tkn_ctx);
// Peek at (do not consume) next token from the tokenizer.
token_t tkn_peek(tokenizer_t *tkn_ctx);

// Delete a token's dynamic memory (`strval` and `params`).
void tkn_delete(token_t token);

// Tests whether a character is a valid hexadecimal constant character ([0-9a-fA-F]).
bool is_hex_char(int c);
