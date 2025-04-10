
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

// Destroy a pre-compilation pass scope.
void c_prescope_destroy(c_prescope_t prescope) {
    map_clear(&prescope.locals);
}



// Pre-compilation pass for expressions.
void c_prepass_expr(c_prepass_t *prepass, c_prescope_t *scope, token_t *expr, bool is_addrof) {
    if (expr->type == TOKENTYPE_IDENT) {
        if (is_addrof) {
            token_t const *tkn = c_prescope_lookup(scope, expr->strval);
            if (tkn) {
                set_add(&prepass->pointer_taken, tkn);
            }
        }
    } else if (expr->type != TOKENTYPE_AST) {
        // Ignored.
    } else if (expr->subtype == C_AST_EXPR_PREFIX || expr->subtype == C_AST_EXPR_SUFFIX) {
        c_prepass_expr(prepass, scope, &expr->params[1], expr->params[0].subtype == C_TKN_AND);
    } else if (expr->subtype == C_AST_EXPR_INFIX) {
        c_prepass_expr(prepass, scope, &expr->params[1], false);
        c_prepass_expr(prepass, scope, &expr->params[2], false);
    } else if (expr->subtype == C_AST_EXPR_INDEX) {
        c_prepass_expr(prepass, scope, &expr->params[0], false);
        c_prepass_expr(prepass, scope, &expr->params[1], false);
    } else if (expr->subtype == C_AST_EXPR_CALL || expr->subtype == C_AST_EXPRS) {
        for (size_t i = 0; i < expr->params_len; i++) {
            c_prepass_expr(prepass, scope, &expr->params[i], false);
        }
    }
}

// Pre-compilation pass for statements.
void c_prepass_stmt(c_prepass_t *prepass, c_prescope_t *scope, token_t *stmt) {
    if (stmt->type != TOKENTYPE_AST) {
        // Ignored.
    } else if (stmt->subtype == C_AST_STMTS) {
        c_prescope_t inner_scope = c_prescope_create(scope);
        for (size_t i = 0; i < stmt->params_len; i++) {
            c_prepass_stmt(prepass, scope, &stmt->params[i]);
        }
        c_prescope_destroy(inner_scope);
    } else if (stmt->subtype == C_AST_EXPRS) {
        c_prepass_expr(prepass, scope, stmt, false);
    } else if (stmt->subtype == C_AST_DECLS) {
        c_prepass_decls(prepass, scope, stmt);
    } else if (stmt->subtype == C_AST_DO_WHILE || stmt->subtype == C_AST_WHILE) {
        c_prepass_expr(prepass, scope, &stmt->params[0], false);
        c_prepass_stmt(prepass, scope, &stmt->params[1]);
    } else if (stmt->subtype == C_AST_FOR_LOOP) {
        c_prepass_decls(prepass, scope, &stmt->params[0]);
        c_prepass_expr(prepass, scope, &stmt->params[1], false);
        c_prepass_expr(prepass, scope, &stmt->params[2], false);
        c_prepass_stmt(prepass, scope, &stmt->params[3]);
    } else if (stmt->subtype == C_AST_IF_ELSE) {
        c_prepass_expr(prepass, scope, &stmt->params[0], false);
        c_prepass_stmt(prepass, scope, &stmt->params[1]);
        if (stmt->params_len == 3) {
            c_prepass_stmt(prepass, scope, &stmt->params[2]);
        }
    } else if (stmt->subtype == C_AST_RETURN) {
        if (stmt->params_len) {
            c_prepass_expr(prepass, scope, &stmt->params[0], false);
        }
    }
}

// Pre-compilation pass for declarations.
void c_prepass_decls(c_prepass_t *prepass, c_prescope_t *scope, token_t *decls) {
    for (size_t i = 1; i < decls->params_len; i++) {
        c_prepass_decl(prepass, scope, &decls->params[i]);
    }
}

// Pre-compilation pass for a single direct (abstract) declarator.
void c_prepass_decl(c_prepass_t *prepass, c_prescope_t *scope, token_t *decl) {
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
        }
    }
}



// Run the pre-compilation pass for a C function.
c_prepass_t c_precompile_pass(token_t *def) {
    c_prepass_t prepass = {
        .pointer_taken = STR_SET_EMPTY,
    };
    c_prescope_t scope = c_prescope_create(NULL);
    token_t     *decl  = &def->params[1];
    for (size_t i = 1; i < decl->params_len; i++) {
        c_prepass_decls(&prepass, &scope, &decl->params[i]);
    }
    token_t *body = &def->params[2];
    for (size_t i = 0; i < body->params_len; i++) {
        c_prepass_stmt(&prepass, &scope, &body->params[i]);
    }
    c_prescope_destroy(scope);
    return prepass;
}

// Clean up pre-compilation pass information.
void c_prepass_destroy(c_prepass_t prepass) {
    set_clear(&prepass.pointer_taken);
}
