
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "c_ast.h"
#include "set.h"
#include "tokenizer.h"



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



// Parse a whole translation unit (all global declarations until EOF).
c_ast_def_list_t *c_parse2(c_parser_t *ctx);

// Parse a compound initializer.
c_ast_init_list_t      *c_parse2_comp_init(c_parser_t *ctx);
// Parse one or more C expressions separated by commas.
c_ast_expr_list_t      *c_parse2_exprs(c_parser_t *ctx);
// Parse a C expression.
c_ast_expr_t           *c_parse2_expr(c_parser_t *ctx);
// Parse a type name.
c_ast_type_name_t      *c_parse2_type_name(c_parser_t *ctx);
// Parse a type specifier/qualifier list.
c_ast_spec_qual_list_t *c_parse2_spec_qual_list(c_parser_t *ctx, bool *is_typedef_out);
// Parse a variable/function declaration/definition.
c_ast_def_t            *c_parse2_def(c_parser_t *ctx, bool allow_func_body);
// Parse a struct or union specifier/definition.
c_ast_struct_spec_t    *c_parse2_struct_spec(c_parser_t *ctx);
// Parse an enum specifier/definition.
c_ast_enum_spec_t      *c_parse2_enum_spec(c_parser_t *ctx);
// Parse a statment.
c_ast_stmt_t           *c_parse2_stmt(c_parser_t *ctx);
// Parse multiple statments.
c_ast_stmt_list_t      *c_parse2_stmts(c_parser_t *ctx);
