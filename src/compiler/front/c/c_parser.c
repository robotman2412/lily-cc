
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "c_parser.h"

#include "strong_malloc.h"



// Is a valid token for the start of an expression?
static bool is_first_expr_tkn(token_t tkn) {
    switch (tkn.type) {
        default: return false;
        case TOKENTYPE_CCONST:
        case TOKENTYPE_ICONST:
        case TOKENTYPE_SCONST:
        case TOKENTYPE_IDENT: return true;
        case TOKENTYPE_KEYWORD: return tkn.subtype == C_KEYW_alignof || tkn.subtype == C_KEYW_sizeof;
        case TOKENTYPE_OTHER:
            switch (tkn.subtype) {
                case C_TKN_ADD:
                case C_TKN_SUB:
                case C_TKN_MUL:
                case C_TKN_LPAR:
                case C_TKN_INC:
                case C_TKN_DEC: return true;
                default: return false;
            }
    }
}



// Parse a C compilation unit into an AST.
token_t c_parse(tokenizer_t *tkn_ctx);


// Parse a function call expression.
token_t c_parse_funccall(tokenizer_t *tkn_ctx, token_t funcname);

// Parse one or more C expressions separated by commas.
token_t c_parse_exprs(tokenizer_t *tkn_ctx) {
    size_t   exprs_len = 0;
    size_t   exprs_cap = 1;
    token_t *exprs     = strong_malloc(exprs_cap * sizeof(token_t));
    if (!is_first_expr_tkn(tkn_peek(tkn_ctx))) {
        // TODO: Return ASTTYPE_GARBAGE.
    }
    *exprs = c_parse_expr(tkn_ctx);
}

// Parse a C expression.
token_t c_parse_expr(tokenizer_t *tkn_ctx) {
    size_t   stack_len = 0;
    size_t   stack_cap = 0;
    token_t *stack;
}

// Try to parse a C statement.
bool c_parse_stmt(tokenizer_t *tkn_ctx, token_t *tkn_out);

// Try to parse a C variable declaration.
bool c_parse_vardecl(tokenizer_t *tkn_ctx, token_t *tkn_out);

// Try to parse a C if statement.
bool c_parse_if_stmt(tokenizer_t *tkn_ctx, token_t *tkn_out);

// Try to parse a C for loop.
bool c_parse_for_loop(tokenizer_t *tkn_ctx, token_t *tkn_out);

// Try to parse a C while loop.
bool c_parse_while_loop(tokenizer_t *tkn_ctx, token_t *tkn_out);
