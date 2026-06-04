
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "arith128.h"
#include "c_ast.h"
#include "c_compiler.h"
#include "c_ir.h"
#include "c_types.h"



// Compile an expression.
// Returns `NULL` on semantic errors.
cir_expr_t *c_compile2_expr(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_t const *expr);

// Compile a ternary expression.
cir_expr_t *c_compile2_expr_ternary(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_ternary_t const *expr);
// Compile an index expression.
cir_expr_t *c_compile2_expr_index(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_index_t const *expr);
// Compile an infix expression.
cir_expr_t *c_compile2_expr_infix(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_infix_t const *expr);
// Compile a prefix expression.
cir_expr_t *c_compile2_expr_prefix(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_prefix_t const *expr);
// Compile a suffix expression.
cir_expr_t *c_compile2_expr_suffix(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_suffix_t const *expr);
// Compile a cast expression.
cir_expr_t *c_compile2_expr_cast(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_cast_t const *cast);
// Compile a call expression.
cir_expr_t *c_compile2_expr_call(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_call_t const *call);
// Compile an identifier as part of an expression.
cir_expr_t *c_compile2_expr_ident(c_compiler_t *cc, cir_scope_t *scope, c_ast_ident_t const *ident);
// Compile an integer constant as part of an expression.
cir_expr_t *c_compile2_expr_iconst(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_iconst_t const *iconst);
// Compile a string constant as part of an expression.
cir_expr_t *c_compile2_expr_sconst(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_sconst_t const *sconst);
// Compile a compound literal as part of an expression.
cir_expr_t *
    c_compile2_expr_compliteral(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_compliteral_t const *compliteral);
// Compile an expression list.
cir_expr_t *c_compile2_expr_exprs(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_list_t const *exprs);

// Compile a compound literal/initializer given a known target type.
cir_value_t *c_compile2_compinit(c_compiler_t *cc, cir_scope_t *scope, c_ast_init_list_t const *init);

// Multiply/divide a value by the size of the inner type of a given pointer type.
// Compile error if the inner type of the pointer is an incomplete type.
// Needed because pointer arithmetic in the C IR does not respect the inner type's size.
cir_expr_t *c_compile2_ptr_premul(c_compiler_t *cc, cir_expr_t *value, c_type_t const *ptr_type, bool is_division);
// Helper that creates a synthetic integer constant.
cir_expr_t *c_compile2_synth_iconst(c_compiler_t *cc, pos_t pos, c_prim_t prim, i128_t value);
