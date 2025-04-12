
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "c_std.h"
#include "c_tokenizer.h"
#include "map.h"
#include "set.h"



// Precompilation pass information.
typedef struct c_prepass   c_prepass_t;
// Precompilation pass scope information.
typedef struct c_prescope  c_prescope_t;
// Pre-compilation pass branching path info.
// Does not apply to labels, which Lily-CC opts not to analyze.
// A single switch-case body is treated as a single path, not one per case.
typedef struct c_prebranch c_prebranch_t;



// Pre-compilation pass information.
struct c_prepass {
    // Hash set of variables that had a pointer taken by pointer to identifier token; set of `token_t const *`.
    set_t pointer_taken;
    // Map of branch AST nodes to sets of variables referenced in that branch;
    // `token_t const *` -> `set_t *` of `token_t const *`.
    map_t branch_vars;
};

// Pre-compilation pass scope information.
struct c_prescope {
    // Parent scope.
    c_prescope_t const *parent;
    // Map of names to local variables; `char *` -> `token_t const *`.
    map_t               locals;
};

// Pre-compilation pass branching path info.
// Does not apply to labels, which Lily-CC opts not to analyze.
// A single switch-case body is treated as a single path, not one per case.
struct c_prebranch {
    // Parent branch.
    c_prebranch_t const *parent;
    // Nearest parent scope.
    c_prescope_t const  *scope;
    // Set of variables referenced in this path.
    // Applies only to variables created outside the branch.
    set_t               *vars_referenced;
};



// Create a new pre-compilation pass scope.
c_prescope_t   c_prescope_create(c_prescope_t const *parent);
// Look up a variable in a pre-compilation pass scope.
token_t const *c_prescope_lookup(c_prescope_t const *scope, char const *name);
// Look up a variable in a pre-compilation pass scope but only if it's within the given parent scope.
token_t const *c_prescope_lookup_parent(c_prescope_t const *scope, c_prescope_t const *parent, char const *name);
// Destroy a pre-compilation pass scope.
void           c_prescope_destroy(c_prescope_t scope);

// Create new pre-compilation pass branching path information.
c_prebranch_t c_prebranch_create(
    c_prepass_t *prepass, token_t const *node, c_prescope_t const *scope, c_prebranch_t const *parent
);

// Pre-compilation pass for expressions.
void c_prepass_expr(
    c_prepass_t *prepass, c_prescope_t *scope, c_prebranch_t *branch, token_t const *expr, bool is_addrof
);
// Pre-compilation pass for statements.
void c_prepass_stmt(c_prepass_t *prepass, c_prescope_t *scope, c_prebranch_t *branch, token_t const *stmt);
// Pre-compilation pass for declarations.
void c_prepass_decls(c_prepass_t *prepass, c_prescope_t *scope, c_prebranch_t *branch, token_t const *decls);
// Pre-compilation pass for a single (abstract) declarator.
void c_prepass_decl(c_prepass_t *prepass, c_prescope_t *scope, c_prebranch_t *branch, token_t const *decl);

// Run the pre-compilation pass for a C function.
c_prepass_t c_precompile_pass(token_t const *def);
// Clean up pre-compilation pass information.
void        c_prepass_destroy(c_prepass_t prepass);
