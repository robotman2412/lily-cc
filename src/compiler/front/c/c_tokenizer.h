
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

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

// C string subtype.
// Only relevant to the preprocessor.
typedef enum {
    // A normal string token as the C frontend wants it.
    C_STR_NORMAL,
    // A raw double quotes string.
    C_STR_RAW_DQUOT,
    // A raw single quotes string.
    C_STR_RAW_SQUOT,
    // An angle-brackets raw string.
    C_STR_ANGLEBRAC,
} c_strtype_t;

// C whitespace subtype.
typedef enum {
    // Plain old unprintable / whitespace characters.
    C_WHITESPACE,
    // Line comment.
    C_LINE_COMMENT,
    // Block comment.
    C_BLOCK_COMMENT,
} c_whitespace_t;

// C identifier subtype.
// Only relevant to the preprocessor.
typedef enum {
    C_IDENT,
    C_PPNUMBER,
} c_identtype_t;


// C tokenizer handle.
typedef struct c_tokenizer c_tokenizer_t;


// C tokenizer handle.
struct c_tokenizer {
    // Common tokenizer data.
    tokenizer_t base;
    // Current C standard.
    int         c_std;
    // Preprocessor tokenizer mode; keywords are left as idents and whitespace is included.
    bool        preproc_mode;
    // Enable the angle-bracket `<>` strings used by `#include`.
    bool        str_anglebrac;
    // Keep comments instead of replacing them with a single space each.
    bool        keep_comments;
};



// Enum names of `c_keyw_t` values.
extern char const *const c_keyw_name[];
// Enum names of `c_tokentype_t` values.
extern char const *const c_token_id[];
// List of keywords.
extern char const *const c_keywords[];
// List of tokens.
extern char const *const c_token_name[];


// Create a new C tokenizer.
c_tokenizer_t *c_tkn_create(srcfile_t *srcfile, int c_std);
// Test whether a character is legal as the first in a C identifier.
bool           c_is_first_sym_char(int c);
// Test whether a character is legal in a C identifier.
bool           c_is_sym_char(int c);

// Convert preprocessing number token to C number token.
token_t c_tkn_conv_number(cctx_t *cctx, int c_std, token_t const *pre_tkn);
// Tokenize string or character constant.
token_t c_tkn_conv_str(cctx_t *cctx, int c_std, token_t const *pre_tkn);

// Wrapper around `srcfile_getc` that handles `\` for newline escapes.
int      c_srcfile_getc(srcfile_t *srcfile, pos_t *pos);
// Get next token from C tokenizer.
token_t  c_tkn_next(tokenizer_t *ctx);
// Try to find the matching C keyword.
// Returns -1 if not a keyword in the current C standard.
c_keyw_t c_keyw_get(int c_std, char const *name);
// Print the source representation of a token.
void     c_tkn_print_src(token_t const *pre_tkn, FILE *to);
// Append the source representation of a token to a heap-allocated string.
// WARNING: Does not NUL-terminate!
void     c_tkn_append_src(token_t const *pre_tkn, char **buf_ptr, size_t *len_ptr, size_t *cap_ptr);


// Test if a token is a certain keyword.
static inline bool c_tkn_is_keyw(token_t tkn, c_keyw_t keyw) {
    return tkn.type == TOKENTYPE_KEYWORD && (c_keyw_t)tkn.subtype == keyw;
}

// Test if a token is of a certain type.
static inline bool c_tkn_is(token_t tkn, c_tokentype_t type) {
    return tkn.type == TOKENTYPE_OTHER && (c_tokentype_t)tkn.subtype == type;
}
