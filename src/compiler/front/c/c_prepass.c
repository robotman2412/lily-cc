
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_prepass.h"

#include "c_parser.h"



// Create a new pre-compilation pass scope.
c_prescope_t c_prescope_create(c_prescope_t const *parent) {
    return (c_prescope_t){
        .parent = parent,
        .locals = STR_MAP_EMPTY,
    };
}

// Look up a variable in a pre-compilation pass scope.
token_t const *c_prescope_lookup(c_prescope_t const *scope, char const *name) {
    while (scope) {
        token_t const *token = map_get(&scope->locals, name);
        if (token) {
            return token;
        }
        scope = scope->parent;
    }
    return NULL;
}

// Look up a variable in a pre-compilation pass scope but only if it's within the given parent scope.
token_t const *c_prescope_lookup_parent(c_prescope_t const *scope, c_prescope_t const *parent, char const *name) {
    bool in_parent = false;
    while (scope) {
        if (scope == parent) {
            in_parent = true;
        }
        token_t const *token = map_get(&scope->locals, name);
        if (token) {
            return in_parent ? token : NULL;
        }
        scope = scope->parent;
    }
    return NULL;
}

// Destroy a pre-compilation pass scope.
void c_prescope_destroy(c_prescope_t prescope) {
    map_clear(&prescope.locals);
}



// Create new pre-compilation pass branching path information.
c_prebranch_t c_prebranch_create(
    c_prepass_t *prepass, token_t const *node, c_prescope_t const *scope, c_prebranch_t const *parent
) {
    c_prebranch_t branch = {
        .parent          = parent,
        .scope           = scope,
        .vars_referenced = strong_calloc(1, sizeof(set_t)),
    };
    *branch.vars_referenced = PTR_SET_EMPTY;
    map_set(&prepass->branch_vars, node, branch.vars_referenced);
    return branch;
}



// Pre-compilation pass for expressions.
void c_prepass_expr(
    c_prepass_t *prepass, c_prescope_t *scope, c_prebranch_t *branch, token_t const *expr, bool is_addrof
) {
    if (expr->type == TOKENTYPE_IDENT) {
        if (is_addrof) {
            // Mark variable as having a pointer taken.
            token_t const *tkn = c_prescope_lookup(scope, expr->strval);
            if (tkn) {
                set_add(&prepass->pointer_taken, tkn);
            }
        } else {
            // Check if the variable needs to be registered in the branching path.
            // It should only be added if it exists in the nearest parent scope to the branch.
            // It's also not checked when the address is taken because that isn't actually reading or writing it.
            while (branch) {
                token_t const *tkn = c_prescope_lookup_parent(scope, branch->scope, expr->strval);
                if (tkn) {
                    set_add(branch->vars_referenced, tkn);
                }
                branch = (c_prebranch_t *)branch->parent;
            }
        }
    } else if (expr->type != TOKENTYPE_AST) {
        // Ignored.
    } else if (expr->subtype == C_AST_EXPR_PREFIX || expr->subtype == C_AST_EXPR_SUFFIX) {
        c_prepass_expr(prepass, scope, branch, &expr->params[1], expr->params[0].subtype == C_TKN_AND);
    } else if (expr->subtype == C_AST_EXPR_INFIX) {
        c_prepass_expr(prepass, scope, branch, &expr->params[1], false);
        c_prepass_expr(prepass, scope, branch, &expr->params[2], false);
    } else if (expr->subtype == C_AST_EXPR_INDEX) {
        c_prepass_expr(prepass, scope, branch, &expr->params[0], false);
        c_prepass_expr(prepass, scope, branch, &expr->params[1], false);
    } else if (expr->subtype == C_AST_EXPR_CALL || expr->subtype == C_AST_EXPRS) {
        for (size_t i = 0; i < expr->params_len; i++) {
            c_prepass_expr(prepass, scope, branch, &expr->params[i], false);
        }
    }
}

// Pre-compilation pass for statements.
void c_prepass_stmt(c_prepass_t *prepass, c_prescope_t *scope, c_prebranch_t *branch, token_t const *stmt) {
    if (stmt->type != TOKENTYPE_AST) {
        // Ignored.
    } else if (stmt->subtype == C_AST_STMTS) {
        c_prescope_t inner_scope = c_prescope_create(scope);
        for (size_t i = 0; i < stmt->params_len; i++) {
            c_prepass_stmt(prepass, scope, branch, &stmt->params[i]);
        }
        c_prescope_destroy(inner_scope);
    } else if (stmt->subtype == C_AST_EXPRS) {
        c_prepass_expr(prepass, scope, branch, stmt, false);
    } else if (stmt->subtype == C_AST_DECLS) {
        c_prepass_decls(prepass, scope, branch, stmt);
    } else if (stmt->subtype == C_AST_DO_WHILE || stmt->subtype == C_AST_WHILE) {
        c_prepass_expr(prepass, scope, branch, &stmt->params[0], false);
        c_prepass_stmt(prepass, scope, branch, &stmt->params[1]);
    } else if (stmt->subtype == C_AST_FOR_LOOP) {
        c_prepass_decls(prepass, scope, branch, &stmt->params[0]);
        c_prebranch_t inner_branch = c_prebranch_create(prepass, stmt, scope, branch);
        c_prepass_expr(prepass, scope, &inner_branch, &stmt->params[1], false);
        c_prepass_expr(prepass, scope, &inner_branch, &stmt->params[2], false);
        c_prepass_stmt(prepass, scope, &inner_branch, &stmt->params[3]);
    } else if (stmt->subtype == C_AST_IF_ELSE) {
        c_prepass_expr(prepass, scope, branch, &stmt->params[0], false);
        c_prebranch_t inner_branch = c_prebranch_create(prepass, stmt, scope, branch);
        c_prepass_stmt(prepass, scope, &inner_branch, &stmt->params[1]);
        if (stmt->params_len == 3) {
            c_prepass_stmt(prepass, scope, &inner_branch, &stmt->params[2]);
        }
    } else if (stmt->subtype == C_AST_RETURN) {
        if (stmt->params_len) {
            // The branch is dropped here because the return ending the function
            // causes the in-memory state of variables to become irrelevant.
            c_prepass_expr(prepass, scope, NULL, &stmt->params[0], false);
        }
    }
}

// Pre-compilation pass for declarations.
void c_prepass_decls(c_prepass_t *prepass, c_prescope_t *scope, c_prebranch_t *branch, token_t const *decls) {
    for (size_t i = 1; i < decls->params_len; i++) {
        c_prepass_decl(prepass, scope, branch, &decls->params[i]);
    }
}

// Pre-compilation pass for a single direct (abstract) declarator.
void c_prepass_decl(c_prepass_t *prepass, c_prescope_t *scope, c_prebranch_t *branch, token_t const *decl) {
    if (decl->type == TOKENTYPE_AST && decl->subtype == C_AST_ASSIGN_DECL) {
        decl = &decl->params[0];
    }
    while (1) {
        if (decl->type == TOKENTYPE_IDENT) {
            if (!map_get(&scope->locals, decl->strval)) {
                map_set(&scope->locals, decl->strval, decl);
            }
            return;

        } else if (decl->type == TOKENTYPE_AST && decl->subtype == C_AST_NOP) {
            return;

        } else if (decl->type == TOKENTYPE_AST && decl->subtype == C_AST_TYPE_FUNC) {
            decl = &decl->params[0];

        } else if (decl->type == TOKENTYPE_AST && decl->subtype == C_AST_TYPE_ARRAY) {
            if (decl->params_len) {
                // Inner node.
                decl = &decl->params[0];
            } else {
                // No inner node.
                return;
            }

        } else if (decl->type == TOKENTYPE_AST && decl->subtype == C_AST_TYPE_PTR_TO) {
            if (decl->params_len > 1 && decl->params[1].type == TOKENTYPE_AST
                && decl->params[1].subtype == C_AST_SPEC_QUAL_LIST) {
                if (decl->params_len > 2) {
                    // Inner node.
                    decl = &decl->params[2];
                } else {
                    // No inner node.
                    return;
                }
            } else if (decl->params_len > 1) {
                // Inner node.
                decl = &decl->params[1];
            } else {
                // No inner node.
                return;
            }
        } else {
            abort();
        }
    }
}



// Run the pre-compilation pass for a C function.
c_prepass_t c_precompile_pass(token_t const *def) {
    c_prepass_t prepass = {
        .pointer_taken = PTR_SET_EMPTY,
        .branch_vars   = PTR_MAP_EMPTY,
    };
    c_prescope_t   scope = c_prescope_create(NULL);
    token_t const *decl  = &def->params[1];
    for (size_t i = 1; i < decl->params_len; i++) {
        c_prepass_decls(&prepass, &scope, NULL, &decl->params[i]);
    }
    token_t const *body = &def->params[2];
    for (size_t i = 0; i < body->params_len; i++) {
        c_prepass_stmt(&prepass, &scope, NULL, &body->params[i]);
    }
    c_prescope_destroy(scope);
    return prepass;
}

// Clean up pre-compilation pass information.
void c_prepass_destroy(c_prepass_t prepass) {
    set_clear(&prepass.pointer_taken);
    map_foreach(ent, &prepass.branch_vars) {
        set_clear(ent->value);
        free(ent->value);
    }
    map_clear(&prepass.branch_vars);
}
