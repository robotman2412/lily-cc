
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_compile.h"



// Compile one entire C translation unit into C IR.
// Returns `NULL` if a a semantic error occurred (`-Werror` excluded).
cir_trans_unit_t *c_compile2(c_compiler_t *cc) {
    fprintf(stderr, "TODO: c_compile2\n");
    abort();
}


// Compile a global unit.
// Returns `NULL` if a semantic error occurred, or it is a `_Static_assert`.
cir_unit_t *c_compile2_unit(c_compiler_t *cc, cir_scope_t *scope, c_ast_def_t const *def) {
    fprintf(stderr, "TODO: c_compile2_unit\n");
    abort();
}

// Compile a declaration unit.
// Produces a function definition or variable definition depending on the type encoded.
cir_unit_t *c_compile2_decl(
    c_compiler_t *cc, cir_scope_t *scope, c_ast_spec_qual_list_t const *spec_qual, c_ast_decl_t const *decl
) {
    fprintf(stderr, "TODO: c_compile2_decl\n");
    abort();
}

// Compile a function definition unit.
cir_func_t *c_compile2_func(c_compiler_t *cc, cir_scope_t *scope, c_ast_def_func_t const *def) {
    fprintf(stderr, "TODO: c_compile2_func\n");
    abort();
}
