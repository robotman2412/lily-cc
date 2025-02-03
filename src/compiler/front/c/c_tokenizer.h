
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "c_std.h"
#include "compiler.h"
#include "tokenizer.h"


// C keywords.
typedef enum {
#define C_KEYW_DEF(since, deprecated, name) C_KEYW_##name,
#include "c_keywords.inc"
    C_N_KEYWS,
} c_keyw_t;

// C token subtype.
typedef enum {
#define C_TOKEN_DEF(id, name) C_TKN_##id,
#include "c_tokens.inc"
    C_N_TKNS,
} c_tokentype_t;


// C tokenizer handle.
typedef struct c_tokenizer c_tokenizer_t;


// C tokenizer handle.
struct c_tokenizer {
    // Common tokenizer data.
    tokenizer_t base;
    // Current C standard.
    int         c_std;
    // Preprocessor tokenizer mode; keywords are left as idents.
    bool        preproc_mode;
};



#ifndef NDEBUG
// Enum names of `c_keyw_t` values.
extern char const *const c_keyw_name[];
// Enum names of `c_tokentype_t` values.
extern char const *const c_tokentype_name[];
#endif

// List of keywords.
extern char const *const c_keywords[];


// Create a new C tokenizer.
tokenizer_t *c_tkn_create(srcfile_t *srcfile, int c_std);
// Test whether a character is legal as the first in a C identifier.
bool         c_is_first_sym_char(int c);
// Test whether a character is legal in a C identifier.
bool         c_is_sym_char(int c);
// Get next token from C tokenizer.
token_t      c_tkn_next(tokenizer_t *ctx);
// Try to find the matching C keyword.
// Returns -1 if not a keyword in the current C standard.
c_keyw_t     c_keyw_get(tokenizer_t const *ctx, char const *name);


// Test if a token is a certain keyword.
static inline bool c_tkn_is_keyw(token_t tkn, c_keyw_t keyw) {
    return tkn.type == TOKENTYPE_KEYWORD && tkn.subtype == keyw;
}

// Test if a token is of a certain type.
static inline bool c_tkn_is(token_t tkn, c_tokentype_t type) {
    return tkn.type == TOKENTYPE_OTHER && tkn.subtype == type;
}
