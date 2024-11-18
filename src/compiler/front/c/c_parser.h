
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#pragma once

#include "c_tokenizer.h"
#include "compiler.h"



// C AST subtypes.
typedef enum {
    // Garbage/malformed tokens and/or AST nodes.
    // Args: One or more tokens and/or AST nodes.
    C_AST_GARBAGE,
    // One or more comma-separated expressions.
    // Args: The expressions in question.
    C_AST_EXPRS,
    // An infix operator expression.
    // Args: Operator token, LHS, RHS.
    C_AST_EXPR_INFIX,
    // A unary prefix operator expression.
    // Args: Operator token, operand.
    C_AST_EXPR_PREFIX,
    // A unary suffix operator expression.
    // Args: Operator token, operand.
    C_AST_EXPR_SUFFIX,
    // An array indexing expression.
    // Args: Array, index.
    C_AST_EXPR_INDEX,
    // A function invocation.
    // Args: Function, args.
    C_AST_EXPR_CALL,
} c_asttype_t;


#ifndef NDEBUG
// Enum names of `c_asttype_t` values.
extern char const *const c_asttype_name[];
#endif


// Parse a C compilation unit into an AST.
token_t c_parse(tokenizer_t *tkn_ctx);

// Parse a function call expression.
token_t c_parse_funccall(tokenizer_t *tkn_ctx, token_t funcname);
// Parse one or more expressions or a type.
token_t c_parse_exprs_or_type(tokenizer_t *tkn_ctx);
// Parse one or more C expressions separated by commas.
token_t c_parse_exprs(tokenizer_t *tkn_ctx);
// Parse a C expression.
token_t c_parse_expr(tokenizer_t *tkn_ctx);
// Try to parse a C statement.
bool    c_parse_stmt(tokenizer_t *tkn_ctx, token_t *tkn_out);
// Try to parse a C variable declaration.
bool    c_parse_vardecl(tokenizer_t *tkn_ctx, token_t *tkn_out);
// Try to parse a C if statement.
bool    c_parse_if_stmt(tokenizer_t *tkn_ctx, token_t *tkn_out);
// Try to parse a C for loop.
bool    c_parse_for_loop(tokenizer_t *tkn_ctx, token_t *tkn_out);
// Try to parse a C while loop.
bool    c_parse_while_loop(tokenizer_t *tkn_ctx, token_t *tkn_out);

#ifndef NDEBUG
// Print a token.
void c_tkn_debug_print(token_t token);
#endif
