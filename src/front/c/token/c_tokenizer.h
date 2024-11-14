
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#pragma once

#include "token/tokenizer.h"


// C keywords.
typedef enum {
#define C_KEYW_DEF(name) C_KEYW_##name,
#include "c_keywords.inc"
    C_N_KEYWS,
} c_keyw_t;

// C token subtype.
typedef enum {
    C_TKN_LPAR,  // `(`
    C_TKN_RPAR,  // `)`
    C_TKN_LBRAC, // `[`
    C_TKN_RBRAC, // `]`
    C_TKN_LCURL, // `{`
    C_TKN_RCURL, // `}`

    C_TKN_QUESTION, // `?`
    C_TKN_COLON,    // `:`
    C_TKN_SEMIC,    // `;`

    C_TKN_INC, // `++`
    C_TKN_DEC, // `--`

    C_TKN_NOT,  // `~`
    C_TKN_LNOT, // `!`
    C_TKN_LAND, // `&&`
    C_TKN_LOR,  // `||`

    C_TKN_ADD, // `+`
    C_TKN_SUB, // `-`
    C_TKN_MUL, // `*`
    C_TKN_DIV, // `/`
    C_TKN_MOD, // `%`
    C_TKN_SHL, // `<<`
    C_TKN_SHR, // `>>`
    C_TKN_AND, // `&`
    C_TKN_OR,  // `|`
    C_TKN_XOR, // `^`

    C_TKN_ADD_S, // `+=`
    C_TKN_SUB_S, // `-=`
    C_TKN_MUL_S, // `*=`
    C_TKN_DIV_S, // `/=`
    C_TKN_MOD_S, // `%=`
    C_TKN_SHL_S, // `<<=`
    C_TKN_SHR_S, // `>>=`
    C_TKN_AND_S, // `&=`
    C_TKN_OR_S,  // `|=`
    C_TKN_XOR_S, // `^=`

    C_TKN_EQ, // `==`
    C_TKN_NE, // `!=`
    C_TKN_LT, // `<'
    C_TKN_LE, // `<='
    C_TKN_GT, // `>'
    C_TKN_GE, // `>='

    C_TKN_DOT,    // `.`
    C_TKN_ARROW,  // `->`
    C_TKN_ASSIGN, // `=`
} c_tokentype_t;



// List of keywords.
extern char const *c_keywords[];

// Test whether a character is legal as the first in a C identifier.
bool    c_is_first_sym_char(int c);
// Test whether a character is legal in a C identifier.
bool    c_is_sym_char(int c);
// Get next token from C tokenizer.
token_t c_tkn_next(tokenizer_t *ctx);


// Test if a token is a certain keyword.
static inline bool c_tkn_is_keyw(token_t tkn, c_keyw_t keyw) {
    return tkn.type == TOKENTYPE_KEYWORD && tkn.subtype == keyw;
}

// Test if a token is of a certain type.
static inline bool c_tkn_is(token_t tkn, c_tokentype_t type) {
    return tkn.type == TOKENTYPE_OTHER && tkn.subtype == type;
}
