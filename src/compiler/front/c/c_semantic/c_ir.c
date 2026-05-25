
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_ir.h"

#include "lilycc_malloc.h"
#include "refcount.h"
#include "vec.h"



cir_ident_t *cir_ident_create(pos_t pos, char *ident) {
    cir_ident_t *node = lilycc_malloc(sizeof(cir_ident_t));
    node->pos         = pos;
    node->ident       = ident;
    return node;
}

void cir_ident_delete(cir_ident_t *node) {
    lilycc_free(node->ident);
    lilycc_free(node);
}


cir_const_t *cir_const_create(pos_t pos, c_prim_t prim, ir_const_t iconst) {
    cir_const_t *node = lilycc_malloc(sizeof(cir_const_t));
    node->pos         = pos;
    node->prim        = prim;
    node->iconst      = iconst;
    return node;
}

void cir_const_delete(cir_const_t *node) {
    lilycc_free(node);
}


cir_comp_const_t *cir_comp_const_create(pos_t pos, rc_t type_rc, uint8_t *blob) {
    cir_comp_const_t *node = lilycc_malloc(sizeof(cir_comp_const_t));
    node->pos              = pos;
    node->type_rc          = type_rc;
    node->blob             = blob;
    return node;
}

void cir_comp_const_delete(cir_comp_const_t *node) {
    rc_delete(node->type_rc);
    lilycc_free(node->blob);
    lilycc_free(node);
}


cir_comp_value_t *cir_comp_value_create(pos_t pos, rc_t type_rc, vec_cir_comp_store_t stores) {
    cir_comp_value_t *node = lilycc_malloc(sizeof(cir_comp_value_t));
    node->pos              = pos;
    node->type_rc          = type_rc;
    node->stores           = stores;
    return node;
}

void cir_comp_value_delete(cir_comp_value_t *node) {
    rc_delete(node->type_rc);
    for (size_t i = 0; i < node->stores.len; i++) {
        cir_expr_delete(node->stores.arr[i].value);
    }
    vec_clear(&node->stores);
    lilycc_free(node);
}


cir_value_t *cir_value_create_ident(cir_ident_t *ident) {
    cir_value_t *node = lilycc_malloc(sizeof(cir_value_t));
    node->pos         = ident->pos;
    node->tag         = CIR_VALUE_IDENT;
    node->ident       = ident;
    return node;
}

cir_value_t *cir_value_create_const(cir_const_t *iconst) {
    cir_value_t *node = lilycc_malloc(sizeof(cir_value_t));
    node->pos         = iconst->pos;
    node->tag         = CIR_VALUE_CONST;
    node->iconst      = iconst;
    return node;
}

cir_value_t *cir_value_create_comp_const(cir_comp_const_t *comp_const) {
    cir_value_t *node = lilycc_malloc(sizeof(cir_value_t));
    node->pos         = comp_const->pos;
    node->tag         = CIR_VALUE_COMP_CONST;
    node->comp_const  = comp_const;
    return node;
}

cir_value_t *cir_value_create_comp_value(cir_comp_value_t *comp_value) {
    cir_value_t *node = lilycc_malloc(sizeof(cir_value_t));
    node->pos         = comp_value->pos;
    node->tag         = CIR_VALUE_COMP_VALUE;
    node->comp_value  = comp_value;
    return node;
}

void cir_value_delete(cir_value_t *node) {
    switch (node->tag) {
        case CIR_VALUE_IDENT: cir_ident_delete(node->ident); break;
        case CIR_VALUE_CONST: cir_const_delete(node->iconst); break;
        case CIR_VALUE_COMP_CONST: cir_comp_const_delete(node->comp_const); break;
        case CIR_VALUE_COMP_VALUE: cir_comp_value_delete(node->comp_value); break;
    }
    lilycc_free(node);
}


cir_call_t *cir_call_create(pos_t pos, cir_expr_t *func, vec_cir_expr_t args) {
    cir_call_t *node = lilycc_malloc(sizeof(cir_call_t));
    node->pos        = pos;
    node->func       = func;
    node->args       = args;
    return node;
}

void cir_call_delete(cir_call_t *node) {
    cir_expr_delete(node->func);
    for (size_t i = 0; i < node->args.len; i++) {
        cir_expr_delete(node->args.arr[i]);
    }
    vec_clear(&node->args);
    lilycc_free(node);
}


cir_cast_t *cir_cast_create(pos_t pos, rc_t type_rc, cir_expr_t *value) {
    cir_cast_t *node = lilycc_malloc(sizeof(cir_cast_t));
    node->pos        = pos;
    node->type_rc    = type_rc;
    node->value      = value;
    return node;
}

void cir_cast_delete(cir_cast_t *node) {
    rc_delete(node->type_rc);
    cir_expr_delete(node->value);
    lilycc_free(node);
}


cir_ternary_t *cir_ternary_create(pos_t pos, cir_expr_t *cond, cir_expr_t *if_expr, cir_expr_t *else_expr) {
    cir_ternary_t *node = lilycc_malloc(sizeof(cir_ternary_t));
    node->pos           = pos;
    node->cond          = cond;
    node->if_expr       = if_expr;
    node->else_expr     = else_expr;
    return node;
}

void cir_ternary_delete(cir_ternary_t *node) {
    cir_expr_delete(node->cond);
    cir_expr_delete(node->if_expr);
    cir_expr_delete(node->else_expr);
    lilycc_free(node);
}


cir_calc_t *cir_calc_create(pos_t pos, cir_calc_op_t op, cir_expr_t *lhs, cir_expr_t *rhs) {
    cir_calc_t *node = lilycc_malloc(sizeof(cir_calc_t));
    node->pos        = pos;
    node->op         = op;
    node->lhs        = lhs;
    node->rhs        = rhs;
    return node;
}

void cir_calc_delete(cir_calc_t *node) {
    cir_expr_delete(node->lhs);
    cir_expr_delete(node->rhs);
    lilycc_free(node);
}


cir_inc_t *cir_inc_create(pos_t pos, cir_expr_t *expr, bool is_pre, bool is_dec) {
    cir_inc_t *node = lilycc_malloc(sizeof(cir_inc_t));
    node->pos       = pos;
    node->expr      = expr;
    node->is_pre    = is_pre;
    node->is_dec    = is_dec;
    return node;
}

void cir_inc_delete(cir_inc_t *node) {
    cir_expr_delete(node->expr);
    lilycc_free(node);
}


cir_addrof_t *cir_addrof_create(pos_t pos, cir_expr_t *expr) {
    cir_addrof_t *node = lilycc_malloc(sizeof(cir_addrof_t));
    node->pos          = pos;
    node->expr         = expr;
    return node;
}

void cir_addrof_delete(cir_addrof_t *node) {
    cir_expr_delete(node->expr);
    lilycc_free(node);
}


cir_deref_t *cir_deref_create(pos_t pos, cir_expr_t *expr) {
    cir_deref_t *node = lilycc_malloc(sizeof(cir_deref_t));
    node->pos         = pos;
    node->expr        = expr;
    return node;
}

void cir_deref_delete(cir_deref_t *node) {
    cir_expr_delete(node->expr);
    lilycc_free(node);
}


cir_expr_t *cir_expr_create_value(cir_value_t *value) {
    cir_expr_t *node = lilycc_malloc(sizeof(cir_expr_t));
    node->pos        = value->pos;
    node->tag        = CIR_EXPR_VALUE;
    node->value      = value;
    return node;
}

cir_expr_t *cir_expr_create_call(cir_call_t *call) {
    cir_expr_t *node = lilycc_malloc(sizeof(cir_expr_t));
    node->pos        = call->pos;
    node->tag        = CIR_EXPR_CALL;
    node->call       = call;
    return node;
}

cir_expr_t *cir_expr_create_cast(cir_cast_t *cast) {
    cir_expr_t *node = lilycc_malloc(sizeof(cir_expr_t));
    node->pos        = cast->pos;
    node->tag        = CIR_EXPR_CAST;
    node->cast       = cast;
    return node;
}

cir_expr_t *cir_expr_create_ternary(cir_ternary_t *ternary) {
    cir_expr_t *node = lilycc_malloc(sizeof(cir_expr_t));
    node->pos        = ternary->pos;
    node->tag        = CIR_EXPR_TERNARY;
    node->ternary    = ternary;
    return node;
}

cir_expr_t *cir_expr_create_calc(cir_calc_t *calc) {
    cir_expr_t *node = lilycc_malloc(sizeof(cir_expr_t));
    node->pos        = calc->pos;
    node->tag        = CIR_EXPR_CALC;
    node->calc       = calc;
    return node;
}

cir_expr_t *cir_expr_create_inc(cir_inc_t *inc) {
    cir_expr_t *node = lilycc_malloc(sizeof(cir_expr_t));
    node->pos        = inc->pos;
    node->tag        = CIR_EXPR_INC;
    node->inc        = inc;
    return node;
}

cir_expr_t *cir_expr_create_addrof(cir_addrof_t *addrof) {
    cir_expr_t *node = lilycc_malloc(sizeof(cir_expr_t));
    node->pos        = addrof->pos;
    node->tag        = CIR_EXPR_ADDROF;
    node->addrof     = addrof;
    return node;
}

cir_expr_t *cir_expr_create_deref(cir_deref_t *deref) {
    cir_expr_t *node = lilycc_malloc(sizeof(cir_expr_t));
    node->pos        = deref->pos;
    node->tag        = CIR_EXPR_DEREF;
    node->deref      = deref;
    return node;
}

void cir_expr_delete(cir_expr_t *node) {
    switch (node->tag) {
        case CIR_EXPR_VALUE: cir_value_delete(node->value); break;
        case CIR_EXPR_CALL: cir_call_delete(node->call); break;
        case CIR_EXPR_CAST: cir_cast_delete(node->cast); break;
        case CIR_EXPR_TERNARY: cir_ternary_delete(node->ternary); break;
        case CIR_EXPR_CALC: cir_calc_delete(node->calc); break;
        case CIR_EXPR_INC: cir_inc_delete(node->inc); break;
        case CIR_EXPR_ADDROF: cir_addrof_delete(node->addrof); break;
        case CIR_EXPR_DEREF: cir_deref_delete(node->deref); break;
    }
    lilycc_free(node);
}


cir_for_t *cir_for_create(pos_t pos, cir_stmt_t *init, cir_expr_t *cond, cir_expr_t *inc, cir_stmt_t *body) {
    cir_for_t *node = lilycc_malloc(sizeof(cir_for_t));
    node->pos       = pos;
    node->init      = init;
    node->cond      = cond;
    node->inc       = inc;
    node->body      = body;
    return node;
}

void cir_for_delete(cir_for_t *node) {
    if (node->init) {
        cir_stmt_delete(node->init);
    }
    if (node->cond) {
        cir_expr_delete(node->cond);
    }
    if (node->inc) {
        cir_expr_delete(node->inc);
    }
    cir_stmt_delete(node->body);
    lilycc_free(node);
}


cir_while_t *cir_while_create(pos_t pos, cir_expr_t *cond, cir_stmt_t *body, bool is_do_while) {
    cir_while_t *node = lilycc_malloc(sizeof(cir_while_t));
    node->pos         = pos;
    node->cond        = cond;
    node->body        = body;
    node->is_do_while = is_do_while;
    return node;
}

void cir_while_delete(cir_while_t *node) {
    cir_expr_delete(node->cond);
    cir_stmt_delete(node->body);
    lilycc_free(node);
}


cir_stmts_t *cir_stmts_create(pos_t pos, vec_cir_stmt_t stmts) {
    cir_stmts_t *node = lilycc_malloc(sizeof(cir_stmts_t));
    node->pos         = pos;
    node->stmts       = stmts;
    return node;
}

void cir_stmts_delete(cir_stmts_t *node) {
    for (size_t i = 0; i < node->stmts.len; i++) {
        cir_stmt_delete(node->stmts.arr[i]);
    }
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


cir_stmt_expr_t *cir_stmt_expr_create(pos_t pos, cir_expr_t *expr) {
    cir_stmt_expr_t *node = lilycc_malloc(sizeof(cir_stmt_expr_t));
    node->pos             = pos;
    node->expr            = expr;
    return node;
}

void cir_stmt_expr_delete(cir_stmt_expr_t *node) {
    cir_expr_delete(node->expr);
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

cir_stmt_t *cir_stmt_create_expr(cir_stmt_expr_t *expr_stmt) {
    cir_stmt_t *node = lilycc_malloc(sizeof(cir_stmt_t));
    node->pos        = expr_stmt->pos;
    node->tag        = CIR_STMT_EXPR;
    node->expr_stmt  = expr_stmt;
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
        case CIR_STMT_EXPR: cir_stmt_expr_delete(node->expr_stmt); break;
    }
    lilycc_free(node);
}


cir_decl_t *cir_decl_create(pos_t pos, rc_t type_rc, char *name, cir_expr_t *init) {
    cir_decl_t *node = lilycc_malloc(sizeof(cir_decl_t));
    node->pos        = pos;
    node->type_rc    = type_rc;
    node->name       = name;
    node->init       = init;
    return node;
}

void cir_decl_delete(cir_decl_t *node) {
    rc_delete(node->type_rc);
    lilycc_free(node->name);
    if (node->init) {
        cir_expr_delete(node->init);
    }
    lilycc_free(node);
}


cir_func_t *cir_func_create(pos_t pos, rc_t type_rc, char *name, vec_cstr_t param_names, cir_stmt_t *body) {
    cir_func_t *node  = lilycc_malloc(sizeof(cir_func_t));
    node->pos         = pos;
    node->type_rc     = type_rc;
    node->name        = name;
    node->param_names = param_names;
    node->body        = body;
    return node;
}

void cir_func_delete(cir_func_t *node) {
    rc_delete(node->type_rc);
    lilycc_free(node->name);
    for (size_t i = 0; i < node->param_names.len; i++) {
        lilycc_free(node->param_names.arr[i]);
    }
    vec_clear(&node->param_names);
    if (node->body) {
        cir_stmt_delete(node->body);
    }
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


cir_trans_unit_t *cir_trans_unit_create(pos_t pos, vec_cir_unit_t units) {
    cir_trans_unit_t *node = lilycc_malloc(sizeof(cir_trans_unit_t));
    node->pos              = pos;
    node->units            = units;
    return node;
}

void cir_trans_unit_delete(cir_trans_unit_t *node) {
    for (size_t i = 0; i < node->units.len; i++) {
        cir_unit_delete(node->units.arr[i]);
    }
    vec_clear(&node->units);
    lilycc_free(node);
}
