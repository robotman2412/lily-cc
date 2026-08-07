
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_compile_stmt.h"

#include "c_ast.h"
#include "c_compile.h"
#include "c_compile_expr.h"
#include "c_ir.h"
#include "c_prim.h"
#include "c_types.h"
#include "compiler.h"
#include "lilycc_malloc.h"
#include "set.h"
#include "unreachable.h"
#include "vec.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>



// Compile a statement.
cir_stmt_t *c_compile2_stmt(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_t const *stmt) {
    switch (stmt->tag) {
        case C_AST_TAG_STMT_FOR: return c_compile2_stmt_for(cc, scope, stmt->stmt_for); break;
        case C_AST_TAG_STMTS: return c_compile2_stmt_stmts(cc, scope, stmt->stmt_stmts); break;
        case C_AST_TAG_STMT_WHILE: return c_compile2_stmt_while(cc, scope, stmt->stmt_while); break;
        case C_AST_TAG_STMT_IF: return c_compile2_stmt_if(cc, scope, stmt->stmt_if); break;
        case C_AST_TAG_STMT_SWITCH: return c_compile2_stmt_switch(cc, scope, stmt->stmt_switch); break;
        case C_AST_TAG_STMT_CASE: return c_compile2_stmt_case(cc, scope, stmt->stmt_case); break;
        case C_AST_TAG_STMT_LABEL: return c_compile2_stmt_label(cc, scope, stmt->stmt_label); break;
        case C_AST_TAG_STMT_RETURN: return c_compile2_stmt_return(cc, scope, stmt->stmt_return); break;
        case C_AST_TAG_STMT_GOTO: return c_compile2_stmt_goto(cc, scope, stmt->stmt_goto); break;
        case C_AST_TAG_STMT_EXPR: return c_compile2_stmt_expr(cc, scope, stmt->stmt_expr); break;
        case C_AST_TAG_STMT_DEF: return c_compile2_stmt_def(cc, scope, stmt->stmt_def);
        case C_AST_TAG_STMT_GARBAGE: return NULL;
    }
    UNREACHABLE();
}


// Compile a statements block.
cir_stmt_t *c_compile2_stmt_stmts(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_list_t const *stmts) {
    cir_scope_t *nested_scope = cir_scope_create(CIR_SCOPE_STMTS, scope);

    bool           errors = false;
    vec_cir_stmt_t res    = {0};
    for (size_t i = 0; i < stmts->items.len; i++) {
        cir_stmt_t *stmt = c_compile2_stmt(cc, nested_scope, stmts->items.arr[i]);
        if (stmt) {
            vec_push(&res, stmt);
        } else {
            errors = true;
        }
    }

    if (errors) {
        for (size_t i = 0; i < res.len; i++) {
            cir_stmt_delete(res.arr[i]);
        }
        vec_clear(&res);
        cir_scope_delete(nested_scope);
        return NULL;
    }

    return cir_stmt_create_stmts(cir_stmts_create(stmts->pos, nested_scope, res));
}

// Compile a for loop statement.
cir_stmt_t *c_compile2_stmt_for(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_for_t const *stmt) {
    cir_scope_t *nested_scope = cir_scope_create(CIR_SCOPE_FOR, scope);
    bool         errors       = false;

    cir_stmt_t *init = NULL;
    if (stmt->init) {
        init = c_compile2_stmt(cc, nested_scope, stmt->init);
        if (!init) {
            errors = true;
        }
    }
    cir_expr_t *cond = NULL;
    if (stmt->cond) {
        cond = c_compile2_expr_exprs(cc, nested_scope, stmt->cond);
        if (!cond) {
            errors = true;
        }
    }
    cir_expr_t *inc = NULL;
    if (stmt->inc) {
        inc = c_compile2_expr_exprs(cc, nested_scope, stmt->inc);
        if (!inc) {
            errors = true;
        }
    }
    if (stmt->body->tag == C_AST_TAG_STMT_DEF) {
        cctx_diagnostic(cc->cctx, stmt->body->pos, DIAG_ERR, "Declaration not allowed here");
    }
    cir_stmt_t *body = c_compile2_stmt(cc, nested_scope, stmt->body);
    if (!body) {
        errors = true;
    }

    if (errors) {
        if (inc) {
            cir_expr_delete(inc);
        }
        if (cond) {
            cir_expr_delete(cond);
        }
        if (init) {
            cir_stmt_delete(init);
        }
        cir_scope_delete(nested_scope);
        return NULL;
    }

    return cir_stmt_create_for(cir_for_create(stmt->pos, nested_scope, init, cond, inc, body));
}

// Compile a while or do...while loop statement.
cir_stmt_t *c_compile2_stmt_while(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_while_t const *stmt) {
    cir_expr_t  *cond         = c_compile2_expr(cc, scope, stmt->cond);
    cir_scope_t *nested_scope = cir_scope_create(CIR_SCOPE_WHILE, scope);
    cir_stmt_t  *body         = c_compile2_stmt(cc, nested_scope, stmt->body);

    if (!cond || !body) {
        if (cond) {
            cir_expr_delete(cond);
        }
        if (body) {
            cir_stmt_delete(body);
        }
        cir_scope_delete(nested_scope);
        return NULL;
    }

    return cir_stmt_create_while(cir_while_create(stmt->pos, nested_scope, cond, body, stmt->is_do_while));
}

// Compile an if...else statement.
cir_stmt_t *c_compile2_stmt_if(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_if_t const *stmt) {
    cir_expr_t *cond      = c_compile2_expr(cc, scope, stmt->cond);
    cir_stmt_t *if_body   = c_compile2_stmt(cc, scope, stmt->if_body);
    cir_stmt_t *else_body = NULL;
    if (stmt->else_body) {
        else_body = c_compile2_stmt(cc, scope, stmt->else_body);
    }

    if (!cond || !if_body || (!else_body && stmt->else_body)) {
        if (cond) {
            cir_expr_delete(cond);
        }
        if (if_body) {
            cir_stmt_delete(if_body);
        }
        if (else_body) {
            cir_stmt_delete(else_body);
        }
        return NULL;
    }

    return cir_stmt_create_if(cir_if_create(stmt->pos, cond, if_body, else_body));
}

// Compile a switch statement.
cir_stmt_t *c_compile2_stmt_switch(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_switch_t const *stmt) {
    fprintf(stderr, "TODO: c_compile2_stmt_switch\n");
    abort();
}

// Compile a case statement.
cir_stmt_t *c_compile2_stmt_case(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_case_t const *stmt) {
    fprintf(stderr, "TODO: c_compile2_stmt_case\n");
    abort();
}

// Compile a label statement.
cir_stmt_t *c_compile2_stmt_label(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_label_t const *stmt) {
    cir_stmt_t *body = NULL;
    if (stmt->body) {
        body = c_compile2_stmt(cc, scope, stmt->body);
        if (!body) {
            return NULL;
        }
    }
    cir_label_t *label = cir_label_create(stmt->pos, lilycc_strdup(stmt->name->name), body);
    if (!cir_scope_add_label(cc->cctx, scope, label)) {
        cir_label_delete(label);
        return NULL;
    }
    return cir_stmt_create_label(label);
}

// Compile a return statement.
cir_stmt_t *c_compile2_stmt_return(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_return_t const *stmt) {
    cir_expr_t *retval = NULL;
    if (stmt->value) {
        retval = c_compile2_expr_exprs(cc, scope, stmt->value);
        if (!retval) {
            return NULL;
        }
    }
    return cir_stmt_create_return(cir_return_create(stmt->pos, retval));
}

// Compile a goto statement.
cir_stmt_t *c_compile2_stmt_goto(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_goto_t const *stmt) {
    (void)cc;
    cir_scope_t *func_scope = cir_scope_func(scope);
    cir_goto_t  *cir_goto   = cir_goto_create(stmt->pos, lilycc_strdup(stmt->target->name));
    if (func_scope) {
        set_add(&func_scope->gotos, cir_goto);
    }
    return cir_stmt_create_goto(cir_goto);
}

// Compile an expression in a statement.
cir_stmt_t *c_compile2_stmt_expr(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_list_t const *stmt) {
    cir_expr_t *expr = c_compile2_expr_exprs(cc, scope, stmt);
    if (!expr) {
        return NULL;
    }
    return cir_stmt_create_expr(expr);
}

// Compile a declaration statement.
cir_stmt_t *c_compile2_stmt_def(c_compiler_t *cc, cir_scope_t *scope, c_ast_def_t const *def) {
    switch (def->tag) {
        case C_AST_TAG_DEFS: {
            c_type_t spec_qual_type = c_compile2_spec_qual_list(cc, def->def_defs->spec_qual, scope);
            if (!c_type_is_valid(spec_qual_type)) {
                return NULL;
            }
            vec_cir_unit_t               units = {0};
            vec_c_ast_init_decl_t const *decls = &def->def_defs->decls->items;

            if (decls->len == 0 && spec_qual_type.prim < C_N_PRIM) {
                cctx_diagnostic(cc->cctx, def->def_defs->pos, DIAG_WARN, "This statement declares nothing");
            }

            for (size_t i = 0; i < decls->len; i++) {
                c_ast_init_decl_t const *decl = decls->arr[i];
                if (spec_qual_type.qual.s_typedef) {
                    c_ast_ident_t const *name;
                    c_type_t             type = c_compile2_type(cc, scope, spec_qual_type, decl->decl, &name);
                    if (!c_type_is_valid(type)) {
                        continue;
                    }
                    assert(name != NULL);
                    if (decl->init) {
                        cctx_diagnostic(cc->cctx, decl->init->pos, DIAG_ERR, "Cannot have initializer for typedef");
                    }
                    cir_scope_add_typedef(cc->cctx, scope, name->name, name->pos, type);

                } else {
                    cir_unit_t *unit = c_compile2_decl(cc, scope, c_type_clone(spec_qual_type), decl);
                    if (unit) {
                        vec_push(&units, unit);
                    }
                }
            }

            c_type_delete(spec_qual_type);
            return cir_stmt_create_units(cir_unit_list_create(def->pos, units));
        }
        case C_AST_TAG_DEF_FUNC: fprintf(stderr, "TODO: Nested functions\n"); abort();
        case C_AST_TAG_DEF_STATIC_ASSERT:
            c_compile2_static_assert(cc, scope, def->def_static_assert);
            return cir_stmt_create_units(cir_unit_list_create(def->pos, (vec_cir_unit_t){0}));
        case C_AST_TAG_DEF_GARBAGE: return NULL;
    }
    UNREACHABLE();
}
