
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_compile_expr.h"

#include "arith128.h"
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
#include "unreachable.h"
#include "vec.h"

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



static c_prim_t usual_arith_conv(c_compiler_t *cc, c_type_ref_t type, c_type_ref_t other_type);



// Whether a primitive is an arithmetic type (integer or floating point).
static bool c_prim_is_arith(c_prim_t prim) {
    return prim < C_PRIM_VOID || prim == C_COMP_POINTER;
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
        c_prim_t prim = usual_arith_conv(cc, ltyp, rtyp);
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

// Determine the resulting primitive type for usual arithmetic conversion, if applicable.
// Returns `C_N_PRIM` if conversion cannot be performed (one of the operands is non-primitive and not an enum type).
// All lesser integer types are promoted to at least the rank of `int`.
// If `other_type` is a valid type, it is used to determine which integer type is the result of promotion.
static c_prim_t usual_arith_conv(c_compiler_t *cc, c_type_ref_t type, c_type_ref_t other_type) {
    // See: ISO/IEC 9899:2023 §6.3.2.8
    c_prim_t prim = type.prim;
    if (type.prim == C_COMP_ENUM) {
        prim = type.extra->enum_type->prim;
    }
    // See: ISO/IEC 9899:2023 §6.3.2.1
    if (prim == C_PRIM_USHORT && !cc->options.int32) {
        prim = C_PRIM_UINT;
    } else if (prim < C_PRIM_SINT) {
        prim = C_PRIM_SINT;
    }

    if (c_type_is_valid(other_type)) {
        // Same promotion for the other type.
        c_prim_t other_prim = other_type.prim;
        if (other_type.prim == C_COMP_ENUM) {
            other_prim = other_type.extra->enum_type->prim;
        }
        if (other_prim == C_PRIM_USHORT && !cc->options.int32) {
            other_prim = C_PRIM_UINT;
        } else if (other_prim < C_PRIM_SINT) {
            other_prim = C_PRIM_SINT;
        }

        if (prim >= C_N_PRIM || other_prim >= C_N_PRIM || prim == other_prim) {
            // No conversion can be done.
            return C_N_PRIM;
        }

        // See: ISO/IEC 9899:2023 §6.3.2.8
        // TODO: _Decimal128, _Decimal64 and _Decimal32.
        if (prim == C_PRIM_LDOUBLE || other_prim == C_PRIM_LDOUBLE) {
            return C_PRIM_LDOUBLE;
        } else if (prim == C_PRIM_DOUBLE || other_prim == C_PRIM_DOUBLE) {
            return C_PRIM_DOUBLE;
        } else if (prim == C_PRIM_FLOAT || other_prim == C_PRIM_FLOAT) {
            return C_PRIM_FLOAT;
        } else if ( // At this point, both types are either integer types or `bool`.
            c_prim_is_uint(cc->options.char_is_signed, prim)
            == c_prim_is_uint(cc->options.char_is_signed, other_prim)
        ) {
            // Conversion with the same sign uses the higher-ranked type.
            return prim > other_prim ? prim : other_prim;
        } else {
            // We have an unsigned and a signed type now; this block handles the remaining three clauses.
            c_prim_t u_prim, s_prim;
            if (c_prim_is_uint(cc->options.char_is_signed, prim)) {
                u_prim = prim;
                s_prim = other_prim;
            } else {
                u_prim = other_prim;
                s_prim = prim;
            }
            ir_prim_t u_ir_prim = c_type_to_ir_type(cc, C_TYPE_FROM_PRIM(u_prim));
            ir_prim_t s_ir_prim = c_type_to_ir_type(cc, C_TYPE_FROM_PRIM(s_prim));

            // Effectively, we must convert to whichever type can represent all values of the other.
            // If not possible, (i.e. same rank in IR types), convert to the unsigned type.
            if (u_ir_prim == s_ir_prim + 1) {
                // Same rank in IR types; use the unsigned type.
                return u_prim;
            } else {
                // Convert to the larger type.
                return u_ir_prim > s_ir_prim ? u_prim : s_prim;
            }
        }
    }

    return prim;
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

    if (val->tag == CIR_EXPR_VALUE && val->value->tag == CIR_VALUE_CONST && type.prim < C_PRIM_VOID) {
        ir_prim_t   dest_prim = c_type_to_ir_type(cc, type);
        ir_const_t  iconst    = ir_cast(dest_prim, val->value->iconst->iconst);
        cir_expr_t *res       = cir_expr_create_value(cir_value_create_const(cir_const_create(pos, type.prim, iconst)));
        c_type_delete(type);
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
        cir_expr_delete(lhs);
        cir_expr_delete(rhs);
        return NULL;
    }

    c_type_t type;
    if (op >= CIR_CALC_LAND) {
        type = C_TYPE_FROM_PRIM(C_PRIM_SINT);
    } else {
        c_prim_t prim = usual_arith_conv(cc, ltyp, rtyp);
        if (prim == C_N_PRIM) {
            type = c_type_clone(ltyp);
        } else {
            type = C_TYPE_FROM_PRIM(prim);
        }
    }

    if ((lhs->tag == CIR_EXPR_VALUE && lhs->value->tag == CIR_VALUE_CONST && lhs->common.type.prim < C_PRIM_VOID)
        && (rhs->tag == CIR_EXPR_VALUE && rhs->value->tag == CIR_VALUE_CONST && rhs->common.type.prim < C_PRIM_VOID)) {
        ir_const_t iconst;
        if (op == CIR_CALC_LAND) {
            bool lhs_b       = ir_calc1(IR_OP1_snez, lhs->value->iconst->iconst).constl;
            bool rhs_b       = ir_calc1(IR_OP1_snez, lhs->value->iconst->iconst).constl;
            iconst.prim_type = c_prim_to_ir_type(cc, C_PRIM_SINT);
            iconst.constl    = lhs_b & rhs_b;
            iconst.consth    = 0;
        } else if (op == CIR_CALC_LOR) {
            bool lhs_b       = ir_calc1(IR_OP1_snez, lhs->value->iconst->iconst).constl;
            bool rhs_b       = ir_calc1(IR_OP1_snez, lhs->value->iconst->iconst).constl;
            iconst.prim_type = c_prim_to_ir_type(cc, C_PRIM_SINT);
            iconst.constl    = lhs_b | rhs_b;
            iconst.consth    = 0;
        } else {
            ir_op2_type_t ir_op = cir_calc_op_to_ir_op2(op);
            iconst              = ir_calc2(ir_op, lhs->value->iconst->iconst, rhs->value->iconst->iconst);
        }

        cir_expr_t *res = cir_expr_create_value(cir_value_create_const(cir_const_create(pos, type.prim, iconst)));
        c_type_delete(type);
        cir_expr_delete(lhs);
        cir_expr_delete(rhs);
        return res;
    }

    return cir_expr_create_calc(cir_calc_create(
        (cir_expr_common_t){
            .pos          = pos,
            .type         = type,
            .allow_addrof = false,
            .is_lvalue    = false,
        },
        op,
        lhs,
        rhs
    ));
}

// Perform address-of operation.
static cir_expr_t *raw_addrof(c_compiler_t *cc, pos_t pos, cir_expr_t *val) {
    if (!val->common.allow_addrof && val->common.type.prim != C_COMP_STRUCT && val->common.type.prim != C_COMP_UNION
        && val->common.type.prim != C_COMP_ARRAY) {
        cctx_diagnostic(cc->cctx, val->common.pos, DIAG_ERR, "Cannot take the address of this value");
        cir_expr_delete(val);
        return NULL;
    }

    cir_expr_common_t common = {
        .pos          = pos,
        .type         = c_type_clone_pointer(val->common.type),
        .is_lvalue    = false,
        .allow_addrof = false,
    };
    return cir_expr_create_addrof(cir_addrof_create(common, val));
}

// Do array decay if needed.
static cir_expr_t *array_decay(c_compiler_t *cc, cir_expr_t *val) {
    c_type_ref_t type = val->common.type;

    if (type.prim != C_COMP_ARRAY) {
        return val;
    }

    cir_expr_t *tmp = raw_addrof(cc, val->common.pos, val);
    assert(tmp != NULL);
    c_type_t ptr_rc = c_type_clone_pointer(type.extra->inner);
    return raw_cast(cc, val->common.pos, ptr_rc, tmp);
}

// Emit a calculation operation as C IR.
// Handles pointer arithmetic, promotion, etc.
// Does not assign; `is_assign` is only used to check arithmetic rules.
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
        cir_expr_t *div
            = raw_calc(cc, pos, CIR_CALC_DIV, cast, c_compile2_synth_iconst(cc, pos, ptrdiff_prim, ui128(size)));
        assert(div != NULL);
        return div;
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

        cir_expr_t *cast1 = raw_cast(cc, pos, C_TYPE_FROM_PRIM(cc->options.size_type), rhs);
        assert(cast1 != NULL);
        cir_expr_t *offset = raw_calc(
            cc,
            pos,
            CIR_CALC_MUL,
            cast1,
            c_compile2_synth_iconst(cc, pos, cc->options.size_type, ui128(size))
        );
        assert(offset != NULL);
        cir_expr_t *cast2 = raw_cast(cc, pos, c_type_clone(ltyp), offset);
        assert(cast2 != NULL);
        return raw_calc(cc, pos, op, lhs, cast2);
    }

    if (!c_prim_is_arith(ltyp.prim)) {
        cctx_diagnostic(cc->cctx, lhs->common.pos, DIAG_ERR, "Expected arithmetic type");
        goto error;
    }
    if (!c_prim_is_arith(rtyp.prim)) {
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

// Emit a calculation-assignment as C IR.
// Handles pointer arithmetic, promotion, etc.
static cir_expr_t *expand_calc_assign(c_compiler_t *cc, pos_t pos, cir_calc_op_t op, cir_expr_t *lhs, cir_expr_t *rhs) {
    cir_tmpval_t *tmp = cir_tmpval_create(lhs);
    lhs               = NULL;

    // Compute using tmpval.
    cir_expr_t *calc = expand_calc(cc, pos, op, cir_expr_create_value(cir_value_create_tmpval(tmp)), rhs, true);
    if (!calc) {
        cir_tmpval_delete(tmp);
        return NULL;
    }
    if (!c_type_is_identical(calc->common.type, tmp->common.type, false)) {
        // Cast should always succeed because of prior type checks in `expand_calc`.
        calc = raw_cast(cc, pos, c_type_clone(tmp->common.type), calc);
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

    if (lhs->tag == CIR_EXPR_VALUE && rhs->tag == CIR_EXPR_VALUE) {
        if (lhs->value->tag == CIR_VALUE_CONST && rhs->value->tag == CIR_VALUE_COMP_CONST) {
            cir_expr_t *tmp = lhs;
            lhs             = rhs;
            rhs             = tmp;
            // Falls through into the next if statement below:
        }
        if (lhs->value->tag == CIR_VALUE_COMP_CONST && rhs->value->tag == CIR_VALUE_CONST) {
            c_type_ref_t type      = lhs->common.type;
            c_type_ref_t elem_type = type.extra->inner;
            uint64_t     size, align;
            if (!c_type_get_size(cc, elem_type, &size, &align)) {
                UNREACHABLE(); // Array type shouldn't have been constructed if the inner type was incomplete.
            }

            ir_const_t ir_index = rhs->value->iconst->iconst;
            ir_index.prim_type  = ir_prim_as_unsigned(ir_index.prim_type);
            ir_const_t bound    = {
                .prim_type = ir_index.prim_type,
                .const128  = ui128(type.extra->length),
            };
            if (ir_calc2(IR_OP2_sge, ir_index, bound).constl) {
                goto non_const_prop;
            }

            cir_expr_t *const_prop = c_compile2_const_from_bytes(
                cc,
                expr->pos,
                elem_type,
                lhs->value->comp_const->blob + size * ir_index.constl
            );
            if (const_prop) {
                cir_expr_delete(lhs);
                cir_expr_delete(rhs);
                return const_prop;
            }
        }
    }

non_const_prop:;
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
    c_compile2_expr_field(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_infix_t const *expr, bool is_arrow) {
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

    // Const-propagation logic.
    if (lhs->tag == CIR_EXPR_VALUE && lhs->value->tag == CIR_VALUE_COMP_CONST) {
        c_type_ref_t   struct_type = lhs->value->comp_const->type;
        c_field_info_t field       = c_type_get_field(cc, struct_type, name, expr->pos);
        if (!c_type_is_valid(field.type)) {
            cir_expr_delete(lhs);
            return NULL;
        }

        cir_expr_t *const_prop
            = c_compile2_const_from_bytes(cc, lhs->common.pos, field.type, lhs->value->comp_const->blob + field.offset);
        if (const_prop != NULL) {
            cir_expr_delete(lhs);
            return const_prop;
        }
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

    c_field_info_t field = c_type_get_field(cc, struct_type, name, expr->pos);
    if (!c_type_is_valid(field.type)) {
        cir_expr_delete(ptr_expr);
        return NULL;
    }

    // Build a pointer-to-field type, then `ptr + offset` as that type, then deref.
    c_type_t    field_ptr_type = c_type_clone_pointer(field.type);
    cir_expr_t *off_iconst = c_compile2_synth_iconst(cc, expr->oper_pos, cc->options.size_type, ui128(field.offset));
    cir_expr_t *add        = cir_expr_create_calc(cir_calc_create(
        (cir_expr_common_t){
            .pos          = expr->pos,
            .is_lvalue    = false,
            .allow_addrof = false,
            .type         = field_ptr_type,
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
        return c_compile2_expr_field(cc, scope, expr, false);
    }
    if (expr->oper == C_TKN_ARROW) {
        return c_compile2_expr_field(cc, scope, expr, true);
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
        return expand_calc_assign(cc, expr->pos, op, lhs, rhs);
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

    switch (expr->oper) {
        case C_TKN_LNOT: { // Logical NOT `!expr`
            cir_expr_t *zero = c_compile2_synth_iconst(cc, expr->pos, prim, I128_ZERO);
            return expand_calc(cc, expr->pos, CIR_CALC_EQ, val, zero, false);
        }
        case C_TKN_NOT: { // Bitwise NOT `~expr`
            cir_expr_t *mask = c_compile2_synth_iconst(cc, expr->pos, prim, UI128_MAX);
            return expand_calc(cc, expr->pos, CIR_CALC_BXOR, val, mask, false);
        }
        case C_TKN_INC:   // Pre-increment `++expr`
        case C_TKN_DEC: { // Pre-decrement `--expr`
            c_type_t    type = c_type_clone(val->common.type);
            cir_expr_t *one  = c_compile2_synth_iconst(cc, expr->pos, cc->options.size_type, ui128(1));
            cir_expr_t *calc
                = expand_calc_assign(cc, expr->pos, expr->oper == C_TKN_INC ? CIR_CALC_ADD : CIR_CALC_SUB, val, one);
            if (!calc) {
                return NULL;
            }
            return raw_cast(cc, expr->pos, type, calc);
        }
        case C_TKN_ADD: { // Arithmetic promote `+`.
            c_prim_t conv = usual_arith_conv(cc, val->common.type, C_TYPE_INVALID);
            assert(conv != C_N_PRIM); // Should always be possible.
            return raw_cast(cc, expr->pos, C_TYPE_FROM_PRIM(conv), val);
        }
        case C_TKN_SUB: { // Arithmetic negate `-`.
            cir_expr_t *zero = c_compile2_synth_iconst(cc, expr->pos, prim, I128_ZERO);
            return expand_calc(cc, expr->pos, CIR_CALC_SUB, zero, val, false);
        }
        case C_TKN_AND: { // Address-of `&`
            if (val->common.type.prim == C_COMP_FUNCTION) {
                // Similar to their funny deref semantics, doing addrof on a function is a no-op.
                // The function instead decays implicitly into a function pointer as needed.
                return val;
            }
            return raw_addrof(cc, expr->pos, val);
        }
        case C_TKN_MUL: { // Dereference `*expr`
            if (val->common.type.prim != C_COMP_POINTER) {
                goto err0;
            }
            cir_expr_common_t common = {
                .pos          = expr->pos,
                .is_lvalue    = false,
                .allow_addrof = false,
                .type         = c_type_clone(val->common.type),
            };
            if (val->common.type.extra->inner.prim == C_COMP_FUNCTION) {
                // Funcptr types have funny semantics that mean deref doesn't actually do anything.
                return val;
            }
            c_type_delete(common.type);
            common.type         = c_type_clone(val->common.type.extra->inner);
            common.allow_addrof = true;
            common.is_lvalue    = true;
            return cir_expr_create_deref(cir_deref_create(common, val));
        }
        default: fprintf(stderr, "BUG: Unhandled prefix operator\n"); abort();
    }

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

    c_type_t    type  = c_type_clone(val->common.type);
    cir_expr_t *one_a = c_compile2_synth_iconst(cc, expr->pos, cc->options.size_type, ui128(1));
    cir_expr_t *res
        = expand_calc_assign(cc, expr->pos, expr->oper == C_TKN_INC ? CIR_CALC_ADD : CIR_CALC_SUB, val, one_a);
    if (!res) {
        return NULL;
    }
    // Simply offset by one in the opposite direction to effectively get the same result as post-increment/decrement.
    cir_expr_t *one_b = c_compile2_synth_iconst(cc, expr->pos, cc->options.size_type, ui128(1));
    cir_expr_t *undo
        = expand_calc(cc, expr->pos, expr->oper == C_TKN_INC ? CIR_CALC_SUB : CIR_CALC_ADD, res, one_b, false);
    assert(undo != NULL); // If the calc one worked, so should this one.
    return raw_cast(cc, expr->pos, type, undo);

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
    return cir_expr_create_value(cir_value_create_scope_val(ident->pos, val));
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
    if (len > INT32_MAX - 1) {
        cctx_diagnostic(cc->cctx, sconst->pos, DIAG_ERR, "String constant exceeds implementation limits");
        return NULL;
    }
    uint8_t *blob = lilycc_malloc(len + 1);
    memcpy(blob, sconst->value.arr, len);
    blob[len] = 0;

    c_type_t type      = {0};
    type.extra         = lilycc_calloc(1, sizeof(c_bigtype_t));
    type.extra->inner  = C_TYPE_FROM_PRIM(C_PRIM_CHAR);
    type.extra->length = (int32_t)len + 1;
    type.prim          = C_COMP_ARRAY;
    type.qual.q_const  = true;

    return cir_expr_create_value(cir_value_create_comp_const(cir_comp_const_create(sconst->pos, type, blob)));
}

// Compile a compound literal as part of an expression.
cir_expr_t *
    c_compile2_expr_compliteral(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_compliteral_t const *compliteral) {
    c_type_t spec_qual_type = c_compile2_spec_qual_list(cc, compliteral->type->spec_qual, scope);
    if (!c_type_is_valid(spec_qual_type)) {
        return NULL;
    } else if (spec_qual_type.qual.s_typedef) {
        cctx_diagnostic(cc->cctx, compliteral->type->spec_qual->pos, DIAG_ERR, "typedef not allowed here");
        c_type_delete(spec_qual_type);
        return NULL;
    }
    c_ast_ident_t const *name;
    c_type_t             type = c_compile2_type(cc, scope, spec_qual_type, compliteral->type->decl, &name);
    if (!c_type_is_valid(type)) {
        return NULL;
    } else if (name) {
        cctx_diagnostic(cc->cctx, name->pos, DIAG_ERR, "name not allowed here");
    }

    return c_compile2_compinit(cc, scope, type, compliteral->type->pos, compliteral->init);
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

// Helper for `c_compile2_compinit` that keeps track of the field being written.
typedef struct {
    // Stack that indicates field being accessed.
    vec_size_t           stack;
    // Stores needed to initialize without optimization.
    vec_cir_comp_store_t stores;
    // Type being initialized.
    c_type_ref_t         type;
    // Current field offset.
    uint64_t             field_offset;
    // Current field type.
    c_type_t             field_type;
} c_init_cursor_t;

// Step into the first field of the current (compound-typed) field.
// Returns whether there is such a field (the compound type is not zero-sized).
static bool c_init_cursor_step_in(c_init_cursor_t *cursor) {
    // Non-owning.
    c_type_t cur = cursor->type;
    for (size_t i = 0;; i++) {
        size_t field = (i < cursor->stack.len) ? cursor->stack.arr[i] : 0;

        if (cur.prim == C_COMP_STRUCT || cur.prim == C_COMP_UNION) {
            c_struct_type_t const *comp = cur.extra->struct_type;
            if (comp->fields.len == 0) {
                return false;
            }
            cur = comp->fields.arr[field].type;

        } else if (cur.prim == C_COMP_ARRAY) {
            if (cur.extra->length <= 0) {
                return false;
            }
            cur = cur.extra->inner;

        } else {
            // Pointers, enums and primitive types.
            break;
        }

        if (i >= cursor->stack.len) {
            vec_push(&cursor->stack, 0);
        }
    }

    cursor->field_type = c_type_clone(cur);
    return true;
}

// Helper for `c_compile_comp_init` that selects a named field.
// Returns whether the field exists.
static inline bool c_init_cursor_select_named(
    c_compiler_t *cc, c_init_cursor_t *cursor, c_ast_ident_t const *name, bool err_notfound
) {
    c_type_ref_t field_type = cursor->field_type;
    if (field_type.prim != C_COMP_STRUCT && field_type.prim != C_COMP_UNION) {
        cctx_diagnostic(cc->cctx, name->pos, DIAG_ERR, "Unexpected named initializer field for non-struct/union type");
        return false;
    }

    c_struct_type_t const *comp = field_type.extra->struct_type;
    for (size_t i = 0; i < comp->fields.len; i++) {
        c_struct_field_t const *field = &comp->fields.arr[i];

        if (field->name && !strcmp(field->name, name->name)) {
            // Matching field name found.
            vec_push(&cursor->stack, i);
            cursor->field_offset += field->offset;
            c_type_t new_type     = c_type_clone(field->type);
            c_type_delete(cursor->field_type);
            cursor->field_type = new_type;
            return true;

        } else if (!field->name && (field_type.prim == C_COMP_STRUCT || field_type.prim == C_COMP_UNION)) {
            // Recursively search in anonymous nested structs/unions.
            c_type_t prev_type = c_type_clone(cursor->field_type);
            vec_push(&cursor->stack, i);
            uint64_t field_offset  = field->offset;
            cursor->field_offset  += field_offset;
            c_type_t new_type      = c_type_clone(field->type);
            c_type_delete(cursor->field_type);
            cursor->field_type = new_type;

            if (c_init_cursor_select_named(cc, cursor, name, false)) {
                c_type_delete(prev_type);
                return true;
            } else {
                // Restore cursor to original value if search fails.
                c_type_delete(cursor->field_type);
                cursor->field_type = prev_type;
                vec_pop(&cursor->stack);
                cursor->field_offset -= field_offset;
            }
        }
    }

    if (err_notfound) {
        cctx_diagnostic(cc->cctx, name->pos, DIAG_ERR, "No such struct/union field");
    }

    return false;
}

// Helper for `c_compile_comp_init` that selects an indexed field.
// Returns whether the field exists.
static inline bool
    c_init_cursor_select_indexed(c_compiler_t *cc, c_init_cursor_t *cursor, cir_expr_t *index, pos_t index_pos) {
    c_type_ref_t field_type = cursor->field_type;
    if (field_type.prim != C_COMP_ARRAY) {
        cctx_diagnostic(cc->cctx, index_pos, DIAG_ERR, "Unexpected indexed initializer field for non-array type");
        cir_expr_delete(index);
        return false;
    }

    assert(index->tag == CIR_EXPR_VALUE);
    assert(index->value->tag == CIR_VALUE_CONST);

    ir_const_t ir_index = index->value->iconst->iconst;
    if (ir_prim_is_signed(ir_index.prim_type)) {
        i128_t s_index = ir_cast(IR_PRIM_s128, ir_index).const128;
        if (cmp128s(s_index, I128_ZERO) < 0) {
            char buf[40];
            itoa128(neg128(s_index), 0, buf);
            cctx_diagnostic(cc->cctx, index_pos, DIAG_ERR, "Negative initializer index -%s is not allowed", buf);
            cir_expr_delete(index);
            return false;
        }
    }
    i128_t u_index = ir_cast(IR_PRIM_u128, ir_index).const128;
    if (cmp128u(u_index, ui128(field_type.extra->length)) > 0) {
        char buf[40];
        itoa128(u_index, 0, buf);
        cctx_diagnostic(
            cc->cctx,
            index_pos,
            DIAG_ERR,
            "Initializer index %s exceeds array bounds (length %" PRId32 ")",
            buf,
            field_type.extra->length
        );
        cir_expr_delete(index);
        return false;
    }

    uint64_t inner_size, inner_align;
    if (!c_type_get_size(cc, field_type.extra->inner, &inner_size, &inner_align)) {
        UNREACHABLE();
    }

    uint64_t index_64 = lo64(u_index);
    vec_push(&cursor->stack, lo64(u_index));
    cursor->field_offset += index_64 * inner_size;
    c_type_t new_type     = c_type_clone(field_type.extra->inner);
    c_type_delete(cursor->field_type);
    cursor->field_type = new_type;
    cir_expr_delete(index);
    return true;
}

// Helper for `c_init_field` that moves the cursor to the next field.
static void c_init_cursor_next(c_compiler_t *cc, c_init_cursor_t *cursor) {
    while (cursor->stack.len) {
        bool     has_next = false;
        // Non-owning.
        c_type_t cur      = cursor->type;
        for (size_t depth = 0; depth < cursor->stack.len; depth++) {
            size_t index = cursor->stack.arr[depth];
            if (cur.prim == C_COMP_STRUCT) {
                c_struct_type_t const  *comp  = cur.extra->struct_type;
                c_struct_field_t const *field = &comp->fields.arr[index];
                cur                           = field->type;
                cursor->field_offset          = field->offset;
                has_next                      = index + 1 < comp->fields.len;
                c_type_delete(cursor->field_type);
                cursor->field_type = c_type_clone(field->type);

            } else if (cur.prim == C_COMP_UNION) {
                c_struct_type_t const  *comp  = cur.extra->struct_type;
                c_struct_field_t const *field = &comp->fields.arr[index];
                cur                           = field->type;
                cursor->field_offset          = field->offset;
                has_next                      = false;
                c_type_delete(cursor->field_type);
                cursor->field_type = c_type_clone(field->type);

            } else if (cur.prim == C_COMP_ARRAY) {
                uint64_t size, align;
                if (!c_type_get_size(cc, cur.extra->inner, &size, &align)) {
                    UNREACHABLE();
                }
                cursor->field_offset += size;
                has_next              = index < (uint32_t)cur.extra->length;
                cur                   = cur.extra->inner;
                c_type_delete(cursor->field_type);
                cursor->field_type = c_type_clone(cur);

            } else {
                // Pointers, enums and primitive types.
                has_next = false;
            }
            if (!has_next) {
                break;
            }
        }
        if (has_next) {
            break;
        }

        vec_pop(&cursor->stack);
    }

    if (cursor->stack.len) {
        cursor->stack.arr[cursor->stack.len - 1]++;
    }
}

// Compile a compound initializer for a scalar type.
static cir_expr_t *
    c_compile_scalar_init(c_compiler_t *cc, cir_scope_t *scope, c_ast_initval_t const *val, c_prim_t prim) {
    // Handle nested compound initializers.
    bool warn_excess = true;
    int  warn_braces = 0;
    while (val->tag == C_AST_TAG_INITVAL_COMPOUND) {
        c_ast_init_list_t const *list = val->initval_compound;

        if (list->items.len == 0) {
            // Empty list; zero it.
            return cir_expr_create_value(cir_value_create_const(cir_const_create(
                list->pos,
                prim,
                (ir_const_t){
                    .prim_type = c_type_to_ir_type(cc, C_TYPE_FROM_PRIM(prim)),
                    .const128  = I128_ZERO,
                }
            )));
        } else if (list->items.len > 1) {
            if (warn_excess) {
                cctx_diagnostic(cc->cctx, list->items.arr[1]->pos, DIAG_WARN, "Excess elements in scalar initializer");
            }
            warn_excess = false;
        }

        c_ast_init_t const *init = list->items.arr[0];
        switch (init->tag) {
            case C_AST_TAG_INIT_NAMED:
                cctx_diagnostic(
                    cc->cctx,
                    init->init_named->name->pos,
                    DIAG_ERR,
                    "Designated initializer used with a scalar type"
                );
                return NULL;
            case C_AST_TAG_INIT_INDEXED:
                cctx_diagnostic(
                    cc->cctx,
                    init->init_indexed->index->pos,
                    DIAG_ERR,
                    "Designated initializer used with a scalar type"
                );
                return NULL;
            case C_AST_TAG_INIT_VAL:
                if (warn_braces == 1) {
                    cctx_diagnostic(
                        cc->cctx,
                        list->items.arr[1]->pos,
                        DIAG_WARN,
                        "Excess braces around scalar initializer"
                    );
                }
                if (warn_braces < 2) {
                    warn_braces++;
                }
                val = init->init_val;
                break;
            case C_AST_TAG_INIT_GARBAGE: return NULL; // Diagnostic already emitted.
        }
    }

    cir_expr_t *value = c_compile2_expr(cc, scope, val->initval_expr);
    if (!value) {
        return NULL;
    }

    if (value->common.type.prim == prim) {
        return value;
    }

    return raw_cast(cc, val->pos, C_TYPE_FROM_PRIM(prim), value);
}

// Compile excess nested initializers.
// Used to check for errors, but the code will effectively run.
// Returns true if a compile error occurred.
static bool c_compile_excess_init(c_compiler_t *cc, cir_scope_t *scope, c_ast_initval_t const *init) {
    if (init->tag == C_AST_TAG_INITVAL_COMPOUND) {
        bool err = false;
        for (size_t i = 0; i < init->initval_compound->items.len; i++) {
            c_ast_init_t const *cur = init->initval_compound->items.arr[i];
            while (true) {
                switch (cur->tag) {
                    case C_AST_TAG_INIT_NAMED: cur = cur->init_named->value; break;
                    case C_AST_TAG_INIT_INDEXED: cur = cur->init_indexed->value; break;
                    case C_AST_TAG_INIT_VAL: err |= c_compile_excess_init(cc, scope, cur->init_val); goto for_continue;
                    case C_AST_TAG_INIT_GARBAGE: err = true; goto for_continue;
                }
            }
        for_continue:;
        }
        return err;
    } else {
        assert(init->tag == C_AST_TAG_INITVAL_EXPR);
        // Detached code created here means that it will compile but never run.
        cir_expr_t *res = c_compile2_expr(cc, scope, init->initval_expr);
        if (res) {
            cir_expr_delete(res);
            return false;
        } else {
            return true;
        }
    }
}

// Compile a compound literal/initializer given a known target type.
cir_expr_t *c_compile2_compinit(
    c_compiler_t *cc, cir_scope_t *scope, c_type_t type, pos_t type_pos, c_ast_init_list_t const *init
) {
    // Check type completeness.
    uint64_t size, align;
    if (!c_type_get_size(cc, type, &size, &align)) {
        cctx_diagnostic(cc->cctx, type_pos, DIAG_ERR, "Usage of incomplete type");
        return NULL;
    }

    // Compile-time optimization for zero-initializations.
    ir_prim_t prim = c_type_to_ir_type(cc, type);
    if (init->items.len == 0) {
        if (prim < IR_N_PRIM) {
            ir_const_t zero = {
                .prim_type = prim,
                .const128  = I128_ZERO,
            };
            cir_const_t *iconst = cir_const_create(init->pos, type.prim, zero);
            c_type_delete(type);
            return cir_expr_create_value(cir_value_create_const(iconst));

        } else {
            return cir_expr_create_value(cir_value_create_comp_const(
                cir_comp_const_create(init->pos, c_type_clone(type), lilycc_calloc(size, 1))
            ));
        }
    }

    // Initializer of scalar types.
    if (prim < IR_N_PRIM) {
        c_prim_t c_prim = type.prim;
        c_type_delete(type);
        c_ast_initval_t dummy = {
            .pos              = init->pos,
            .tag              = C_AST_TAG_INITVAL_COMPOUND,
            .initval_compound = (c_ast_init_list_t *)init,
        };
        return c_compile_scalar_init(cc, scope, &dummy, c_prim);
    }

    c_init_cursor_t cursor = {
        .stack        = {0},
        .stores       = {0},
        .type         = type,
        .field_offset = 0,
        .field_type   = C_TYPE_INVALID,
    };
    if (type.prim == C_COMP_ARRAY) {
        cursor.field_type = c_type_clone(type.extra->inner);
    } else {
        assert(type.prim == C_COMP_STRUCT || type.prim == C_COMP_UNION);
        c_struct_type_t const *comp = type.extra->struct_type;
        if (comp->fields.len > 0) {
            cursor.field_type = c_type_clone(comp->fields.arr[0].type);
        }
    }

    // Collect unoptimized stores from the initializer.
    bool error    = false;
    bool is_const = true;
    for (size_t i = 0; i < init->items.len; i++) {
        bool                field_error = false;
        bool                can_step_in = true;
        c_ast_init_t const *field       = init->items.arr[i];

        if (field->tag == C_AST_TAG_INIT_NAMED || field->tag == C_AST_TAG_INIT_INDEXED) {
            can_step_in = false;
            vec_clear(&cursor.stack);
            cursor.field_offset = 0;
            c_type_delete(cursor.field_type);
            cursor.field_type = c_type_clone(cursor.type);
        }

        while (1) {
            if (field->tag == C_AST_TAG_INIT_NAMED) {
                // Named initializer field, e.g. `.foo = bar`.
                if (!c_init_cursor_select_named(cc, &cursor, field->init_named->name, true)) {
                    field_error = true;
                }
                field = field->init_named->value;

            } else if (field->tag == C_AST_TAG_INIT_INDEXED) {
                // Indexed initializer field, e.g. `[foo] = bar`.
                cir_expr_t *res = c_compile2_expr(cc, scope, field->init_indexed->index);
                if (res == NULL || !c_init_cursor_select_indexed(cc, &cursor, res, field->init_indexed->index->pos)) {
                    field_error = true;
                }
                field = field->init_indexed->value;

            } else {
                assert(field->tag == C_AST_TAG_INIT_VAL);
                break;
            }
        }

        c_ast_initval_t const *val = field->init_val;
        if (field_error) {
            error = true;
            break;

        } else if (cursor.stack.len == 0) {
            // An excess initializer; the expressions are compiled but will not run.
            error |= c_compile_excess_init(cc, scope, val);

        } else if (val->tag == C_AST_TAG_INITVAL_COMPOUND) {
            // Nested initializer.
            cir_expr_t *res;
            if (cursor.field_type.prim < C_PRIM_VOID) {
                // Directly compile scalar initializer.
                res = c_compile_scalar_init(cc, scope, val, cursor.field_type.prim);

            } else {
                // Recursively compile compound initializer.
                res = c_compile2_compinit(
                    cc,
                    scope,
                    c_type_clone(cursor.field_type),
                    (pos_t){0},
                    val->initval_compound
                );
            }

            if (!res) {
                error = true;
            } else {
                is_const &= res->tag == CIR_EXPR_VALUE
                            && (res->value->tag == CIR_VALUE_CONST || res->value->tag == CIR_VALUE_COMP_CONST);

                cir_comp_store_t store = {
                    .offset = cursor.field_offset,
                    .value  = res,
                };
                vec_push(&cursor.stores, store);
            }

            c_init_cursor_next(cc, &cursor);

        } else {
            cir_expr_t *res = c_compile2_expr(cc, scope, val->initval_expr);
            if (!res) {
                error = true;
            } else {
                is_const &= res->tag == CIR_EXPR_VALUE
                            && (res->value->tag == CIR_VALUE_CONST || res->value->tag == CIR_VALUE_COMP_CONST);

                if (can_step_in && !c_type_is_compatible(cursor.field_type, res->common.type)) {
                    c_init_cursor_step_in(&cursor);
                }

                if (!c_type_is_compatible(cursor.field_type, res->common.type)) {
                    cctx_diagnostic(cc->cctx, field->pos, DIAG_ERR, "Initializer with value of incompatible type");
                    error = true;

                } else {
                    cir_expr_t *cast = raw_cast(cc, res->common.pos, c_type_clone(cursor.field_type), res);
                    assert(cast != NULL);
                    cir_comp_store_t store = {
                        .offset = cursor.field_offset,
                        .value  = cast,
                    };
                    vec_push(&cursor.stores, store);
                }
            }

            c_init_cursor_next(cc, &cursor);
        }
    }

    if (error) {
        goto out_del_stores;
    }

    // Emit warnings about reinitializations of fields.
    uint32_t *bytes_init = lilycc_calloc((size + 3) / 4, sizeof(uint32_t));
    for (size_t i = 0; i < cursor.stores.len; i++) {
        cir_comp_store_t const *store = &cursor.stores.arr[i];
        uint64_t                write_size, write_align;
        if (!c_type_get_size(cc, store->value->common.type, &write_size, &write_align)) {
            UNREACHABLE();
        }

        bool overlap = false;
        for (size_t x = 0; x < write_size; x++) {
            size_t byte            = x + store->offset;
            overlap               |= (bytes_init[byte / 32] >> (byte % 32)) & 1;
            bytes_init[byte / 32] |= 1 << (byte % 32);
        }

        if (overlap) {
            cctx_diagnostic(cc->cctx, store->value->common.pos, DIAG_WARN, "Initializer overwrites previous value");
        }
    }
    lilycc_free(bytes_init);

    // Collect the writes.
    cir_value_t *compinit;
    if (is_const) {
        uint8_t *blob = lilycc_calloc(size, 1);

        for (size_t i = 0; i < cursor.stores.len; i++) {
            cir_comp_store_t const *store = &cursor.stores.arr[i];
            assert(store->value->tag == CIR_EXPR_VALUE);
            cir_value_t const *value = store->value->value;

            if (value->tag == CIR_VALUE_COMP_CONST) {
                uint64_t size, align;
                if (!c_type_get_size(cc, value->common.type, &size, &align)) {
                    UNREACHABLE();
                }
                memcpy(blob + store->offset, value->comp_const->blob, size);

            } else {
                assert(value->tag == CIR_VALUE_CONST);
                ir_const_to_blob(value->iconst->iconst, blob + store->offset, cc->options.big_endian);
            }
        }

        compinit = cir_value_create_comp_const(cir_comp_const_create(init->pos, c_type_clone(type), blob));

    out_del_stores:
        for (size_t i = 0; i < cursor.stores.len; i++) {
            cir_expr_delete(cursor.stores.arr[i].value);
        }
        vec_clear(&cursor.stores);
    } else {
        compinit = cir_value_create_comp_value(cir_comp_value_create(init->pos, c_type_clone(type), cursor.stores));
    }

    // Final cleanup.
    c_type_delete(type);
    c_type_delete(cursor.field_type);
    vec_clear(&cursor.stack);

    if (error) {
        return NULL;
    } else {
        return cir_expr_create_value(compinit);
    }
}

// Helper that creates a synthetic integer constant.
cir_expr_t *c_compile2_synth_iconst(c_compiler_t *cc, pos_t pos, c_prim_t prim, i128_t value) {
    ir_prim_t  ir_prim  = c_prim_to_ir_type(cc, prim);
    ir_const_t ir_const = ir_cast(ir_prim, IR_CONST_S128(value));

    cir_value_t *cir_value = cir_value_create_const(cir_const_create(pos, prim, ir_const));

    return cir_expr_create_value(cir_value);
}

// Helper that converts bytes into a constant value.
// Unlike other functions, returning NULL is not an error but indicates this const-propagation is not possible.
// This function assumes that `type` is a complete type.
cir_expr_t *c_compile2_const_from_bytes(c_compiler_t *cc, pos_t pos, c_type_ref_t type, uint8_t const *blob) {
    if (type.prim == C_COMP_STRUCT || type.prim == C_COMP_UNION || type.prim == C_COMP_ARRAY) {
        uint64_t size, align;
        if (!c_type_get_size(cc, type, &size, &align)) {
            UNREACHABLE(); // Must be a complete type because so is the struct we're reading from.
        }

        assert(size < SIZE_MAX); // Should be impossible because we're reading from a blob already.
        uint8_t *new_blob = lilycc_malloc(size);
        memcpy(new_blob, blob, size);

        c_type_t type2               = c_type_clone(type);
        type2.qual                   = (c_qual_t){.q_const = true};
        cir_comp_const_t *comp_const = cir_comp_const_create(pos, type2, new_blob);
        return cir_expr_create_value(cir_value_create_comp_const(comp_const));

    } else if (type.prim < C_PRIM_VOID || type.prim == C_COMP_ENUM) {
        c_prim_t prim = type.prim;
        if (type.prim == C_COMP_ENUM) {
            prim = type.extra->enum_type->prim;
        }
        ir_prim_t ir_prim = c_type_to_ir_type(cc, type);

        ir_const_t iconst = ir_const_from_blob(ir_prim, blob, cc->options.big_endian);
        return cir_expr_create_value(cir_value_create_const(cir_const_create(pos, prim, iconst)));
    }

    return NULL;
}
