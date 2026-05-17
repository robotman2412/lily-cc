
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "compiler.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>



// Abstract tokenizer handle.
typedef struct tokenizer tokenizer_t;
// Offset type for `tkn_getc`.
typedef struct tknoff    tknoff_t;


// Abstract tokenizer handle.
struct tokenizer {
    // Associated frontend context.
    cctx_t    *cctx;
    // Current source file.
    srcfile_t *file;
    // Current file position.
    pos_t      pos;
    // Number of buffered tokens.
    size_t     tkn_buffer_len;
    // Allocated capacity of the buffered-token array.
    size_t     tkn_buffer_cap;
    // Buffered tokens (FIFO; index 0 is the next token returned).
    token_t   *tkn_buffer;
    // Function to call to get next token.
    token_t (*next)(tokenizer_t *tkn_ctx);
    // Extra function to call to clean up tokenizer.
    void (*cleanup)(tokenizer_t *tkn_ctx);
};

// Offset type for `tkn_getc`.
struct tknoff {
    size_t offset;
    int    col_offset;
    int    line_offset;
};



// Delete a tokenizer context.
// Deletes the token in the buffer but not any tokens consumed.
void tkn_ctx_delete(tokenizer_t *tkn_ctx);

// Consume next token from the tokenizer.
token_t tkn_next(tokenizer_t *tkn_ctx);
// Peek at (do not consume) next token from the tokenizer.
token_t tkn_peek(tokenizer_t *tkn_ctx);
// Peek at (do not consume) next token from the tokenizer.
// Depth 0 is one ahead, depth 1 is two ahead, etc. The buffer grows as needed.
token_t tkn_peek_n(tokenizer_t *tkn_ctx, size_t depth);
// Opposite of tkn_next; stuff a token back to the front of the buffer.
void    tkn_unget(tokenizer_t *tkn_ctx, token_t token);

// Read a character from a token's `strval` and update offset.
// Returns -1 on end of token.
int tkn_getc(token_t const *tkn, tknoff_t *off);

// Delete a token's dynamic memory (`strval` and `params`).
void    tkn_delete(token_t token);
// Perform a deep copy of a token.
token_t tkn_clone(token_t const *token);
// Delete an array of tokens and each token within.
void    tkn_arr_delete(size_t tokens_len, token_t *tokens);

// Tests whether a character is a valid hexadecimal constant character ([0-9a-fA-F]).
bool is_hex_char(int c);

#ifndef NDEBUG
// Print a token.
void tkn_debug_print(token_t token, char const *const keyw[], char const *const ast[], char const *const tkn[]);
// Build a test case that asserts an exact value for a token.
void tkn_debug_testcase(token_t token, char const *const keyw[], char const *const ast[], char const *const tkn[]);
#endif
