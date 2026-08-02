
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "c_ast.h"
#include "c_compiler.h"
#include "c_ir.h"
#include "c_types.h"



// Compile one entire C translation unit into C IR.
// Returns `NULL` if a a semantic error occurred (`-Werror` excluded).
cir_trans_unit_t *c_compile2(c_compiler_t *cc, c_ast_def_list_t const *ast);

// Compile a static assertion unit.
void c_compile2_static_assert(c_compiler_t *cc, cir_scope_t *scope, c_ast_def_static_assert_t *s_assert);
// Compile a declaration unit.
// Produces a function definition or variable definition depending on the type encoded.
cir_unit_t *
    c_compile2_decl(c_compiler_t *cc, cir_scope_t *scope, c_type_t spec_qual_type, c_ast_init_decl_t const *decl);
// Compile a function definition unit.
cir_unit_t *c_compile2_func(c_compiler_t *cc, cir_scope_t *scope, c_ast_def_func_t const *def);

// Create a C type from a specifier-qualifer list.
c_type_opt_t c_compile2_spec_qual_list(c_compiler_t *cc, c_ast_spec_qual_list_t const *list, cir_scope_t *scope);
// Compile the type encoded by a declaration or type name.
c_type_opt_t c_compile2_type(
    c_compiler_t         *cc,
    cir_scope_t          *scope,
    c_type_t              spec_qual_type,
    c_ast_decl_t const   *decl,
    c_ast_ident_t const **name_out
);
