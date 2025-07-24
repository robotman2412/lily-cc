
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "compiler.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>



// Maximum depth for peeking minus one.
#define TKN_PEEK_MAX 1

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
    // Number of buffered tokens.
    uint8_t    tkn_buffer_len;
    // Buffered token.
    token_t    tkn_buffer[TKN_PEEK_MAX + 1];
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
// Peek at (do not consume) next token from the tokenizer.
// Depth 0 is one ahead, depth 1 is two ahead, etc.
token_t tkn_peek_n(tokenizer_t *tkn_ctx, int depth);
// Opposite of tkn_next; stuff up to one token back into the buffer.
// Will abort if there is already a token there.
void    tkn_unget(tokenizer_t *tkn_ctx, token_t token);

// Delete a token's dynamic memory (`strval` and `params`).
void tkn_delete(token_t token);
// Delete an array of tokens and each token within.
void tkn_arr_delete(size_t tokens_len, token_t *tokens);

// Tests whether a character is a valid hexadecimal constant character ([0-9a-fA-F]).
bool is_hex_char(int c);

#ifndef NDEBUG
// Print a token.
void tkn_debug_print(token_t token, char const *const keyw[], char const *const ast[], char const *const tkn[]);
// Build a test case that asserts an exact value for a token.
void tkn_debug_testcase(token_t token, char const *const keyw[], char const *const ast[], char const *const tkn[]);
#endif
