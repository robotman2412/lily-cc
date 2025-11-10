
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "set.h"
#include "tokenizer.h"



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
    // A function invocation or cast (e.g. `identifier(exprs)` or `(exprs)(exprs)`).
    // Args: Function, args.
    C_AST_EXPR_CALL,
    // A cast (e.g. `(typename) expr`).
    // Args: Type, expr.
    C_AST_EXPR_CAST,
    // An array type (e.g. `int[]` or `int[expr]`).
    // Args: Thing the array binds to, index expression (optional).
    C_AST_TYPE_ARRAY,
    // Type pointer node (e.g. the `*` in `int *[2]` or `int (*)[2]`).
    // Args: Pointer node/token, type spec list (optional), thing that the pointer binds to (optional).
    C_AST_TYPE_PTR_TO,
    // Type of a function; separate node to distinguish between calls and types.
    // Args: Return type, zero or more argument types.
    C_AST_TYPE_FUNC,
    // Struct/union/enum name/definition/declaration node.
    // Args: Keyword, name, definition (optional).
    C_AST_NAMED_STRUCT,
    // Struct/union/enum reference.
    // Args: Keyword, definition.
    C_AST_ANON_STRUCT,
    // Type name node (e.g. `mytype_t` or `int *[2][3]`).
    // Args: Type specifier/qualifier list, pointer/index node.
    C_AST_TYPE_NAME,
    // Type specifier/qualifier list (e.g. `const int` or `extern size_t`). Re-used for subsets of this list.
    // Args: List of type specifier and/or qualifier tokens.
    C_AST_SPEC_QUAL_LIST,
    // Declaration list.
    // Args: Type specifier/qualifier list, (assignment) declarators.
    C_AST_DECLS,
    // Assignment declarator.
    // Args: Declarator, expression.
    C_AST_ASSIGN_DECL,
    // Enum variant.
    // Args: Identifier, expression (optional).
    C_AST_ENUM_VARIANT,
    // Function definition.
    // Args: Type specifier/qualifier list, declarator, body.
    C_AST_FUNC_DEF,
    // One or more statements.
    // Args: Statements.
    C_AST_STMTS,
    // For loop.
    // Args: Setup, condition, increment, body.
    C_AST_FOR_LOOP,
    // While loop.
    // Args: Condition, body.
    C_AST_WHILE,
    // Do ... while loop.
    // Args: Condition, body.
    C_AST_DO_WHILE,
    // If statement.
    // Args: Condition, if body, else body (optional).
    C_AST_IF_ELSE,
    // Switch statement.
    // Args: Statements and/or case labels.
    C_AST_SWITCH,
    // Case label.
    // Args: Value or lower limit, upper limit.
    C_AST_CASE_LABEL,
    // Named label.
    // Args: Identifier.
    C_AST_LABEL,
    // Return statement.
    // Args: Expression (optional).
    C_AST_RETURN,
    // Goto statement.
    // Args: Ident.
    C_AST_GOTO,
    // Empty expression or statement.
    // Args: Node.
    C_AST_NOP,
} c_asttype_t;

// C parser context.
typedef struct {
    // Tokenizer to use.
    tokenizer_t *tkn_ctx;
    // Set of type names; this makes parsing a great deal easier.
    set_t        type_names;
    // Local set of type names (types local to a function).
    set_t        local_type_names;
    // Currently parsing a function body.
    bool         func_body;
} c_parser_t;


#ifndef NDEBUG
// Enum names of `c_asttype_t` values.
extern char const *const c_asttype_name[];
#endif


// Parse a C compilation unit into an AST.
token_t c_parse(c_parser_t *ctx);

// Parse one or more C expressions separated by commas.
token_t c_parse_exprs(c_parser_t *ctx);
// Parse a C expression.
token_t c_parse_expr(c_parser_t *ctx);
// Parse a type name.
token_t c_parse_type_name(c_parser_t *ctx);
// Parse a type specifier/qualifier list.
token_t c_parse_spec_qual_list(c_parser_t *ctx, bool *is_typedef_out);
// Parse a variable/function declaration/definition.
token_t c_parse_decls(c_parser_t *ctx, bool allow_func_body);
// Parse a struct or union specifier/definition.
token_t c_parse_struct_spec(c_parser_t *ctx);
// Parse an enum specifier/definition.
token_t c_parse_enum_spec(c_parser_t *ctx);
// Parse a statment.
token_t c_parse_stmt(c_parser_t *ctx);
// Parse multiple statments.
token_t c_parse_stmts(c_parser_t *ctx);

#ifndef NDEBUG
// Print a token.
void c_tkn_debug_print(token_t token);
// Build a test case that asserts an exact value for a token.
void c_tkn_debug_testcase(token_t token);
#endif
