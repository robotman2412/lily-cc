
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_compile_expr.h"

#include "c_ast.h"
#include "c_compile.h"
#include "c_ir.h"
#include "c_prim.h"
#include "c_tokenizer.h"
#include "c_types.h"
#include "c_types1.h"
#include "compiler.h"
#include "ir_interpreter.h"
#include "ir_types.h"
#include "lilycc_malloc.h"
#include "vec.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



// Whether a primitive is an arithmetic type (integer or floating point).
static bool c_prim_is_arith(c_prim_t prim) {
    return prim < C_N_PRIM && prim != C_PRIM_VOID;
}

// Determine the common result type of a ternary expression's two value operands.
// Integer promotion follows the infix rules; arrays first decay into pointers.
// For two pointers: if exactly one points to void, the other's type is used; if their inner
// types are incompatible, `void *` is used; otherwise the (compatible) left pointer type is kept.
// Takes shares of the operand type rcs (not consumed); returns a new owned type rc, or NULL on
// incompatible operands (after emitting a diagnostic).
static c_type_opt_t c_compile2_ternary_result_type(c_compiler_t *cc, pos_t pos, c_type_ref_t lref, c_type_ref_t rref) {
    c_type_t ltyp = c_type_clone_array_decay(lref);
    c_type_t rtyp = c_type_clone_array_decay(rref);

    // Both arithmetic: apply the usual arithmetic conversions.
    if (c_prim_is_arith(ltyp.prim) && c_prim_is_arith(rtyp.prim)) {
        c_prim_t prim = c_prim_promote(cc, ltyp.prim, rtyp.prim);
        c_type_delete(ltyp);
        c_type_delete(rtyp);
        return C_TYPE_FROM_PRIM(prim);
    }

    // Both pointers: apply the pointer compatibility rules.
    if (ltyp.prim == C_COMP_POINTER && rtyp.prim == C_COMP_POINTER) {
        bool lvoid = ltyp.extra->inner.prim == C_PRIM_VOID;
        bool rvoid = rtyp.extra->inner.prim == C_PRIM_VOID;
        if (lvoid != rvoid) {
            // Exactly one points to void: use the other (non-void) pointer's type.
            c_type_delete(lvoid ? ltyp : rtyp);
            return lvoid ? rtyp : ltyp;
        }
        if (c_type_is_compatible(ltyp.extra->inner, rtyp.extra->inner)) {
            // Compatible inner types: keep the left pointer type.
            c_type_delete(rtyp);
            return ltyp;
        }
        // Incompatible inner types: fall back to `void *`.
        c_type_delete(ltyp);
        c_type_delete(rtyp);
        return c_type_clone_pointer(C_TYPE_FROM_PRIM(C_PRIM_VOID));
    }

    // Identical struct/union/void operands: that type is the result.
    if (c_type_is_identical(ltyp, rtyp, false)) {
        c_type_delete(rtyp);
        return ltyp;
    }

    cctx_diagnostic(cc->cctx, pos, DIAG_ERR, "Incompatible operand types in ternary expression");
    c_type_delete(ltyp);
    c_type_delete(rtyp);
    return C_TYPE_INVALID;
}


// Perform or const-propagate a cast.
// Transfers ownership of `type`.
static cir_expr_t *raw_cast(c_compiler_t *cc, pos_t pos, c_type_t type, cir_expr_t *val) {
    if (c_type_is_identical(type, val->common.type, true)) {
        return val;
    } else if (!c_type_is_castable(type, val->common.type)) {
        cctx_diagnostic(cc->cctx, pos, DIAG_ERR, "Invalid cast");
        return NULL;
    }

    if (val->tag == CIR_EXPR_VALUE && val->value->tag == CIR_VALUE_CONST) {
        ir_prim_t   dest_prim = c_type_to_ir_type(cc, type);
        ir_const_t  iconst    = ir_cast(dest_prim, val->value->iconst->iconst);
        cir_expr_t *res       = cir_expr_create_value(cir_value_create_const(
            (cir_expr_common_t){
                .pos          = pos,
                .type         = type,
                .is_lvalue    = false,
                .allow_addrof = false,
            },
            cir_const_create(pos, type.prim, iconst)
        ));
        cir_expr_delete(val);
        return res;
    }

    return cir_expr_create_cast(cir_cast_create(
        (cir_expr_common_t){
            .pos          = pos,
            .type         = type,
            .is_lvalue    = false,
            .allow_addrof = false,
        },
        val
    ));
}

// Perform or const-propagate a raw calculation.
static cir_expr_t *raw_calc(c_compiler_t *cc, pos_t pos, cir_calc_op_t op, cir_expr_t *lhs, cir_expr_t *rhs) {
    c_type_ref_t ltyp = lhs->common.type;
    c_type_ref_t rtyp = rhs->common.type;

    if (!c_type_is_compatible(ltyp, rtyp)) {
        cctx_diagnostic(cc->cctx, pos, DIAG_ERR, "Incompatible types for expression");
        return NULL;
    }

    if ((lhs->tag == CIR_EXPR_VALUE && lhs->value->tag == CIR_VALUE_CONST)
        && (rhs->tag == CIR_EXPR_VALUE && rhs->value->tag == CIR_VALUE_CONST)) {
        c_type_t   res_type;
        ir_const_t iconst;
        if (op == CIR_CALC_LAND) {
            bool lhs_b       = ir_calc1(IR_OP1_snez, lhs->value->iconst->iconst).constl;
            bool rhs_b       = ir_calc1(IR_OP1_snez, lhs->value->iconst->iconst).constl;
            iconst.prim_type = c_prim_to_ir_type(cc, C_PRIM_SINT);
            iconst.constl    = lhs_b & rhs_b;
            iconst.consth    = 0;
            res_type         = C_TYPE_FROM_PRIM(C_PRIM_SINT);
        } else if (op == CIR_CALC_LOR) {
            bool lhs_b       = ir_calc1(IR_OP1_snez, lhs->value->iconst->iconst).constl;
            bool rhs_b       = ir_calc1(IR_OP1_snez, lhs->value->iconst->iconst).constl;
            iconst.prim_type = c_prim_to_ir_type(cc, C_PRIM_SINT);
            iconst.constl    = lhs_b | rhs_b;
            iconst.consth    = 0;
            res_type         = C_TYPE_FROM_PRIM(C_PRIM_SINT);
        } else {
            ir_op2_type_t ir_op = cir_calc_op_to_ir_op2(op);
            iconst              = ir_calc2(ir_op, lhs->value->iconst->iconst, rhs->value->iconst->iconst);
            res_type            = c_type_clone(lhs->common.type);
        }

        cir_expr_t *res = cir_expr_create_value(cir_value_create_const(
            (cir_expr_common_t){
                .pos          = pos,
                .type         = res_type,
                .is_lvalue    = false,
                .allow_addrof = false,
            },
            cir_const_create(pos, res_type.prim, iconst)
        ));
        cir_expr_delete(lhs);
        cir_expr_delete(rhs);
        return res;
    }

    return cir_expr_create_calc(cir_calc_create(
        (cir_expr_common_t){
            .pos          = pos,
            .type         = c_type_clone(lhs->common.type),
            .allow_addrof = false,
            .is_lvalue    = false,
        },
        op,
        lhs,
        rhs
    ));
}

// Perform address-of operation.
static cir_expr_t *raw_addrof(c_compiler_t *cc, cir_expr_t *val) {
    if (!val->common.allow_addrof) {
        cctx_diagnostic(cc->cctx, val->common.pos, DIAG_ERR, "Cannot take the address of this value");
        cir_expr_delete(val);
        return NULL;
    }

    abort();
}

// Do array decay if needed.
static cir_expr_t *array_decay(c_compiler_t *cc, cir_expr_t *val) {
    c_type_ref_t type = val->common.type;

    if (type.prim != C_COMP_ARRAY) {
        return val;
    }

    cir_expr_t *tmp = raw_addrof(cc, val);
    if (!tmp) {
        return NULL;
    }
    c_type_t ptr_rc = c_type_clone_pointer(type.extra->inner);
    return raw_cast(cc, val->common.pos, ptr_rc, tmp);
}

// Emit a calculation operation as C IR.
// Handles pointer arithmetic, promotion, etc.
static cir_expr_t *
    expand_calc(c_compiler_t *cc, pos_t pos, cir_calc_op_t op, cir_expr_t *lhs, cir_expr_t *rhs, bool is_assign) {
    lhs = array_decay(cc, lhs);
    rhs = array_decay(cc, rhs);
    if (!lhs || !rhs) {
        goto error;
    }

    c_type_ref_t ltyp = lhs->common.type;
    c_type_ref_t rtyp = rhs->common.type;

    if (ltyp.prim == C_PRIM_VOID || (ltyp.prim >= C_N_PRIM && ltyp.prim != C_COMP_POINTER)) {
        cctx_diagnostic(cc->cctx, lhs->common.pos, DIAG_ERR, "Expected arithmetic or complete pointer type");
        goto error;
    }
    if (rtyp.prim == C_PRIM_VOID || (rtyp.prim >= C_N_PRIM && rtyp.prim != C_COMP_POINTER)) {
        cctx_diagnostic(cc->cctx, rhs->common.pos, DIAG_ERR, "Expected arithmetic or complete pointer type");
        goto error;
    }

    // Pointer difference.
    if (!is_assign && op == CIR_CALC_SUB && ltyp.prim == C_COMP_POINTER && rtyp.prim == C_COMP_POINTER) {
        if (!c_type_is_compatible(ltyp.extra->inner, rtyp.extra->inner)) {
            cctx_diagnostic(cc->cctx, pos, DIAG_ERR, "Incompatible pointer types");
            goto error;
        }
        uint64_t size, align;
        if (!c_type_get_size(cc, ltyp.extra->inner, &size, &align)) {
            cctx_diagnostic(cc->cctx, pos, DIAG_ERR, "Use of incomplete pointer type");
            goto error;
        }

        c_prim_t ptrdiff_prim = cc->options.size_type - 1;

        cir_expr_t *sub = raw_calc(cc, pos, CIR_CALC_SUB, lhs, rhs);
        if (!sub) {
            return NULL;
        }
        cir_expr_t *cast = raw_cast(cc, pos, C_TYPE_FROM_PRIM(ptrdiff_prim), sub);
        assert(cast != NULL);
        return raw_calc(cc, pos, CIR_CALC_DIV, sub, c_compile2_synth_iconst(cc, pos, ptrdiff_prim, ui128(size)));
    }

    // Pointer offset.
    if (
        (op == CIR_CALC_ADD // Add to pointer
         && ((ltyp.prim == C_COMP_POINTER && rtyp.prim != C_COMP_POINTER)
             || (!is_assign && rtyp.prim == C_COMP_POINTER // Pointer is lhs.
                 && ltyp.prim != C_COMP_POINTER)))         // Pointer is rhs (only for non-assignments).
        || (op == CIR_CALC_SUB && ltyp.prim == C_COMP_POINTER && rtyp.prim == C_COMP_POINTER) // Subtract from pointer.
    ) {
        if (rtyp.prim == C_COMP_POINTER) {
            cir_expr_t *tmp = lhs;
            lhs             = rhs;
            rhs             = tmp;
        }

        c_type_ref_t ltyp = lhs->common.type;

        uint64_t size, align;
        if (!c_type_get_size(cc, ltyp.extra->inner, &size, &align)) {
            cctx_diagnostic(cc->cctx, pos, DIAG_ERR, "Use of incomplete pointer type");
            goto error;
        }

        rhs = raw_cast(cc, pos, C_TYPE_FROM_PRIM(cc->options.size_type), rhs);
        assert(rhs != NULL);
        rhs = raw_calc(cc, pos, op, rhs, c_compile2_synth_iconst(cc, pos, cc->options.size_type, ui128(size)));
        assert(rhs != NULL);
        rhs = raw_cast(cc, pos, c_type_clone(ltyp), rhs);
        assert(rhs != NULL);
        return raw_calc(cc, pos, op, lhs, rhs);
    }

    if (ltyp.prim == C_PRIM_VOID || ltyp.prim >= C_N_PRIM) {
        cctx_diagnostic(cc->cctx, lhs->common.pos, DIAG_ERR, "Expected arithmetic type");
        goto error;
    }
    if (rtyp.prim == C_PRIM_VOID || rtyp.prim >= C_N_PRIM) {
        cctx_diagnostic(cc->cctx, rhs->common.pos, DIAG_ERR, "Expected arithmetic type");
        goto error;
    }

    // The remaining are regular arithmetic.
    return raw_calc(cc, pos, op, lhs, rhs);

error:
    if (lhs) {
        cir_expr_delete(lhs);
    }
    if (rhs) {
        cir_expr_delete(rhs);
    }
    return NULL;
}



// Compile an expression.
// Returns `NULL` on semantic errors.
cir_expr_t *c_compile2_expr(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_t const *expr) {
    switch (expr->tag) {
        case C_AST_TAG_EXPR_TERNARY: return c_compile2_expr_ternary(cc, scope, expr->expr_ternary);
        case C_AST_TAG_EXPR_INDEX: return c_compile2_expr_index(cc, scope, expr->expr_index);
        case C_AST_TAG_EXPR_INFIX: return c_compile2_expr_infix(cc, scope, expr->expr_infix);
        case C_AST_TAG_EXPR_PREFIX: return c_compile2_expr_prefix(cc, scope, expr->expr_prefix);
        case C_AST_TAG_EXPR_SUFFIX: return c_compile2_expr_suffix(cc, scope, expr->expr_suffix);
        case C_AST_TAG_EXPR_CAST: return c_compile2_expr_cast(cc, scope, expr->expr_cast);
        case C_AST_TAG_EXPR_CALL: return c_compile2_expr_call(cc, scope, expr->expr_call);
        case C_AST_TAG_EXPR_IDENT: return c_compile2_expr_ident(cc, scope, expr->expr_ident);
        case C_AST_TAG_EXPR_ICONST: return c_compile2_expr_iconst(cc, scope, expr->expr_iconst);
        case C_AST_TAG_EXPR_SCONST: return c_compile2_expr_sconst(cc, scope, expr->expr_sconst);
        case C_AST_TAG_EXPR_COMPLITERAL: return c_compile2_expr_compliteral(cc, scope, expr->expr_compliteral);
        case C_AST_TAG_EXPR_GARBAGE: return NULL;
        case C_AST_TAG_EXPRS: return c_compile2_expr_exprs(cc, scope, expr->expr_exprs);
        default: abort();
    }
}


// Compile a ternary expression.
cir_expr_t *c_compile2_expr_ternary(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_ternary_t const *expr) {
    cir_expr_t *cond      = c_compile2_expr(cc, scope, expr->cond);
    cir_expr_t *if_expr   = expr->if_expr ? c_compile2_expr(cc, scope, expr->if_expr) : NULL;
    cir_expr_t *else_expr = c_compile2_expr(cc, scope, expr->else_expr);
    if (!cond || (!if_expr && expr->if_expr) || !else_expr) {
        goto error;
    }

    c_type_opt_t type
        = c_compile2_ternary_result_type(cc, expr->pos, (if_expr ?: cond)->common.type, else_expr->common.type);
    if (!c_type_is_valid(type)) {
        goto error;
    }

    cir_expr_common_t common = {
        .type         = type,
        .pos          = expr->pos,
        .is_lvalue    = false,
        .allow_addrof = false,
    };
    return cir_expr_create_ternary(cir_ternary_create(common, cond, if_expr, else_expr));

error:
    if (cond) {
        cir_expr_delete(cond);
    }
    if (if_expr) {
        cir_expr_delete(if_expr);
    }
    if (else_expr) {
        cir_expr_delete(else_expr);
    }
    return NULL;
}

// Compile an index expression.
cir_expr_t *c_compile2_expr_index(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_index_t const *expr) {
    cir_expr_t *lhs = c_compile2_expr(cc, scope, expr->lhs);
    cir_expr_t *rhs = c_compile2_expr(cc, scope, expr->rhs);
    if (!lhs || !rhs) {
        goto error;
    }

    // TODO: Const prop.

    cir_expr_t *add = expand_calc(cc, expr->pos, CIR_CALC_ADD, lhs, rhs, false);
    if (!add) {
        return NULL;
    }

    cir_expr_common_t common = {
        .pos          = expr->pos,
        .type         = c_type_clone(add->common.type.extra->inner),
        .allow_addrof = true,
        .is_lvalue    = true,
    };
    return cir_expr_create_deref(cir_deref_create(common, add));

error:
    if (lhs) {
        cir_expr_delete(lhs);
    }
    if (rhs) {
        cir_expr_delete(rhs);
    }
    return NULL;
}

// Map an infix operator token to its `cir_calc_op_t`.
// Works for both plain operators (e.g. `+`) and the calc part of compound assignments (e.g. `+=`).
static bool tkn_to_calc_op(c_tokentype_t oper, cir_calc_op_t *out) {
    switch (oper) {
        case C_TKN_ADD:
        case C_TKN_ADD_S: *out = CIR_CALC_ADD; return true;
        case C_TKN_SUB:
        case C_TKN_SUB_S: *out = CIR_CALC_SUB; return true;
        case C_TKN_MUL:
        case C_TKN_MUL_S: *out = CIR_CALC_MUL; return true;
        case C_TKN_DIV:
        case C_TKN_DIV_S: *out = CIR_CALC_DIV; return true;
        case C_TKN_MOD:
        case C_TKN_MOD_S: *out = CIR_CALC_MOD; return true;
        case C_TKN_SHL:
        case C_TKN_SHL_S: *out = CIR_CALC_SHL; return true;
        case C_TKN_SHR:
        case C_TKN_SHR_S: *out = CIR_CALC_SHR; return true;
        case C_TKN_AND:
        case C_TKN_AND_S: *out = CIR_CALC_BAND; return true;
        case C_TKN_OR:
        case C_TKN_OR_S: *out = CIR_CALC_BOR; return true;
        case C_TKN_XOR:
        case C_TKN_XOR_S: *out = CIR_CALC_BXOR; return true;
        case C_TKN_LAND: *out = CIR_CALC_LAND; return true;
        case C_TKN_LOR: *out = CIR_CALC_LOR; return true;
        case C_TKN_EQ: *out = CIR_CALC_EQ; return true;
        case C_TKN_NE: *out = CIR_CALC_NE; return true;
        case C_TKN_LT: *out = CIR_CALC_LT; return true;
        case C_TKN_LE: *out = CIR_CALC_LE; return true;
        case C_TKN_GT: *out = CIR_CALC_GT; return true;
        case C_TKN_GE: *out = CIR_CALC_GE; return true;
        default: return false;
    }
}

// Whether `oper` is one of the compound-assignment tokens (`+=`, `-=`, ...).
static bool is_compound_assign(c_tokentype_t oper) {
    return oper >= C_TKN_ADD_S && oper <= C_TKN_XOR_S;
}

// Compile a struct/union member access (`.` or `->`).
static cir_expr_t *
    c_compile2_expr_member(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_infix_t const *expr, bool is_arrow) {
    // RHS must be a bare identifier denoting the member name.
    if (expr->rhs->tag != C_AST_TAG_EXPR_IDENT) {
        cctx_diagnostic(cc->cctx, expr->rhs->pos, DIAG_ERR, "Expected member name");
        return NULL;
    }
    char const *name = expr->rhs->expr_ident->name;

    cir_expr_t *lhs = c_compile2_expr(cc, scope, expr->lhs);
    if (!lhs) {
        return NULL;
    }

    // For `->`, the LHS is a pointer; for `.`, take its address.
    cir_expr_t *ptr_expr;
    c_type_t    struct_type; // Reference.
    if (is_arrow) {
        c_type_ref_t type = lhs->common.type;
        if (type.prim != C_COMP_POINTER) {
            cctx_diagnostic(cc->cctx, expr->oper_pos, DIAG_ERR, "Left operand of -> must be a pointer");
            cir_expr_delete(lhs);
            return NULL;
        }
        struct_type = type.extra->inner;
        ptr_expr    = lhs;
    } else {
        c_type_ref_t type = lhs->common.type;
        if (type.prim != C_COMP_STRUCT && type.prim != C_COMP_UNION) {
            cctx_diagnostic(cc->cctx, expr->oper_pos, DIAG_ERR, "Left operand of . must be a struct or union");
            cir_expr_delete(lhs);
            return NULL;
        }
        if (!lhs->common.allow_addrof) {
            cctx_diagnostic(cc->cctx, expr->oper_pos, DIAG_ERR, "Cannot access member of non-addressable value");
            cir_expr_delete(lhs);
            return NULL;
        }
        struct_type     = type;
        c_type_t ptr_rc = c_type_clone_pointer(lhs->common.type);
        ptr_expr        = cir_expr_create_addrof(cir_addrof_create(
            (cir_expr_common_t){
                .pos          = expr->pos,
                .is_lvalue    = false,
                .allow_addrof = false,
                .type         = ptr_rc,
            },
            lhs
        ));
    }

    if (struct_type.prim != C_COMP_STRUCT && struct_type.prim != C_COMP_UNION) {
        cctx_diagnostic(cc->cctx, expr->oper_pos, DIAG_ERR, "Member access on non-struct/union type");
        cir_expr_delete(ptr_expr);
        return NULL;
    }

    c_field_info_t field = c_type_get_field(cc, struct_type, name);
    if (!c_type_is_valid(field.type)) {
        cctx_diagnostic(cc->cctx, expr->rhs->pos, DIAG_ERR, "No member named '%s'", name);
        cir_expr_delete(ptr_expr);
        return NULL;
    }

    // TODO: Const-prop field read.

    // Build a pointer-to-field type, then `ptr + offset` as that type, then deref.
    c_type_t    field_ptr_rc = c_type_clone_pointer(field.type);
    cir_expr_t *off_iconst   = c_compile2_synth_iconst(cc, expr->oper_pos, cc->options.size_type, ui128(field.offset));
    cir_expr_t *add          = cir_expr_create_calc(cir_calc_create(
        (cir_expr_common_t){
            .pos          = expr->pos,
            .is_lvalue    = false,
            .allow_addrof = false,
            .type         = field_ptr_rc,
        },
        CIR_CALC_ADD,
        ptr_expr,
        off_iconst
    ));
    return cir_expr_create_deref(cir_deref_create(
        (cir_expr_common_t){
            .pos          = expr->pos,
            .is_lvalue    = true,
            .allow_addrof = true,
            .type         = c_type_clone(field.type),
        },
        add
    ));
}

// Compile an infix expression.
cir_expr_t *c_compile2_expr_infix(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_infix_t const *expr) {
    // Member access has a special RHS (a bare identifier), handle it before evaluating either side normally.
    if (expr->oper == C_TKN_DOT) {
        return c_compile2_expr_member(cc, scope, expr, false);
    }
    if (expr->oper == C_TKN_ARROW) {
        return c_compile2_expr_member(cc, scope, expr, true);
    }

    cir_expr_t *lhs = c_compile2_expr(cc, scope, expr->lhs);
    cir_expr_t *rhs = c_compile2_expr(cc, scope, expr->rhs);
    if (!lhs || !rhs) {
        goto error;
    }


    c_type_ref_t ltyp = lhs->common.type;

    // Plain assignment: type-check, then emit an assign node.
    if (expr->oper == C_TKN_ASSIGN) {
        if (!lhs->common.is_lvalue || ltyp.qual.q_const) {
            cctx_diagnostic(cc->cctx, expr->oper_pos, DIAG_ERR, "Left operand of = is not a modifiable lvalue");
            goto error;
        }
        if (!c_type_is_castable(ltyp, rhs->common.type)) {
            cctx_diagnostic(cc->cctx, expr->oper_pos, DIAG_ERR, "Incompatible types in assignment");
            goto error;
        }
        // Insert an implicit cast if the RHS type differs.
        if (!c_type_is_identical(ltyp, rhs->common.type, false)) {
            rhs = cir_expr_create_cast(cir_cast_create(
                (cir_expr_common_t){
                    .pos          = rhs->common.pos,
                    .is_lvalue    = false,
                    .allow_addrof = false,
                    .type         = c_type_clone(ltyp),
                },
                rhs
            ));
        }
        return cir_expr_create_assign(cir_assign_create(lhs, rhs));
    }

    cir_calc_op_t op;
    if (!tkn_to_calc_op(expr->oper, &op)) {
        fprintf(stderr, "BUG: Unhandled infix operator %s\n", c_token_name[expr->oper]);
        abort();
    }

    bool compound = is_compound_assign(expr->oper);
    if (compound && (!lhs->common.is_lvalue || ltyp.qual.q_const)) {
        cctx_diagnostic(
            cc->cctx,
            expr->oper_pos,
            DIAG_ERR,
            "Left operand of %s is not a modifiable lvalue",
            c_token_name[expr->oper]
        );
        goto error;
    }

    if (compound) {
        cir_tmpval_t *tmp = cir_tmpval_create(lhs);
        lhs               = NULL;

        // Compute using tmpval.
        cir_expr_t *calc
            = expand_calc(cc, expr->pos, op, cir_expr_create_value(cir_value_create_tmpval(tmp)), rhs, true);
        if (!calc) {
            cir_tmpval_delete(tmp);
            return NULL;
        }
        if (!c_type_is_identical(calc->common.type, tmp->common.type, false)) {
            // Cast should always succeed because of prior type checks in `expand_calc`.
            calc = raw_cast(cc, expr->pos, c_type_clone(tmp->common.type), calc);
            assert(calc != NULL);
        }

        // Store result back to tmpval, thereby writing to lhs.
        cir_expr_t *assign
            = cir_expr_create_assign(cir_assign_create(cir_expr_create_value(cir_value_create_tmpval(tmp)), calc));

        vec_cir_tmpval_t tmps = {0};
        vec_push(&tmps, tmp);
        vec_cir_expr_t exprs = {0};
        vec_push(&exprs, assign);

        return cir_expr_create_exprs(cir_exprs_create2(cir_expr_common_clone(&calc->common), tmps, exprs));

    } else {
        return expand_calc(cc, expr->pos, op, lhs, rhs, false);
    }

error:
    if (lhs) {
        cir_expr_delete(lhs);
    }
    if (rhs) {
        cir_expr_delete(rhs);
    }
    return NULL;
}

// Compile a prefix expression.
cir_expr_t *c_compile2_expr_prefix(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_prefix_t const *expr) {
    cir_expr_t *val = c_compile2_expr(cc, scope, expr->val);
    if (!val) {
        return NULL;
    }

    // All prefix operators require pointer or integral type.
    if (
        val->common.type.prim >= C_PRIM_VOID && val->common.type.prim != C_COMP_POINTER
        && expr->oper != C_TKN_AND // The addrof operator allows *any* type.
        && !(
            (expr->oper == C_TKN_INC || expr->oper == C_TKN_DEC) && val->common.type.prim == C_COMP_POINTER
        ) // The inc/dec operators also allow pointer types.
        && !(
            (expr->oper == C_TKN_MUL || expr->oper == C_TKN_LNOT)
            && (val->common.type.prim == C_COMP_ARRAY || val->common.type.prim == C_COMP_POINTER)
        ) // The deref and logical not operators also allow pointer types and array decay.
    ) {
        cctx_diagnostic(
            cc->cctx,
            expr->oper_pos,
            DIAG_ERR,
            "Invalid argument type supplied to %s",
            c_token_name[expr->oper]
        );
        goto err0;
    }

    c_prim_t prim = val->common.type.prim < C_N_PRIM ? val->common.type.prim : cc->options.size_type;

    cir_expr_common_t common = {
        .pos          = expr->pos,
        .is_lvalue    = false,
        .allow_addrof = false,
        .type         = c_type_clone(val->common.type),
    };

    switch (expr->oper) {
        case C_TKN_LNOT: { // Logical NOT `!expr`
            cir_expr_t *zero = c_compile2_synth_iconst(cc, expr->pos, prim, I128_ZERO);
            return cir_expr_create_calc(cir_calc_create(common, CIR_CALC_EQ, val, zero));
        }
        case C_TKN_NOT: { // Bitwise NOT `~expr`
            cir_expr_t *mask = c_compile2_synth_iconst(cc, expr->pos, prim, UI128_MAX);
            return cir_expr_create_calc(cir_calc_create(common, CIR_CALC_BXOR, val, mask));
        }
        case C_TKN_INC: // Pre-increment `++expr`
        case C_TKN_DEC: // Pre-decrement `--expr`
            fprintf(stderr, "TODO: pre-increment/decrement\n");
            abort();
        case C_TKN_ADD: // Passthru `+`.
            c_type_delete(common.type);
            return val;
        case C_TKN_SUB: { // Arithmetic negate `-`.
            cir_expr_t *zero = c_compile2_synth_iconst(cc, expr->pos, prim, I128_ZERO);
            return cir_expr_create_calc(cir_calc_create(common, CIR_CALC_SUB, zero, val));
        }
        case C_TKN_AND: { // Address-of `&`
            if (!val->common.allow_addrof) {
                cctx_diagnostic(cc->cctx, expr->oper_pos, DIAG_ERR, "Cannot take the address of this rvalue");
                goto err1;
            }
            if (val->common.type.prim == C_COMP_FUNCTION) {
                // Similar to their funny deref semantics, doing addrof on a function is a no-op.
                // The function instead decays implicitly into a function pointer as needed.
                return val;
            }
            c_type_delete(common.type);
            common.type = c_type_clone_pointer(val->common.type);
            return cir_expr_create_deref(cir_deref_create(common, val));
        }
        case C_TKN_MUL: { // Dereference `*expr`
            if (val->common.type.prim != C_COMP_POINTER) {
                goto err1;
            }
            if (val->common.type.extra->inner.prim == C_COMP_FUNCTION) {
                // Funcptr types have funny semantics that mean deref doesn't actually do anything.
                return val;
            }
            c_type_delete(common.type);
            common.type = c_type_clone(val->common.type.extra->inner);
            return cir_expr_create_deref(cir_deref_create(common, val));
        }
        default: fprintf(stderr, "BUG: Unhandled prefix operator\n"); abort();
    }

err1:
    c_type_delete(common.type);
err0:
    cir_expr_delete(val);
    return NULL;
}

// Compile a suffix expression.
cir_expr_t *c_compile2_expr_suffix(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_suffix_t const *expr) {
    cir_expr_t *val = c_compile2_expr(cc, scope, expr->val);
    if (!val) {
        return NULL;
    }

    if (expr->oper != C_TKN_DEC && expr->oper != C_TKN_INC) {
        fprintf(stderr, "BUG: Unhandled suffix operator\n");
        abort();
    }

    // All suffix operators require pointer or integral type.
    if (val->common.type.prim >= C_PRIM_VOID && val->common.type.prim != C_COMP_POINTER) {
        cctx_diagnostic(
            cc->cctx,
            expr->oper_pos,
            DIAG_ERR,
            "Invalid argument type supplied to %s",
            c_token_name[expr->oper]
        );
        goto err0;
    }

    fprintf(stderr, "TODO: post-increment/decrement\n");
    abort();

err0:
    cir_expr_delete(val);
    return NULL;
}

// Compile a cast expression.
cir_expr_t *c_compile2_expr_cast(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_cast_t const *cast) {
    c_type_t spec_qual = c_compile2_spec_qual_list(cc, cast->type->spec_qual, scope);
    if (!c_type_is_valid(spec_qual)) {
        return NULL;
    }
    if (spec_qual.qual.s_typedef) {
        cctx_diagnostic(cc->cctx, cast->type->spec_qual->pos, DIAG_ERR, "typedef not allowed here");
        c_type_delete(spec_qual);
        return NULL;
    }
    c_ast_ident_t const *name;

    c_type_t type = c_compile2_type(cc, scope, spec_qual, cast->type->decl, &name);
    if (name) {
        cctx_diagnostic(cc->cctx, name->pos, DIAG_ERR, "Identifier not allowed here");
    }

    cir_expr_t *val = c_compile2_expr(cc, scope, cast->val);
    if (!val) {
        c_type_delete(type);
        return NULL;
    }

    return raw_cast(cc, cast->pos, type, val);
}

// Compile a call expression.
cir_expr_t *c_compile2_expr_call(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_call_t const *call) {
    cir_expr_t *func   = c_compile2_expr(cc, scope, call->func);
    bool        errors = !func;

    vec_cir_expr_t args = {0};
    for (size_t i = 0; i < call->args->items.len; i++) {
        cir_expr_t *arg = c_compile2_expr(cc, scope, call->args->items.arr[i]);
        if (!arg) {
            errors = true;
        } else {
            vec_push(&args, arg);
        }
    }

    if (errors) {
        goto error;
    }

    c_type_t func_type = func->common.type; // Reference.

    // Unwrap a function pointer to get at the underlying function type.
    if (func_type.prim == C_COMP_POINTER) {
        func_type = func_type.extra->inner;
    }
    if (func_type.prim != C_COMP_FUNCTION) {
        cctx_diagnostic(cc->cctx, call->pos, DIAG_ERR, "Called object is not a function or function pointer");
        goto error;
    }

    if (args.len != func_type.extra->func_type->args.len) {
        cctx_diagnostic(
            cc->cctx,
            call->pos,
            DIAG_ERR,
            "Function expects %zu argument(s), got %zu",
            func_type.extra->func_type->args.len,
            args.len
        );
        goto error;
    }

    // Type-check each argument and insert an implicit cast where the type differs.
    for (size_t i = 0; i < args.len; i++) {
        cir_expr_t *arg = args.arr[i];
        if (!c_type_is_castable(arg->common.type, arg->common.type)) {
            cctx_diagnostic(cc->cctx, arg->common.pos, DIAG_ERR, "Incompatible argument type in function call");
            goto error;
        }
        if (!c_type_is_identical(arg->common.type, arg->common.type, false)) {
            args.arr[i] = cir_expr_create_cast(cir_cast_create(
                (cir_expr_common_t){
                    .pos          = arg->common.pos,
                    .is_lvalue    = false,
                    .allow_addrof = false,
                    .type         = c_type_clone(func_type.extra->func_type->args.arr[i].type),
                },
                arg
            ));
        }
    }

    return cir_expr_create_call(cir_call_create(
        (cir_expr_common_t){
            .pos          = call->pos,
            .is_lvalue    = false,
            .allow_addrof = false,
            .type         = c_type_clone(func_type.extra->func_type->returns),
        },
        func,
        args
    ));

error:
    if (func) {
        cir_expr_delete(func);
    }
    for (size_t i = 0; i < args.len; i++) {
        cir_expr_delete(args.arr[i]);
    }
    vec_clear(&args);
    return NULL;
}

// Compile an identifier as part of an expression.
cir_expr_t *c_compile2_expr_ident(c_compiler_t *cc, cir_scope_t *scope, c_ast_ident_t const *ident) {
    cir_scope_val_t *val = cir_scope_lookup_value(scope, ident->name);
    if (!val) {
        cctx_diagnostic(cc->cctx, ident->pos, DIAG_ERR, "Use of undeclared identifier '%s'", ident->name);
        return NULL;
    }
    return cir_expr_create_value(cir_value_create_scope_val(val));
}

// Compile an integer constant as part of an expression.
cir_expr_t *c_compile2_expr_iconst(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_iconst_t const *iconst) {
    (void)scope;
    return c_compile2_synth_iconst(cc, iconst->pos, iconst->prim, iconst->iconst);
}

// Compile a string constant as part of an expression.
cir_expr_t *c_compile2_expr_sconst(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_sconst_t const *sconst) {
    (void)scope;

    // String literals have type `char[N]`, NUL-terminated.
    size_t len = sconst->value.len;
    if (len > INT32_MAX) {
        cctx_diagnostic(cc->cctx, sconst->pos, DIAG_ERR, "String constant exceeds implementation limits");
        return NULL;
    }
    uint8_t *blob = lilycc_malloc(len + 1);
    memcpy(blob, sconst->value.arr, len);
    blob[len] = 0;

    c_type_t type      = {0};
    type.extra         = lilycc_calloc(1, sizeof(c_bigtype_t));
    type.extra->inner  = C_TYPE_FROM_PRIM(C_PRIM_CHAR);
    type.extra->length = (int32_t)len;
    type.prim          = C_COMP_ARRAY;
    type.qual.q_const  = true;

    cir_expr_common_t common = {
        .pos          = sconst->pos,
        .type         = c_type_clone(type),
        .is_lvalue    = true,
        .allow_addrof = true,
    };
    return cir_expr_create_value(cir_value_create_comp_const(common, cir_comp_const_create(sconst->pos, type, blob)));
}

// Compile a compound literal as part of an expression.
cir_expr_t *
    c_compile2_expr_compliteral(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_compliteral_t const *compliteral) {
    fprintf(stderr, "TODO: c_compile2_expr_compliteral\n");
    abort();
}

// Compile an expression list.
cir_expr_t *c_compile2_expr_exprs(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_list_t const *exprs) {
    vec_cir_expr_t out    = {0};
    bool           errors = false;

    for (size_t i = 0; i < exprs->items.len; i++) {
        cir_expr_t *expr = c_compile2_expr(cc, scope, exprs->items.arr[i]);
        if (!expr) {
            errors = true;
        } else {
            vec_push(&out, expr);
        }
    }

    if (errors) {
        for (size_t i = 0; i < out.len; i++) {
            cir_expr_delete(out.arr[i]);
        }
        vec_clear(&out);
        return NULL;
    }

    cir_expr_common_t common;
    if (out.len) {
        common     = cir_expr_common_clone(&out.arr[out.len - 1]->common);
        common.pos = exprs->pos;
    } else {
        common = (cir_expr_common_t){
            .pos          = exprs->pos,
            .allow_addrof = false,
            .is_lvalue    = false,
            .type         = C_TYPE_FROM_PRIM(C_PRIM_VOID),
        };
    }

    return cir_expr_create_exprs(cir_exprs_create(common, out));
}

// Compile a compound literal/initializer given a known target type.
cir_value_t *c_compile2_compinit(c_compiler_t *cc, cir_scope_t *scope, c_type_t type, c_ast_init_list_t const *init) {
    fprintf(stderr, "TODO: c_compile2_compinit\n");
    abort();
}

// Helper that creates a synthetic integer constant.
cir_expr_t *c_compile2_synth_iconst(c_compiler_t *cc, pos_t pos, c_prim_t prim, i128_t value) {
    ir_prim_t  ir_prim  = c_prim_to_ir_type(cc, prim);
    ir_const_t ir_const = ir_cast(ir_prim, IR_CONST_S128(value));

    cir_value_t *cir_value = cir_value_create_const(
        (cir_expr_common_t){
            .pos          = pos,
            .is_lvalue    = false,
            .allow_addrof = false,
            .type         = C_TYPE_FROM_PRIM(prim),
        },
        cir_const_create(pos, prim, ir_const)
    );

    return cir_expr_create_value(cir_value);
}
