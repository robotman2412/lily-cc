
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

typedef enum {
#define IR_KEYW_DEF(keyw) IR_KEYW_##keyw,
#include "defs/ir_keywords.inc"
    IR_N_KEYWS,
} ir_keyw_t;

#include "tokenizer.h"

typedef enum {
    // A comma.
    IR_TKN_COMMA,
    // An opening parenthesis.
    IR_TKN_LPAR,
    // A closing parenthesis.
    IR_TKN_RPAR,
    // An undefined of some type.
    IR_TKN_UNDEF,
    // A variable assignment; equals symbol.
    IR_TKN_ASSIGN,
    // A plus symbol.
    IR_TKN_ADD,
    // A minus symbol.
    IR_TKN_SUB,
} ir_tokentype_t;

typedef enum {
    // A global identifier.
    IR_IDENT_GLOBAL,
    // A function-local identifier.
    IR_IDENT_LOCAL,
    // An identifier without prefix or suffix.
    IR_IDENT_BARE,
    // A register identifier.
    IR_IDENT_REG,
} ir_identtype_t;

// List of keywords.
extern char const *const ir_keywords[];

// Create an IR text tokenizer.
tokenizer_t *ir_tkn_create(srcfile_t *srcfile);
// Get next token from IR tokenizer.
token_t      ir_tkn_next(tokenizer_t *ctx);
// Get an IR keyword by C-string.
ir_keyw_t    ir_keyw_get(char const *keyw);
