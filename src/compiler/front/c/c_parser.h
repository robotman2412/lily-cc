
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "c_tokenizer.h"
#include "compiler.h"



// C AST subtypes.
typedef enum {
    // Garbage/malformed tokens and/or AST nodes.
    // Args: One or more tokens and/or AST nodes.
    C_AST_GARBAGE,
    // One or more comma-separated expressions (e.g. `(expr, expr)` or `(expr)`).
    // Args: The expressions in question.
    C_AST_EXPRS,
    // An infix operator expression (e.g. `expr + expr`).
    // Args: Operator token, LHS, RHS.
    C_AST_EXPR_INFIX,
    // A unary prefix operator expression (e.g. `~expr`).
    // Args: Operator token, operand.
    C_AST_EXPR_PREFIX,
    // A unary suffix operator expression (e.g. `expr++`).
    // Args: Operator token, operand.
    C_AST_EXPR_SUFFIX,
    // An array indexing expression (e.g. `expr[expr]`).
    // Args: Array, index.
    C_AST_EXPR_INDEX,
    // A function invocation or cast (e.g. `identifier(exprs)`, `(typename) expr` or `(exprs)(exprs)`).
    // Args: Function/type, args.
    C_AST_EXPR_CALL,
    // Type pointer with qualifier node (e.g. the `*const` in `int *const`).
    // Args: List of qualifier tokens.
    C_AST_TYPE_PTR_QUAL,
    // Type pointer node (e.g. the `*` in `int *[2]` or `int (*)[2]`).
    // Args: Pointer node/token, type that the pointer points to.
    C_AST_TYPE_PTR_TO,
    // Struct/union/enum name/definition/declaration node.
    // Args: Keyword, name and/or definition.
    C_AST_STRUCT,
    // Type name node (e.g. `mytype_t` or `int *[2][3]`).
    // Args: Type specifier/qualifier list, pointer/index node.
    C_AST_TYPE_NAME,
    // Type specifier/qualifier list (e.g. `const int` or `extern size_t`).
    // Args: List of type specifier and/or qualifier tokens.
    C_AST_SPEC_QUAL_LIST,
} c_asttype_t;


#ifndef NDEBUG
// Enum names of `c_asttype_t` values.
extern char const *const c_asttype_name[];
#endif


// Parse a C compilation unit into an AST.
token_t c_parse(tokenizer_t *tkn_ctx);

// Parse one or more C expressions separated by commas.
token_t c_parse_exprs(tokenizer_t *tkn_ctx);
// Parse a C expression.
token_t c_parse_expr(tokenizer_t *tkn_ctx);
// Parse a type name.
token_t c_parse_type_name(tokenizer_t *tkn_ctx);
// Parse a type specifier/qualifier list.
token_t c_parse_spec_qual_list(tokenizer_t *tkn_ctx);
// Parse a variable/function declaration/definition.
token_t c_parse_decls(tokenizer_t *tkn_ctx);
// Parse a struct or union specifier/definition.
token_t c_parse_struct_spec(tokenizer_t *tkn_ctx);
// Parse an enum specifier/definition.
token_t c_parse_enum_spec(tokenizer_t *tkn_ctx);

#ifndef NDEBUG
// Print a token.
void c_tkn_debug_print(token_t token);
// Build a test case that asserts an exact value for a token.
void c_tkn_debug_testcase(token_t token);
#endif
