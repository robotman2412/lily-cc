
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "c_std.h"
#include "c_tokenizer.h"
#include "map.h"
#include "set.h"



// Precompilation pass information.
typedef struct c_prepass  c_prepass_t;
// Precompilation pass scope information.
typedef struct c_prescope c_prescope_t;
// Pre-compilation pass variable.
typedef struct c_prevar   c_prevar_t;



// Pre-compilation pass information.
struct c_prepass {
    // Hash set of variables that had a pointer taken by pointer to identifier token; set of `token_t const *`.
    set_t pointer_taken;
};

// Pre-compilation pass scope information.
struct c_prescope {
    // Parent scope.
    c_prescope_t const *parent;
    // Map of names to local variables; `char *` -> `token_t const *`.
    map_t               locals;
};



// Create a new pre-compilation pass scope.
c_prescope_t   c_prescope_create(c_prescope_t const *parent);
// Look up a variable in a pre-compilation pass scope.
token_t const *c_prescope_lookup(c_prescope_t const *scope, char const *name);
// Destroy a pre-compilation pass scope.
void           c_prescope_destroy(c_prescope_t scope);

// Pre-compilation pass for expressions.
void c_prepass_expr(c_prepass_t *prepass, c_prescope_t *scope, token_t *expr, bool is_addrof);
// Pre-compilation pass for statements.
void c_prepass_stmt(c_prepass_t *prepass, c_prescope_t *scope, token_t *stmt);
// Pre-compilation pass for declarations.
void c_prepass_decls(c_prepass_t *prepass, c_prescope_t *scope, token_t *decls);
// Pre-compilation pass for a single (abstract) declarator.
void c_prepass_decl(c_prepass_t *prepass, c_prescope_t *scope, token_t *decl);

// Run the pre-compilation pass for a C function.
c_prepass_t c_precompile_pass(token_t *def);
// Clean up pre-compilation pass information.
void        c_prepass_destroy(c_prepass_t prepass);
