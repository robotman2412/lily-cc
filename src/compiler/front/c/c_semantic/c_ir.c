
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_ir.h"

#include "c_prim.h"
#include "c_types.h"
#include "compiler.h"
#include "ir_serialization.h"
#include "lilycc_malloc.h"
#include "map.h"
#include "vec.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>



static inline void cir_expr_common_delete(cir_expr_common_t common) {
    c_type_delete(common.type);
}

cir_expr_common_t cir_expr_common_clone(cir_expr_common_t const *common) {
    cir_expr_common_t new = *common;
    new.type              = c_type_clone(common->type);
    return new;
}


cir_const_t *cir_const_create(pos_t pos, c_prim_t prim, ir_const_t iconst) {
    cir_const_t *node = lilycc_malloc(sizeof(cir_const_t));
    assert(prim < C_N_PRIM);
    node->pos    = pos;
    node->prim   = prim;
    node->iconst = iconst;
    return node;
}

void cir_const_delete(cir_const_t *node) {
    lilycc_free(node);
}


cir_comp_const_t *cir_comp_const_create(pos_t pos, c_type_t type, uint8_t *blob) {
    cir_comp_const_t *node = lilycc_malloc(sizeof(cir_comp_const_t));
    node->pos              = pos;
    node->type             = type;
    node->blob             = blob;
    return node;
}

void cir_comp_const_delete(cir_comp_const_t *node) {
    c_type_delete(node->type);
    lilycc_free(node->blob);
    lilycc_free(node);
}


cir_comp_value_t *cir_comp_value_create(pos_t pos, c_type_t type, vec_cir_comp_store_t stores) {
    cir_comp_value_t *node = lilycc_malloc(sizeof(cir_comp_value_t));
    node->pos              = pos;
    node->type             = type;
    node->stores           = stores;
    return node;
}

void cir_comp_value_delete(cir_comp_value_t *node) {
    c_type_delete(node->type);
    for (size_t i = 0; i < node->stores.len; i++) {
        cir_expr_delete(node->stores.arr[i].value);
    }
    vec_clear(&node->stores);
    lilycc_free(node);
}


cir_value_t *cir_value_create_tmpval(cir_tmpval_t const *tmpval) {
    cir_value_t *node = lilycc_malloc(sizeof(cir_value_t));
    node->common      = cir_expr_common_clone(&tmpval->common);
    node->tag         = CIR_VALUE_TMPVAL;
    node->tmpval      = tmpval;
    return node;
}

cir_value_t *cir_value_create_scope_val(cir_scope_val_t const *scope_val) {
    cir_value_t *node = lilycc_malloc(sizeof(cir_value_t));
    switch (scope_val->tag) {
        case CIR_SCOPE_VAL_DECL:
            node->common = (cir_expr_common_t){
                .pos          = scope_val->decl->pos,
                .is_lvalue    = true,
                .allow_addrof = true,
                .type         = c_type_clone(scope_val->decl->type),
            };
            break;
        case CIR_SCOPE_VAL_FUNC:
            node->common = (cir_expr_common_t){
                .pos          = scope_val->func->pos,
                .is_lvalue    = true,
                .allow_addrof = true,
                .type         = c_type_clone(scope_val->func->type),
            };
            break;
        case CIR_SCOPE_VAL_ENUM_CONST:
            node->common = (cir_expr_common_t){
                .pos          = scope_val->enum_const->pos,
                .is_lvalue    = false,
                .allow_addrof = false,
                .type         = C_TYPE_FROM_PRIM(C_PRIM_SINT), // TODO: Packed enums.
            };
            break;
    }
    node->tag       = CIR_VALUE_SCOPE_VAL;
    node->scope_val = scope_val;
    return node;
}

cir_value_t *cir_value_create_const(cir_expr_common_t common, cir_const_t *iconst) {
    cir_value_t *node = lilycc_malloc(sizeof(cir_value_t));
    node->common      = common;
    node->common.pos  = iconst->pos;
    node->tag         = CIR_VALUE_CONST;
    node->iconst      = iconst;
    return node;
}

cir_value_t *cir_value_create_comp_const(cir_expr_common_t common, cir_comp_const_t *comp_const) {
    cir_value_t *node = lilycc_malloc(sizeof(cir_value_t));
    node->common      = common;
    node->common.pos  = comp_const->pos;
    node->tag         = CIR_VALUE_COMP_CONST;
    node->comp_const  = comp_const;
    return node;
}

cir_value_t *cir_value_create_comp_value(cir_expr_common_t common, cir_comp_value_t *comp_value) {
    cir_value_t *node = lilycc_malloc(sizeof(cir_value_t));
    node->common      = common;
    node->common.pos  = comp_value->pos;
    node->tag         = CIR_VALUE_COMP_VALUE;
    node->comp_value  = comp_value;
    return node;
}

void cir_value_delete(cir_value_t *node) {
    cir_expr_common_delete(node->common);
    switch (node->tag) {
        case CIR_VALUE_TMPVAL:
        case CIR_VALUE_SCOPE_VAL: break;
        case CIR_VALUE_CONST: cir_const_delete(node->iconst); break;
        case CIR_VALUE_COMP_CONST: cir_comp_const_delete(node->comp_const); break;
        case CIR_VALUE_COMP_VALUE: cir_comp_value_delete(node->comp_value); break;
    }
    lilycc_free(node);
}


cir_call_t *cir_call_create(cir_expr_common_t common, cir_expr_t *func, vec_cir_expr_t args) {
    cir_call_t *node = lilycc_malloc(sizeof(cir_call_t));
    node->common     = common;
    node->func       = func;
    node->args       = args;
    return node;
}

void cir_call_delete(cir_call_t *node) {
    cir_expr_common_delete(node->common);
    cir_expr_delete(node->func);
    for (size_t i = 0; i < node->args.len; i++) {
        cir_expr_delete(node->args.arr[i]);
    }
    vec_clear(&node->args);
    lilycc_free(node);
}


cir_cast_t *cir_cast_create(cir_expr_common_t common, cir_expr_t *value) {
    cir_cast_t *node = lilycc_malloc(sizeof(cir_cast_t));
    node->common     = common;
    node->value      = value;
    return node;
}

void cir_cast_delete(cir_cast_t *node) {
    cir_expr_common_delete(node->common);
    cir_expr_delete(node->value);
    lilycc_free(node);
}


cir_ternary_t *
    cir_ternary_create(cir_expr_common_t common, cir_expr_t *cond, cir_expr_t *if_expr, cir_expr_t *else_expr) {
    cir_ternary_t *node = lilycc_malloc(sizeof(cir_ternary_t));
    node->common        = common;
    node->cond          = cond;
    node->if_expr       = if_expr;
    node->else_expr     = else_expr;
    return node;
}

void cir_ternary_delete(cir_ternary_t *node) {
    cir_expr_common_delete(node->common);
    cir_expr_delete(node->cond);
    if (node->if_expr) {
        cir_expr_delete(node->if_expr);
    }
    cir_expr_delete(node->else_expr);
    lilycc_free(node);
}


cir_calc_t *cir_calc_create(cir_expr_common_t common, cir_calc_op_t op, cir_expr_t *lhs, cir_expr_t *rhs) {
    cir_calc_t *node = lilycc_malloc(sizeof(cir_calc_t));
    node->common     = common;
    node->op         = op;
    node->lhs        = lhs;
    node->rhs        = rhs;
    return node;
}

void cir_calc_delete(cir_calc_t *node) {
    cir_expr_common_delete(node->common);
    cir_expr_delete(node->lhs);
    cir_expr_delete(node->rhs);
    lilycc_free(node);
}


cir_addrof_t *cir_addrof_create(cir_expr_common_t common, cir_expr_t *expr) {
    cir_addrof_t *node = lilycc_malloc(sizeof(cir_addrof_t));
    node->common       = common;
    node->expr         = expr;
    return node;
}

void cir_addrof_delete(cir_addrof_t *node) {
    cir_expr_common_delete(node->common);
    cir_expr_delete(node->expr);
    lilycc_free(node);
}


cir_deref_t *cir_deref_create(cir_expr_common_t common, cir_expr_t *expr) {
    cir_deref_t *node = lilycc_malloc(sizeof(cir_deref_t));
    node->common      = common;
    node->expr        = expr;
    return node;
}

void cir_deref_delete(cir_deref_t *node) {
    cir_expr_common_delete(node->common);
    cir_expr_delete(node->expr);
    lilycc_free(node);
}


cir_tmpval_t *cir_tmpval_create(cir_expr_t *inner) {
    cir_tmpval_t *node = lilycc_malloc(sizeof(cir_tmpval_t));
    node->common       = cir_expr_common_clone(&inner->common);
    node->inner        = inner;
    return node;
}

void cir_tmpval_delete(cir_tmpval_t *node) {
    cir_expr_delete(node->inner);
    cir_expr_common_delete(node->common);
    lilycc_free(node);
}


cir_exprs_t *cir_exprs_create(cir_expr_common_t common, vec_cir_expr_t exprs) {
    return cir_exprs_create2(common, (vec_cir_tmpval_t){0}, exprs);
}

cir_exprs_t *cir_exprs_create2(cir_expr_common_t common, vec_cir_tmpval_t tmpvals, vec_cir_expr_t exprs) {
    cir_exprs_t *node = lilycc_malloc(sizeof(cir_exprs_t));
    node->common      = common;
    node->tmpvals     = tmpvals;
    node->exprs       = exprs;
    return node;
}

void cir_exprs_delete(cir_exprs_t *node) {
    cir_expr_common_delete(node->common);
    for (size_t i = 0; i < node->exprs.len; i++) {
        cir_expr_delete(node->exprs.arr[i]);
    }
    vec_clear(&node->tmpvals);
    vec_clear(&node->exprs);
    lilycc_free(node);
}


cir_assign_t *cir_assign_create(cir_expr_t *lhs, cir_expr_t *rhs) {
    cir_assign_t *node        = lilycc_malloc(sizeof(cir_assign_t));
    node->common              = cir_expr_common_clone(&lhs->common);
    node->common.allow_addrof = false;
    node->common.is_lvalue    = false;
    node->common.type.qual    = (c_qual_t){0};
    node->lhs                 = lhs;
    node->rhs                 = rhs;
    return node;
}

void cir_assign_delete(cir_assign_t *node) {
    cir_expr_common_delete(node->common);
    cir_expr_delete(node->lhs);
    cir_expr_delete(node->rhs);
    lilycc_free(node);
}


cir_expr_t *cir_expr_create_value(cir_value_t *value) {
    cir_expr_t *node = lilycc_malloc(sizeof(cir_expr_t));
    node->common     = cir_expr_common_clone(&value->common);
    node->tag        = CIR_EXPR_VALUE;
    node->value      = value;
    return node;
}

cir_expr_t *cir_expr_create_call(cir_call_t *call) {
    cir_expr_t *node = lilycc_malloc(sizeof(cir_expr_t));
    node->common     = cir_expr_common_clone(&call->common);
    node->tag        = CIR_EXPR_CALL;
    node->call       = call;
    return node;
}

cir_expr_t *cir_expr_create_cast(cir_cast_t *cast) {
    cir_expr_t *node = lilycc_malloc(sizeof(cir_expr_t));
    node->common     = cir_expr_common_clone(&cast->common);
    node->tag        = CIR_EXPR_CAST;
    node->cast       = cast;
    return node;
}

cir_expr_t *cir_expr_create_ternary(cir_ternary_t *ternary) {
    cir_expr_t *node = lilycc_malloc(sizeof(cir_expr_t));
    node->common     = cir_expr_common_clone(&ternary->common);
    node->tag        = CIR_EXPR_TERNARY;
    node->ternary    = ternary;
    return node;
}

cir_expr_t *cir_expr_create_calc(cir_calc_t *calc) {
    cir_expr_t *node = lilycc_malloc(sizeof(cir_expr_t));
    node->common     = cir_expr_common_clone(&calc->common);
    node->tag        = CIR_EXPR_CALC;
    node->calc       = calc;
    return node;
}

cir_expr_t *cir_expr_create_addrof(cir_addrof_t *addrof) {
    cir_expr_t *node = lilycc_malloc(sizeof(cir_expr_t));
    node->common     = cir_expr_common_clone(&addrof->common);
    node->tag        = CIR_EXPR_ADDROF;
    node->addrof     = addrof;
    return node;
}

cir_expr_t *cir_expr_create_deref(cir_deref_t *deref) {
    cir_expr_t *node = lilycc_malloc(sizeof(cir_expr_t));
    node->common     = cir_expr_common_clone(&deref->common);
    node->tag        = CIR_EXPR_DEREF;
    node->deref      = deref;
    return node;
}

cir_expr_t *cir_expr_create_exprs(cir_exprs_t *exprs) {
    if (exprs->exprs.len == 1 && exprs->tmpvals.len == 0) {
        cir_expr_t *expr = exprs->exprs.arr[0];
        vec_clear(&exprs->exprs);
        cir_expr_common_delete(exprs->common);
        lilycc_free(exprs);
        return expr;
    }

    cir_expr_t *node = lilycc_malloc(sizeof(cir_expr_t));
    node->common     = cir_expr_common_clone(&exprs->common);
    node->tag        = CIR_EXPR_EXPRS;
    node->exprs      = exprs;
    return node;
}

cir_expr_t *cir_expr_create_assign(cir_assign_t *assign) {
    cir_expr_t *node = lilycc_malloc(sizeof(cir_expr_t));
    node->common     = cir_expr_common_clone(&assign->common);
    node->tag        = CIR_EXPR_ASSIGN;
    node->assign     = assign;
    return node;
}

cir_expr_t *cir_expr_create_stmt(cir_stmt_t *stmt) {
    cir_expr_t *node = lilycc_malloc(sizeof(cir_expr_t));
    node->common     = (cir_expr_common_t){
        .pos          = stmt->pos,
        .type         = C_TYPE_FROM_PRIM(C_PRIM_VOID),
        .is_lvalue    = false,
        .allow_addrof = false,
    };
    node->tag  = CIR_EXPR_ASSIGN;
    node->stmt = stmt;
    return node;
}

void cir_expr_delete(cir_expr_t *node) {
    cir_expr_common_delete(node->common);
    switch (node->tag) {
        case CIR_EXPR_VALUE: cir_value_delete(node->value); break;
        case CIR_EXPR_CALL: cir_call_delete(node->call); break;
        case CIR_EXPR_CAST: cir_cast_delete(node->cast); break;
        case CIR_EXPR_TERNARY: cir_ternary_delete(node->ternary); break;
        case CIR_EXPR_CALC: cir_calc_delete(node->calc); break;
        case CIR_EXPR_ADDROF: cir_addrof_delete(node->addrof); break;
        case CIR_EXPR_DEREF: cir_deref_delete(node->deref); break;
        case CIR_EXPR_EXPRS: cir_exprs_delete(node->exprs); break;
        case CIR_EXPR_ASSIGN: cir_assign_delete(node->assign); break;
        case CIR_EXPR_STMT: cir_stmt_delete(node->stmt); break;
    }
    lilycc_free(node);
}


cir_for_t *cir_for_create(
    pos_t pos, cir_scope_t *scope, cir_stmt_t *init, cir_expr_t *cond, cir_expr_t *inc, cir_stmt_t *body
) {
    cir_for_t *node = lilycc_malloc(sizeof(cir_for_t));
    node->pos       = pos;
    node->scope     = scope;
    node->init      = init;
    node->cond      = cond;
    node->inc       = inc;
    node->body      = body;
    return node;
}

void cir_for_delete(cir_for_t *node) {
    cir_stmt_delete(node->body);
    if (node->inc) {
        cir_expr_delete(node->inc);
    }
    if (node->cond) {
        cir_expr_delete(node->cond);
    }
    if (node->init) {
        cir_stmt_delete(node->init);
    }
    cir_scope_delete(node->scope);
    lilycc_free(node);
}


cir_while_t *cir_while_create(pos_t pos, cir_scope_t *scope, cir_expr_t *cond, cir_stmt_t *body, bool is_do_while) {
    cir_while_t *node = lilycc_malloc(sizeof(cir_while_t));
    node->pos         = pos;
    node->scope       = scope;
    node->cond        = cond;
    node->body        = body;
    node->is_do_while = is_do_while;
    return node;
}

void cir_while_delete(cir_while_t *node) {
    cir_expr_delete(node->cond);
    cir_stmt_delete(node->body);
    cir_scope_delete(node->scope);
    lilycc_free(node);
}


cir_stmts_t *cir_stmts_create(pos_t pos, cir_scope_t *scope, vec_cir_stmt_t stmts) {
    cir_stmts_t *node = lilycc_malloc(sizeof(cir_stmts_t));
    node->pos         = pos;
    node->scope       = scope;
    node->stmts       = stmts;
    return node;
}

void cir_stmts_delete(cir_stmts_t *node) {
    for (size_t i = 0; i < node->stmts.len; i++) {
        cir_stmt_delete(node->stmts.arr[i]);
    }
    cir_scope_delete(node->scope);
    vec_clear(&node->stmts);
    lilycc_free(node);
}


cir_if_t *cir_if_create(pos_t pos, cir_expr_t *cond, cir_stmt_t *if_body, cir_stmt_t *else_body) {
    cir_if_t *node  = lilycc_malloc(sizeof(cir_if_t));
    node->pos       = pos;
    node->cond      = cond;
    node->if_body   = if_body;
    node->else_body = else_body;
    return node;
}

void cir_if_delete(cir_if_t *node) {
    cir_expr_delete(node->cond);
    cir_stmt_delete(node->if_body);
    if (node->else_body) {
        cir_stmt_delete(node->else_body);
    }
    lilycc_free(node);
}


cir_label_t *cir_label_create(pos_t pos, char *name, cir_stmt_t *body) {
    cir_label_t *node = lilycc_malloc(sizeof(cir_label_t));
    node->pos         = pos;
    node->name        = name;
    node->body        = body;
    return node;
}

void cir_label_delete(cir_label_t *node) {
    lilycc_free(node->name);
    cir_stmt_delete(node->body);
    lilycc_free(node);
}


cir_return_t *cir_return_create(pos_t pos, cir_expr_t *value) {
    cir_return_t *node = lilycc_malloc(sizeof(cir_return_t));
    node->pos          = pos;
    node->value        = value;
    return node;
}

void cir_return_delete(cir_return_t *node) {
    if (node->value) {
        cir_expr_delete(node->value);
    }
    lilycc_free(node);
}


cir_stmt_t *cir_stmt_create_stmts(cir_stmts_t *stmts) {
    cir_stmt_t *node = lilycc_malloc(sizeof(cir_stmt_t));
    node->pos        = stmts->pos;
    node->tag        = CIR_STMT_STMTS;
    node->stmts      = stmts;
    return node;
}

cir_stmt_t *cir_stmt_create_for(cir_for_t *for_loop) {
    cir_stmt_t *node = lilycc_malloc(sizeof(cir_stmt_t));
    node->pos        = for_loop->pos;
    node->tag        = CIR_STMT_FOR;
    node->for_loop   = for_loop;
    return node;
}

cir_stmt_t *cir_stmt_create_while(cir_while_t *while_loop) {
    cir_stmt_t *node = lilycc_malloc(sizeof(cir_stmt_t));
    node->pos        = while_loop->pos;
    node->tag        = CIR_STMT_WHILE;
    node->while_loop = while_loop;
    return node;
}

cir_stmt_t *cir_stmt_create_if(cir_if_t *if_stmt) {
    cir_stmt_t *node = lilycc_malloc(sizeof(cir_stmt_t));
    node->pos        = if_stmt->pos;
    node->tag        = CIR_STMT_IF;
    node->if_stmt    = if_stmt;
    return node;
}

cir_stmt_t *cir_stmt_create_label(cir_label_t *label) {
    cir_stmt_t *node = lilycc_malloc(sizeof(cir_stmt_t));
    node->pos        = label->pos;
    node->tag        = CIR_STMT_LABEL;
    node->label      = label;
    return node;
}

cir_stmt_t *cir_stmt_create_return(cir_return_t *return_stmt) {
    cir_stmt_t *node  = lilycc_malloc(sizeof(cir_stmt_t));
    node->pos         = return_stmt->pos;
    node->tag         = CIR_STMT_RETURN;
    node->return_stmt = return_stmt;
    return node;
}

cir_stmt_t *cir_stmt_create_expr(cir_expr_t *expr) {
    cir_stmt_t *node = lilycc_malloc(sizeof(cir_stmt_t));
    node->pos        = expr->common.pos;
    node->tag        = CIR_STMT_EXPR;
    node->expr       = expr;
    return node;
}

cir_stmt_t *cir_stmt_create_units(cir_unit_list_t *units) {
    cir_stmt_t *node = lilycc_malloc(sizeof(cir_stmt_t));
    node->pos        = units->pos;
    node->tag        = CIR_STMT_UNITS;
    node->units      = units;
    return node;
}

void cir_stmt_delete(cir_stmt_t *node) {
    switch (node->tag) {
        case CIR_STMT_STMTS: cir_stmts_delete(node->stmts); break;
        case CIR_STMT_FOR: cir_for_delete(node->for_loop); break;
        case CIR_STMT_WHILE: cir_while_delete(node->while_loop); break;
        case CIR_STMT_IF: cir_if_delete(node->if_stmt); break;
        case CIR_STMT_LABEL: cir_label_delete(node->label); break;
        case CIR_STMT_RETURN: cir_return_delete(node->return_stmt); break;
        case CIR_STMT_EXPR: cir_expr_delete(node->expr); break;
        case CIR_STMT_UNITS: cir_unit_list_delete(node->units); break;
    }
    lilycc_free(node);
}


cir_decl_t *cir_decl_create(pos_t pos, c_type_t type, char *name, cir_expr_t *init) {
    cir_decl_t *node = lilycc_malloc(sizeof(cir_decl_t));
    node->pos        = pos;
    node->type       = type;
    node->name       = name;
    node->init       = init;
    return node;
}

void cir_decl_delete(cir_decl_t *node) {
    c_type_delete(node->type);
    lilycc_free(node->name);
    if (node->init) {
        cir_expr_delete(node->init);
    }
    lilycc_free(node);
}


cir_func_t *cir_func_create(pos_t pos, cir_scope_t *scope, c_type_t type, char *name, vec_cir_stmt_t body) {
    cir_func_t *node = lilycc_malloc(sizeof(cir_func_t));
    node->pos        = pos;
    node->scope      = scope;
    node->type       = type;
    node->name       = name;
    node->body       = body;
    return node;
}

void cir_func_delete(cir_func_t *node) {
    c_type_delete(node->type);
    lilycc_free(node->name);
    for (size_t i = 0; i < node->body.len; i++) {
        cir_stmt_delete(node->body.arr[i]);
    }
    vec_clear(&node->body);
    cir_scope_delete(node->scope);
    lilycc_free(node);
}


cir_unit_t *cir_unit_create_decl(cir_decl_t *decl) {
    cir_unit_t *node = lilycc_malloc(sizeof(cir_unit_t));
    node->pos        = decl->pos;
    node->tag        = CIR_UNIT_DECL;
    node->decl       = decl;
    return node;
}

cir_unit_t *cir_unit_create_func(cir_func_t *func) {
    cir_unit_t *node = lilycc_malloc(sizeof(cir_unit_t));
    node->pos        = func->pos;
    node->tag        = CIR_UNIT_FUNC;
    node->func       = func;
    return node;
}

void cir_unit_delete(cir_unit_t *node) {
    switch (node->tag) {
        case CIR_UNIT_DECL: cir_decl_delete(node->decl); break;
        case CIR_UNIT_FUNC: cir_func_delete(node->func); break;
    }
    lilycc_free(node);
}


cir_unit_list_t *cir_unit_list_create(pos_t pos, vec_cir_unit_t units) {
    cir_unit_list_t *node = lilycc_malloc(sizeof(cir_unit_list_t));
    node->pos             = pos;
    node->units           = units;
    return node;
}

void cir_unit_list_delete(cir_unit_list_t *node) {
    for (size_t i = 0; i < node->units.len; i++) {
        cir_unit_delete(node->units.arr[i]);
    }
    vec_clear(&node->units);
    lilycc_free(node);
}


cir_trans_unit_t *cir_trans_unit_create(cir_scope_t *scope, vec_cir_unit_t units) {
    cir_trans_unit_t *node = lilycc_malloc(sizeof(cir_trans_unit_t));
    node->scope            = scope;
    node->units            = units;
    return node;
}

void cir_trans_unit_delete(cir_trans_unit_t *node) {
    for (size_t i = 0; i < node->units.len; i++) {
        cir_unit_delete(node->units.arr[i]);
    }
    cir_scope_delete(node->scope);
    vec_clear(&node->units);
    lilycc_free(node);
}


cir_scope_t *cir_scope_create(cir_scope_kind_t kind, cir_scope_t *parent) {
    cir_scope_t *scope = lilycc_malloc(sizeof(cir_scope_t));
    scope->kind        = kind;
    scope->parent      = parent;
    scope->values      = STR_MAP_EMPTY;
    scope->typedefs    = STR_MAP_EMPTY;
    scope->tags        = STR_MAP_EMPTY;
    scope->labels      = STR_MAP_EMPTY;
    return scope;
}

void cir_scope_delete(cir_scope_t *scope) {
    map_foreach_value(cir_scope_val_t, val, &scope->values) {
        if (val->tag == CIR_SCOPE_VAL_ENUM_CONST) {
            lilycc_free(val->enum_const);
        }
        lilycc_free(val);
    }
    map_clear(&scope->values);
    map_foreach(ent, &scope->typedefs) {
        c_type_delete(*(c_type_t const *)ent->value);
        lilycc_free(ent->value);
    }
    map_clear(&scope->typedefs);
    map_foreach(ent, &scope->tags) {
        c_comp_type_delete(ent->value);
    }
    map_clear(&scope->tags);
    // Labels are non-owning; just drop the map.
    map_clear(&scope->labels);
    lilycc_free(scope);
}

// Walk up to the enclosing function scope, or `NULL` if none.
static cir_scope_t *cir_scope_func(cir_scope_t *scope) {
    while (scope && scope->kind != CIR_SCOPE_FUNC) {
        scope = scope->parent;
    }
    return scope;
}

static void redef_diag(cctx_t *ctx, char const *name, pos_t redef, pos_t orig) {
    cctx_diagnostic(ctx, redef, DIAG_ERR, "Redefinition of %s", name);
    cctx_diagnostic(ctx, orig, DIAG_HINT, "Original definition of %s", name);
}

// Allocate and insert a value entry. Returns `false` if `name` already exists.
static bool cir_scope_add_value(cctx_t *ctx, cir_scope_t *scope, char const *name, cir_scope_val_t val) {
    while (scope->kind == CIR_SCOPE_WHILE) {
        scope = scope->parent;
    }
    assert(scope != NULL);

    cir_scope_val_t const *exist = map_get(&scope->values, name);
    if (exist) {
        redef_diag(ctx, name, *val.pos, *exist->pos);
        goto error;
    }

    cir_typedef_t const *conflict = map_get(&scope->typedefs, name);
    if (conflict) {
        redef_diag(ctx, name, *val.pos, conflict->pos);
        goto error;
    }

    cir_scope_val_t *entry = lilycc_malloc(sizeof(cir_scope_val_t));
    *entry                 = val;
    map_set(&scope->values, name, entry);
    return true;

error:
    switch (val.tag) {
        case CIR_SCOPE_VAL_DECL: cir_decl_delete(val.decl); break;
        case CIR_SCOPE_VAL_FUNC: cir_func_delete(val.func); break;
        case CIR_SCOPE_VAL_ENUM_CONST: cir_const_delete(val.enum_const); break;
    }
    return false;
}

bool cir_scope_add_decl(cctx_t *ctx, cir_scope_t *scope, cir_decl_t *decl) {
    return cir_scope_add_value(ctx, scope, decl->name, (cir_scope_val_t){.tag = CIR_SCOPE_VAL_DECL, .decl = decl});
}

bool cir_scope_add_func(cctx_t *ctx, cir_scope_t *scope, cir_func_t *func) {
    return cir_scope_add_value(ctx, scope, func->name, (cir_scope_val_t){.tag = CIR_SCOPE_VAL_FUNC, .func = func});
}

bool cir_scope_add_enum_const(cctx_t *ctx, cir_scope_t *scope, char const *name, cir_const_t *enum_const) {
    return cir_scope_add_value(
        ctx,
        scope,
        name,
        (cir_scope_val_t){.tag = CIR_SCOPE_VAL_ENUM_CONST, .enum_const = enum_const}
    );
}

bool cir_scope_add_typedef(cctx_t *ctx, cir_scope_t *scope, char const *name, pos_t pos, c_type_t type) {
    cir_typedef_t const *exist = map_get(&scope->typedefs, name);
    if (exist) {
        c_type_delete(type);
        if (c_type_is_identical(exist->type, type, true)) {
            return true;
        }
        redef_diag(ctx, name, pos, exist->pos);
        return false;
    }

    cir_scope_val_t const *conflict = map_get(&scope->values, name);
    if (conflict) {
        c_type_delete(type);
        redef_diag(ctx, name, pos, *conflict->pos);
        return false;
    }

    cir_typedef_t *box = lilycc_malloc(sizeof(cir_typedef_t));
    box->type          = type;
    box->pos           = pos;
    map_set(&scope->typedefs, name, box);
    return true;
}

bool cir_scope_add_tag(cctx_t *ctx, cir_scope_t *scope, c_comp_type_t *type) {
    c_comp_type_t const *exist = map_get(&scope->tags, type->name);
    if (exist) {
        redef_diag(ctx, type->name, type->pos, exist->pos);
        c_comp_type_delete(type);
        return false;
    }
    map_set(&scope->tags, type->name, type);
    return true;
}

bool cir_scope_add_label(cctx_t *ctx, cir_scope_t *scope, cir_label_t *label) {
    cir_scope_t *func_scope = cir_scope_func(scope);
    if (!func_scope) {
        cctx_diagnostic(ctx, label->pos, DIAG_ERR, "Cannot add label outside a function");
        return false;
    }
    cir_label_t const *exist = map_get(&func_scope->labels, label->name);
    if (exist) {
        redef_diag(ctx, label->name, label->pos, exist->pos);
        return false;
    }
    map_set(&func_scope->labels, label->name, label);
    return true;
}

cir_scope_val_t *cir_scope_lookup_value(cir_scope_t const *scope, char const *name) {
    for (; scope; scope = scope->parent) {
        cir_scope_val_t *val = map_get(&scope->values, name);
        if (val) {
            return val;
        }
    }
    return NULL;
}

cir_typedef_t const *cir_scope_lookup_typedef(cir_scope_t const *scope, char const *name) {
    for (; scope; scope = scope->parent) {
        cir_typedef_t const *type = map_get(&scope->typedefs, name);
        if (type) {
            return type;
        }
    }
    return NULL;
}

c_comp_type_t *cir_scope_lookup_tag(cir_scope_t const *scope, char const *name) {
    for (; scope; scope = scope->parent) {
        c_comp_type_t *type = map_get(&scope->tags, name);
        if (type) {
            return type;
        }
    }
    return NULL;
}

cir_label_t *cir_scope_lookup_label(cir_scope_t const *scope, char const *name) {
    cir_scope_t *func_scope = cir_scope_func((cir_scope_t *)scope);
    if (!func_scope) {
        return NULL;
    }
    return map_get(&func_scope->labels, name);
}



static void pindent(int indent, FILE *to) {
    while (indent > 0) {
        fputs("  ", to);
        indent--;
    }
}


void cir_scope_dbg(cir_scope_t const *scope, int indent, FILE *to) {
    indent++;
    fprintf(to, "scope %p\n", scope);

    pindent(indent, to);
    fputs("kind: ", to);
    switch (scope->kind) {
        case CIR_SCOPE_GLOBAL: fputs("global\n", to); break;
        case CIR_SCOPE_FUNC: fputs("func\n", to); break;
        case CIR_SCOPE_STMTS: fputs("stmts\n", to); break;
        case CIR_SCOPE_SWITCH: fputs("switch\n", to); break;
        case CIR_SCOPE_WHILE: fputs("while\n", to); break;
        case CIR_SCOPE_FOR: fputs("for\n", to); break;
    }

    if (scope->parent) {
        pindent(indent, to);
        fprintf(to, "parent: %p\n", scope->parent);
    }

    map_foreach_kv(char const *, name, cir_scope_val_t, val, &scope->values) {
        pindent(indent, to);
        fprintf(to, "values[\"%s\"]: ", name);
        cir_scope_val_dbg(val, indent, to);
    }
}

void cir_scope_val_dbg(cir_scope_val_t const *scope_val, int indent, FILE *to) {
    indent++;
    fputs("scope_val:", to);
    switch (scope_val->tag) {
        case CIR_SCOPE_VAL_DECL: fprintf(to, "decl %p\n", scope_val->decl); break;
        case CIR_SCOPE_VAL_FUNC: fprintf(to, "func %p\n", scope_val->func); break;

        case CIR_SCOPE_VAL_ENUM_CONST:
            fprintf(
                to,
                "enum_const @ %s:%d:%d\n",
                scope_val->enum_const->pos.srcfile->name,
                scope_val->enum_const->pos.line + 1,
                scope_val->enum_const->pos.col + 1
            );

            pindent(indent, to);
            fputs("prim: ", to);
            c_prim_print(scope_val->enum_const->prim, to);
            fputc('\n', to);

            pindent(indent, to);
            fputs("iconst: ", to);
            ir_const_serialize(scope_val->enum_const->iconst, to);
            fputc('\n', to);
            break;
    }
}

void cir_typedef_dbg(cir_typedef_t const *cir_typedef, int indent, FILE *to) {
    indent++;
    fprintf(
        to,
        "decl @ %s:%d:%d\n",
        cir_typedef->pos.srcfile->name,
        cir_typedef->pos.line + 1,
        cir_typedef->pos.col + 1
    );

    pindent(indent, to);
    fputs("type: ", to);
    c_type_print(cir_typedef->type, NULL, true, to);
    fputc('\n', to);
}


void cir_const_dbg(cir_const_t const *iconst, int indent, FILE *to) {
    indent++;
    fprintf(to, "const @ %s:%d:%d\n", iconst->pos.srcfile->name, iconst->pos.line + 1, iconst->pos.col + 1);

    pindent(indent, to);
    fputs("type: ", to);
    c_prim_print(iconst->prim, to);
    fputc('\n', to);

    pindent(indent, to);
    fputs("iconst: ", to);
    ir_const_serialize(iconst->iconst, to);
    fputc('\n', to);
}

void cir_comp_const_dbg(cir_comp_const_t const *comp_const, int indent, FILE *to) {
    indent++;
    fprintf(
        to,
        "comp_const @ %s:%d:%d\n",
        comp_const->pos.srcfile->name,
        comp_const->pos.line + 1,
        comp_const->pos.col + 1
    );

    pindent(indent, to);
    fputs("type: ", to);
    c_type_print(comp_const->type, NULL, true, to);
    fputc('\n', to);
}

void cir_comp_store_dbg(cir_comp_store_t const *comp_store, int indent, FILE *to) {
    indent++;
    fprintf(to, "comp_store\n");

    pindent(indent, to);
    fprintf(to, "offset: %" PRIu64 "\n", comp_store->offset);

    pindent(indent, to);
    fputs("value: ", to);
    cir_expr_dbg(comp_store->value, indent, to);
}

void cir_comp_value_dbg(cir_comp_value_t const *comp_value, int indent, FILE *to) {
    indent++;
    fprintf(
        to,
        "comp_value @ %s:%d:%d\n",
        comp_value->pos.srcfile->name,
        comp_value->pos.line + 1,
        comp_value->pos.col + 1
    );

    pindent(indent, to);
    fputs("type: ", to);
    c_type_print(comp_value->type, NULL, true, to);
    fputc('\n', to);
}

void cir_value_dbg(cir_value_t const *value, int indent, FILE *to) {
    fputs("value:", to);
    switch (value->tag) {
        case CIR_VALUE_TMPVAL: fprintf(to, "tmpval %p\n", value->tmpval); break;
        case CIR_VALUE_SCOPE_VAL:
            switch (value->scope_val->tag) {
                case CIR_SCOPE_VAL_DECL: fprintf(to, "scope_val:decl \"%s\"\n", value->scope_val->decl->name); break;
                case CIR_SCOPE_VAL_FUNC: fprintf(to, "scope_val:func \"%s\"\n", value->scope_val->func->name); break;
                case CIR_SCOPE_VAL_ENUM_CONST:
                    fputs("scope_val:enum_const\n", to);

                    pindent(indent + 1, to);
                    fputs("type: ", to);
                    c_prim_print(value->scope_val->enum_const->prim, to);
                    fputc('\n', to);

                    pindent(indent + 1, to);
                    fputs("iconst: ", to);
                    ir_const_serialize(value->scope_val->enum_const->iconst, to);
                    fputc('\n', to);
                    break;
            }
            break;
        case CIR_VALUE_CONST: cir_const_dbg(value->iconst, indent, to); break;
        case CIR_VALUE_COMP_CONST: cir_comp_const_dbg(value->comp_const, indent, to); break;
        case CIR_VALUE_COMP_VALUE: cir_comp_value_dbg(value->comp_value, indent, to); break;
    }
}


void cir_call_dbg(cir_call_t const *call, int indent, FILE *to) {
    indent++;
    fprintf(
        to,
        "call @ %s:%d:%d\n",
        call->common.pos.srcfile->name,
        call->common.pos.line + 1,
        call->common.pos.col + 1
    );

    pindent(indent, to);
    fputs("type: ", to);
    c_type_print(call->common.type, NULL, true, to);
    fputc('\n', to);

    pindent(indent, to);
    fputs(call->common.allow_addrof ? "addrof, " : "", to);
    fputs(call->common.is_lvalue ? "lvalue\n" : "rvalue\n", to);

    pindent(indent, to);
    fputs("func: ", to);
    cir_expr_dbg(call->func, indent, to);

    for (size_t i = 0; i < call->args.len; i++) {
        pindent(indent, to);
        fprintf(to, "args[%zu]: ", i);
        cir_expr_dbg(call->args.arr[i], indent, to);
    }
}

void cir_cast_dbg(cir_cast_t const *cast, int indent, FILE *to) {
    indent++;
    fprintf(
        to,
        "cast @ %s:%d:%d\n",
        cast->common.pos.srcfile->name,
        cast->common.pos.line + 1,
        cast->common.pos.col + 1
    );

    pindent(indent, to);
    fputs("type: ", to);
    c_type_print(cast->common.type, NULL, true, to);
    fputc('\n', to);

    pindent(indent, to);
    fputs(cast->common.allow_addrof ? "addrof, " : "", to);
    fputs(cast->common.is_lvalue ? "lvalue\n" : "rvalue\n", to);

    pindent(indent, to);
    fputs("value: ", to);
    cir_expr_dbg(cast->value, indent, to);
}

void cir_ternary_dbg(cir_ternary_t const *ternary, int indent, FILE *to) {
    indent++;
    fprintf(
        to,
        "ternary @ %s:%d:%d\n",
        ternary->common.pos.srcfile->name,
        ternary->common.pos.line + 1,
        ternary->common.pos.col + 1
    );

    pindent(indent, to);
    fputs("type: ", to);
    c_type_print(ternary->common.type, NULL, true, to);
    fputc('\n', to);

    pindent(indent, to);
    fputs(ternary->common.allow_addrof ? "addrof, " : "", to);
    fputs(ternary->common.is_lvalue ? "lvalue\n" : "rvalue\n", to);

    pindent(indent, to);
    fputs("cond: ", to);
    cir_expr_dbg(ternary->cond, indent, to);

    pindent(indent, to);
    fputs("if_expr: ", to);
    cir_expr_dbg(ternary->if_expr, indent, to);

    pindent(indent, to);
    fputs("else_expr: ", to);
    cir_expr_dbg(ternary->else_expr, indent, to);
}

void cir_calc_dbg(cir_calc_t const *calc, int indent, FILE *to) {
    indent++;
    fprintf(
        to,
        "calc @ %s:%d:%d\n",
        calc->common.pos.srcfile->name,
        calc->common.pos.line + 1,
        calc->common.pos.col + 1
    );

    pindent(indent, to);
    fputs("type: ", to);
    c_type_print(calc->common.type, NULL, true, to);
    fputc('\n', to);

    pindent(indent, to);
    fputs(calc->common.allow_addrof ? "addrof, " : "", to);
    fputs(calc->common.is_lvalue ? "lvalue\n" : "rvalue\n", to);

    pindent(indent, to);
    fputs("op: ", to);
    switch (calc->op) {
        case CIR_CALC_ADD: fputs("add", to); break;
        case CIR_CALC_SUB: fputs("sub", to); break;
        case CIR_CALC_MUL: fputs("mul", to); break;
        case CIR_CALC_DIV: fputs("div", to); break;
        case CIR_CALC_MOD: fputs("mod", to); break;
        case CIR_CALC_SHL: fputs("shl", to); break;
        case CIR_CALC_SHR: fputs("shr", to); break;
        case CIR_CALC_BAND: fputs("band", to); break;
        case CIR_CALC_BOR: fputs("bor", to); break;
        case CIR_CALC_BXOR: fputs("bxor", to); break;
        case CIR_CALC_LAND: fputs("land", to); break;
        case CIR_CALC_LOR: fputs("lor", to); break;
        case CIR_CALC_EQ: fputs("eq", to); break;
        case CIR_CALC_NE: fputs("ne", to); break;
        case CIR_CALC_LT: fputs("lt", to); break;
        case CIR_CALC_LE: fputs("le", to); break;
        case CIR_CALC_GT: fputs("gt", to); break;
        case CIR_CALC_GE: fputs("ge", to); break;
    }
    fputc('\n', to);

    pindent(indent, to);
    fputs("lhs: ", to);
    cir_expr_dbg(calc->lhs, indent, to);

    pindent(indent, to);
    fputs("rhs: ", to);
    cir_expr_dbg(calc->rhs, indent, to);
}

void cir_addrof_dbg(cir_addrof_t const *addrof, int indent, FILE *to) {
    indent++;
    fprintf(
        to,
        "addrof @ %s:%d:%d\n",
        addrof->common.pos.srcfile->name,
        addrof->common.pos.line + 1,
        addrof->common.pos.col + 1
    );

    pindent(indent, to);
    fputs("type: ", to);
    c_type_print(addrof->common.type, NULL, true, to);
    fputc('\n', to);

    pindent(indent, to);
    fputs(addrof->common.allow_addrof ? "addrof, " : "", to);
    fputs(addrof->common.is_lvalue ? "lvalue\n" : "rvalue\n", to);

    pindent(indent, to);
    fputs("expr: ", to);
    cir_expr_dbg(addrof->expr, indent, to);
}

void cir_deref_dbg(cir_deref_t const *deref, int indent, FILE *to) {
    indent++;
    fprintf(
        to,
        "deref @ %s:%d:%d\n",
        deref->common.pos.srcfile->name,
        deref->common.pos.line + 1,
        deref->common.pos.col + 1
    );

    pindent(indent, to);
    fputs("type: ", to);
    c_type_print(deref->common.type, NULL, true, to);
    fputc('\n', to);

    pindent(indent, to);
    fputs(deref->common.allow_addrof ? "addrof, " : "", to);
    fputs(deref->common.is_lvalue ? "lvalue\n" : "rvalue\n", to);

    pindent(indent, to);
    fputs("expr: ", to);
    cir_expr_dbg(deref->expr, indent, to);
}

void cir_tmpval_dbg(cir_tmpval_t const *tmpval, int indent, FILE *to) {
    indent++;
    fprintf(
        to,
        "tmpval %p @ %s:%d:%d\n",
        tmpval,
        tmpval->common.pos.srcfile->name,
        tmpval->common.pos.line + 1,
        tmpval->common.pos.col + 1
    );

    pindent(indent, to);
    fputs("type: ", to);
    c_type_print(tmpval->common.type, NULL, true, to);
    fputc('\n', to);

    pindent(indent, to);
    fputs(tmpval->common.allow_addrof ? "addrof, " : "", to);
    fputs(tmpval->common.is_lvalue ? "lvalue\n" : "rvalue\n", to);

    pindent(indent, to);
    fputs("inner: ", to);
    cir_expr_dbg(tmpval->inner, indent, to);
}

void cir_exprs_dbg(cir_exprs_t const *exprs, int indent, FILE *to) {
    indent++;
    fprintf(
        to,
        "exprs @ %s:%d:%d\n",
        exprs->common.pos.srcfile->name,
        exprs->common.pos.line + 1,
        exprs->common.pos.col + 1
    );

    pindent(indent, to);
    fputs("type: ", to);
    c_type_print(exprs->common.type, NULL, true, to);
    fputc('\n', to);

    pindent(indent, to);
    fputs(exprs->common.allow_addrof ? "addrof, " : "", to);
    fputs(exprs->common.is_lvalue ? "lvalue\n" : "rvalue\n", to);

    for (size_t i = 0; i < exprs->tmpvals.len; i++) {
        pindent(indent, to);
        fprintf(to, "tmpvals[%zu]: ", i);
        cir_tmpval_dbg(exprs->tmpvals.arr[i], indent, to);
    }

    for (size_t i = 0; i < exprs->exprs.len; i++) {
        pindent(indent, to);
        fprintf(to, "exprs[%zu]: ", i);
        cir_expr_dbg(exprs->exprs.arr[i], indent, to);
    }
}

void cir_assign_dbg(cir_assign_t const *assign, int indent, FILE *to) {
    indent++;
    fprintf(
        to,
        "assign @ %s:%d:%d\n",
        assign->common.pos.srcfile->name,
        assign->common.pos.line + 1,
        assign->common.pos.col + 1
    );

    pindent(indent, to);
    fputs("type: ", to);
    c_type_print(assign->common.type, NULL, true, to);
    fputc('\n', to);

    pindent(indent, to);
    fputs(assign->common.allow_addrof ? "addrof, " : "", to);
    fputs(assign->common.is_lvalue ? "lvalue\n" : "rvalue\n", to);

    pindent(indent, to);
    fputs("lhs: ", to);
    cir_expr_dbg(assign->lhs, indent, to);

    pindent(indent, to);
    fputs("rhs: ", to);
    cir_expr_dbg(assign->rhs, indent, to);
}

void cir_expr_dbg(cir_expr_t const *expr, int indent, FILE *to) {
    fputs("expr:", to);
    switch (expr->tag) {
        case CIR_EXPR_VALUE: cir_value_dbg(expr->value, indent, to); break;
        case CIR_EXPR_CALL: cir_call_dbg(expr->call, indent, to); break;
        case CIR_EXPR_CAST: cir_cast_dbg(expr->cast, indent, to); break;
        case CIR_EXPR_TERNARY: cir_ternary_dbg(expr->ternary, indent, to); break;
        case CIR_EXPR_CALC: cir_calc_dbg(expr->calc, indent, to); break;
        case CIR_EXPR_ADDROF: cir_addrof_dbg(expr->addrof, indent, to); break;
        case CIR_EXPR_DEREF: cir_deref_dbg(expr->deref, indent, to); break;
        case CIR_EXPR_EXPRS: cir_exprs_dbg(expr->exprs, indent, to); break;
        case CIR_EXPR_ASSIGN: cir_assign_dbg(expr->assign, indent, to); break;
        case CIR_EXPR_STMT: cir_stmt_dbg(expr->stmt, indent, to); break;
    }
}


void cir_stmts_dbg(cir_stmts_t const *stmts, int indent, FILE *to) {
    indent++;
    fprintf(to, "stmts @ %s:%d:%d\n", stmts->pos.srcfile->name, stmts->pos.line + 1, stmts->pos.col + 1);

    pindent(indent, to);
    fputs("scope: ", to);
    cir_scope_dbg(stmts->scope, indent, to);

    for (size_t i = 0; i < stmts->stmts.len; i++) {
        pindent(indent, to);
        fprintf(to, "stmts[%zu]: ", i);
        cir_stmt_dbg(stmts->stmts.arr[i], indent, to);
    }
}

void cir_for_dbg(cir_for_t const *cir_for, int indent, FILE *to) {
    indent++;
    fprintf(to, "for @ %s:%d:%d\n", cir_for->pos.srcfile->name, cir_for->pos.line + 1, cir_for->pos.col + 1);

    pindent(indent, to);
    fputs("scope: ", to);
    cir_scope_dbg(cir_for->scope, indent, to);

    if (cir_for->init) {
        pindent(indent, to);
        fputs("init: ", to);
        cir_stmt_dbg(cir_for->init, indent, to);
    }

    if (cir_for->cond) {
        pindent(indent, to);
        fputs("cond: ", to);
        cir_expr_dbg(cir_for->cond, indent, to);
    }

    if (cir_for->inc) {
        pindent(indent, to);
        fputs("inc: ", to);
        cir_expr_dbg(cir_for->inc, indent, to);
    }

    pindent(indent, to);
    fputs("body: ", to);
    cir_stmt_dbg(cir_for->body, indent, to);
}

void cir_while_dbg(cir_while_t const *cir_while, int indent, FILE *to) {
    indent++;
    fprintf(to, "while @ %s:%d:%d\n", cir_while->pos.srcfile->name, cir_while->pos.line + 1, cir_while->pos.col + 1);

    pindent(indent, to);
    fputs("scope: ", to);
    cir_scope_dbg(cir_while->scope, indent, to);

    pindent(indent, to);
    fputs("cond: ", to);
    cir_expr_dbg(cir_while->cond, indent, to);

    pindent(indent, to);
    fputs("body: ", to);
    cir_stmt_dbg(cir_while->body, indent, to);
}

void cir_if_dbg(cir_if_t const *cir_if, int indent, FILE *to) {
    indent++;
    fprintf(to, "if @ %s:%d:%d\n", cir_if->pos.srcfile->name, cir_if->pos.line + 1, cir_if->pos.col + 1);

    pindent(indent, to);
    fputs("cond: ", to);
    cir_expr_dbg(cir_if->cond, indent, to);

    pindent(indent, to);
    fputs("if_body: ", to);
    cir_stmt_dbg(cir_if->if_body, indent, to);

    if (cir_if->else_body) {
        pindent(indent, to);
        fputs("else_body: ", to);
        cir_stmt_dbg(cir_if->else_body, indent, to);
    }
}

void cir_label_dbg(cir_label_t const *label, int indent, FILE *to) {
    indent++;
    fprintf(to, "label @ %s:%d:%d\n", label->pos.srcfile->name, label->pos.line + 1, label->pos.col + 1);

    pindent(indent, to);
    fprintf(to, "name: \"%s\"\n", label->name);

    pindent(indent, to);
    fputs("body: ", to);
    cir_stmt_dbg(label->body, indent, to);
}

void cir_return_dbg(cir_return_t const *cir_return, int indent, FILE *to) {
    indent++;
    fprintf(
        to,
        "return @ %s:%d:%d\n",
        cir_return->pos.srcfile->name,
        cir_return->pos.line + 1,
        cir_return->pos.col + 1
    );

    pindent(indent, to);
    fputs("value: ", to);
    cir_expr_dbg(cir_return->value, indent, to);
}

void cir_stmt_dbg(cir_stmt_t const *stmt, int indent, FILE *to) {
    fputs("stmt:", to);
    switch (stmt->tag) {
        case CIR_STMT_STMTS: cir_stmts_dbg(stmt->stmts, indent, to); break;
        case CIR_STMT_FOR: cir_for_dbg(stmt->for_loop, indent, to); break;
        case CIR_STMT_WHILE: cir_while_dbg(stmt->while_loop, indent, to); break;
        case CIR_STMT_IF: cir_if_dbg(stmt->if_stmt, indent, to); break;
        case CIR_STMT_LABEL: cir_label_dbg(stmt->label, indent, to); break;
        case CIR_STMT_RETURN: cir_return_dbg(stmt->return_stmt, indent, to); break;
        case CIR_STMT_EXPR: cir_expr_dbg(stmt->expr, indent, to); break;
        case CIR_STMT_UNITS: cir_unit_list_dbg(stmt->units, indent, to); break;
    }
}


void cir_decl_dbg(cir_decl_t const *decl, int indent, FILE *to) {
    indent++;
    fprintf(to, "decl %p @ %s:%d:%d\n", decl, decl->pos.srcfile->name, decl->pos.line + 1, decl->pos.col + 1);

    pindent(indent, to);
    fprintf(to, "name: \"%s\"\n", decl->name);

    pindent(indent, to);
    fputs("type: ", to);
    c_type_print(decl->type, NULL, true, to);
    fputc('\n', to);

    if (decl->init) {
        pindent(indent, to);
        fputs("init: ", to);
        cir_expr_dbg(decl->init, indent, to);
    }
}

void cir_func_dbg(cir_func_t const *func, int indent, FILE *to) {
    indent++;
    fprintf(to, "func %p @ %s:%d:%d\n", func, func->pos.srcfile->name, func->pos.line + 1, func->pos.col + 1);

    pindent(indent, to);
    fprintf(to, "name: \"%s\"\n", func->name);

    pindent(indent, to);
    fputs("scope: ", to);
    cir_scope_dbg(func->scope, indent, to);

    pindent(indent, to);
    fputs("type: ", to);
    c_type_print(func->type, NULL, true, to);
    fputc('\n', to);

    for (size_t i = 0; i < func->body.len; i++) {
        pindent(indent, to);
        fprintf(to, "body[%zu]: ", i);
        cir_stmt_dbg(func->body.arr[i], indent, to);
    }
}

void cir_unit_dbg(cir_unit_t const *unit, int indent, FILE *to) {
    fputs("unit:", to);
    switch (unit->tag) {
        case CIR_UNIT_DECL: cir_decl_dbg(unit->decl, indent, to); break;
        case CIR_UNIT_FUNC: cir_func_dbg(unit->func, indent, to); break;
    }
}

void cir_unit_list_dbg(cir_unit_list_t const *unit_list, int indent, FILE *to) {
    indent++;
    fprintf(
        to,
        "unit_list @ %s:%d:%d\n",
        unit_list->pos.srcfile->name,
        unit_list->pos.line + 1,
        unit_list->pos.col + 1
    );

    for (size_t i = 0; i < unit_list->units.len; i++) {
        pindent(indent, to);
        fprintf(to, "units[%zu]: ", i);
        cir_unit_dbg(unit_list->units.arr[i], indent, to);
    }
}

void cir_trans_unit_dbg(cir_trans_unit_t const *trans_unit, int indent, FILE *to) {
    indent++;
    fprintf(to, "trans_unit\n");

    pindent(indent, to);
    fputs("scope: ", to);
    cir_scope_dbg(trans_unit->scope, indent, to);

    for (size_t i = 0; i < trans_unit->units.len; i++) {
        pindent(indent, to);
        fprintf(to, "units[%zu]: ", i);
        cir_unit_dbg(trans_unit->units.arr[i], indent, to);
    }
}
