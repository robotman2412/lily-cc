
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_compile_expr.h"

#include "c_ir.h"
#include "c_tokenizer.h"
#include "c_types.h"
#include "compiler.h"
#include "ir_interpreter.h"
#include "refcount.h"
#include "vec.h"

#include <assert.h>
#include <stdio.h>



// Compile an expression.
// Returns `NULL` on semantic errors.
cir_expr_t *c_compile2_expr(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_t const *expr) {
    switch (expr->tag) {
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


// Compile an index expression.
cir_expr_t *c_compile2_expr_index(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_index_t const *expr) {
    cir_expr_t *lhs = c_compile2_expr(cc, scope, expr->lhs);
    cir_expr_t *rhs = c_compile2_expr(cc, scope, expr->rhs);
    if (!lhs || !rhs) {
        goto error;
    }

    // Apply array-to-pointer decay and integer promotion to both operands.
    lhs = c_compile2_promotion(cc, lhs, true);
    rhs = c_compile2_promotion(cc, rhs, true);

    bool lhs_is_ptr = c_type_is_pointer(lhs->common.type_rc->data);
    bool rhs_is_ptr = c_type_is_pointer(rhs->common.type_rc->data);

    // Exactly one operand must be of pointer type.
    if (lhs_is_ptr == rhs_is_ptr) {
        cctx_diagnostic(
            cc->cctx,
            expr->pos,
            DIAG_ERR,
            "Exactly one operand to index expression must be of pointer type"
        );
        goto error;
    }

    // Arrange so that `lhs` holds the pointer and `rhs` holds the integer index.
    if (rhs_is_ptr) {
        cir_expr_t *tmp = lhs;
        lhs             = rhs;
        rhs             = tmp;
    }
    c_type_t const *ptr_type = lhs->common.type_rc->data;

    // Pointer arithmetic in the C IR does not implicitly scale by the inner type's size; do it explicitly.
    cir_expr_t *scaled = c_compile2_ptr_premul(cc, rhs, ptr_type, false);
    if (!scaled) {
        goto error;
    }
    rhs = scaled;

    // ptr + index, retaining the pointer type.
    cir_expr_t *add = cir_expr_create_calc(cir_calc_create(
        (cir_expr_common_t){
            .pos          = expr->pos,
            .type_rc      = rc_share(lhs->common.type_rc),
            .is_lvalue    = false,
            .allow_addrof = false,
        },
        CIR_CALC_ADD,
        false,
        lhs,
        rhs
    ));

    // Dereference to produce the indexed lvalue.
    return cir_expr_create_deref(cir_deref_create(
        (cir_expr_common_t){
            .pos          = expr->pos,
            .type_rc      = rc_share(ptr_type->inner),
            .is_lvalue    = true,
            .allow_addrof = true,
        },
        add
    ));

error:
    if (lhs) {
        cir_expr_delete(lhs);
    }
    if (rhs) {
        cir_expr_delete(rhs);
    }
    return NULL;
}

// Cast an expression to a given primitive type if it isn't already.
// Consumes `value`; returns the (possibly wrapped) expression.
static cir_expr_t *c_compile2_cast_to_prim(c_compiler_t *cc, cir_expr_t *value, c_prim_t target) {
    c_type_t const *type = value->common.type_rc->data;
    if (type->primitive == target) {
        return value;
    }
    return cir_expr_create_cast(cir_cast_create(
        (cir_expr_common_t){
            .pos          = value->common.pos,
            .is_lvalue    = false,
            .allow_addrof = false,
            .type_rc      = rc_share(&cc->prim_rcs[target]),
        },
        value
    ));
}

// Map an infix operator token to its `cir_calc_op_t`.
// Works for both plain operators (e.g. `+`) and the calc part of compound assignments (e.g. `+=`).
static bool tkn_to_calc_op(c_tokentype_t oper, cir_calc_op_t *out) {
    switch (oper) {
        case C_TKN_ADD: case C_TKN_ADD_S: *out = CIR_CALC_ADD; return true;
        case C_TKN_SUB: case C_TKN_SUB_S: *out = CIR_CALC_SUB; return true;
        case C_TKN_MUL: case C_TKN_MUL_S: *out = CIR_CALC_MUL; return true;
        case C_TKN_DIV: case C_TKN_DIV_S: *out = CIR_CALC_DIV; return true;
        case C_TKN_MOD: case C_TKN_MOD_S: *out = CIR_CALC_MOD; return true;
        case C_TKN_SHL: case C_TKN_SHL_S: *out = CIR_CALC_SHL; return true;
        case C_TKN_SHR: case C_TKN_SHR_S: *out = CIR_CALC_SHR; return true;
        case C_TKN_AND: case C_TKN_AND_S: *out = CIR_CALC_BAND; return true;
        case C_TKN_OR:  case C_TKN_OR_S:  *out = CIR_CALC_BOR; return true;
        case C_TKN_XOR: case C_TKN_XOR_S: *out = CIR_CALC_BXOR; return true;
        case C_TKN_LAND: *out = CIR_CALC_LAND; return true;
        case C_TKN_LOR:  *out = CIR_CALC_LOR; return true;
        case C_TKN_EQ:   *out = CIR_CALC_EQ; return true;
        case C_TKN_NE:   *out = CIR_CALC_NE; return true;
        case C_TKN_LT:   *out = CIR_CALC_LT; return true;
        case C_TKN_LE:   *out = CIR_CALC_LE; return true;
        case C_TKN_GT:   *out = CIR_CALC_GT; return true;
        case C_TKN_GE:   *out = CIR_CALC_GE; return true;
        default: return false;
    }
}

// Whether `oper` is one of the compound-assignment tokens (`+=`, `-=`, ...).
static bool is_compound_assign(c_tokentype_t oper) {
    return oper >= C_TKN_ADD_S && oper <= C_TKN_XOR_S;
}

// Compile a struct/union member access (`.` or `->`).
static cir_expr_t *c_compile2_expr_member(
    c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_infix_t const *expr, bool is_arrow
) {
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
    cir_expr_t     *ptr_expr;
    c_type_t const *struct_type;
    if (is_arrow) {
        lhs                  = c_compile2_promotion(cc, lhs, false);
        c_type_t const *type = lhs->common.type_rc->data;
        if (type->primitive != C_COMP_POINTER) {
            cctx_diagnostic(cc->cctx, expr->oper_pos, DIAG_ERR, "Left operand of -> must be a pointer");
            cir_expr_delete(lhs);
            return NULL;
        }
        struct_type = type->inner->data;
        ptr_expr    = lhs;
    } else {
        c_type_t const *type = lhs->common.type_rc->data;
        if (type->primitive != C_COMP_STRUCT && type->primitive != C_COMP_UNION) {
            cctx_diagnostic(cc->cctx, expr->oper_pos, DIAG_ERR, "Left operand of . must be a struct or union");
            cir_expr_delete(lhs);
            return NULL;
        }
        if (!lhs->common.allow_addrof) {
            cctx_diagnostic(cc->cctx, expr->oper_pos, DIAG_ERR, "Cannot access member of non-addressable value");
            cir_expr_delete(lhs);
            return NULL;
        }
        struct_type = type;
        rc_t ptr_rc = c_type_to_pointer(cc, rc_share(lhs->common.type_rc));
        ptr_expr    = cir_expr_create_addrof(cir_addrof_create(
            (cir_expr_common_t){
                .pos          = expr->pos,
                .is_lvalue    = false,
                .allow_addrof = false,
                .type_rc      = ptr_rc,
            },
            lhs
        ));
    }

    if (struct_type->primitive != C_COMP_STRUCT && struct_type->primitive != C_COMP_UNION) {
        cctx_diagnostic(cc->cctx, expr->oper_pos, DIAG_ERR, "Member access on non-struct/union type");
        cir_expr_delete(ptr_expr);
        return NULL;
    }

    uint64_t         offset = 0;
    c_field_t const *field  = c_type_get_field(cc, struct_type, name, &offset);
    if (!field) {
        cctx_diagnostic(cc->cctx, expr->rhs->pos, DIAG_ERR, "No member named '%s'", name);
        cir_expr_delete(ptr_expr);
        return NULL;
    }

    // Build a pointer-to-field type, then `ptr + offset` as that type, then deref.
    rc_t        field_ptr_rc = c_type_to_pointer(cc, rc_share(field->type_rc));
    cir_expr_t *off_iconst   = c_compile2_synth_iconst(cc, expr->oper_pos, cc->options.size_type, int128(0, offset));
    cir_expr_t *add          = cir_expr_create_calc(cir_calc_create(
        (cir_expr_common_t){
            .pos          = expr->pos,
            .is_lvalue    = false,
            .allow_addrof = false,
            .type_rc      = field_ptr_rc,
        },
        CIR_CALC_ADD,
        false,
        ptr_expr,
        off_iconst
    ));
    return cir_expr_create_deref(cir_deref_create(
        (cir_expr_common_t){
            .pos          = expr->pos,
            .is_lvalue    = true,
            .allow_addrof = true,
            .type_rc      = rc_share(field->type_rc),
        },
        add
    ));
}

// Build an arithmetic / comparison / logical / bitwise binary op given already-validated promoted operands.
// Consumes `lhs` and `rhs`. The result has the common type for arithmetic ops, or `int` for comparison/logical ops.
// If `is_assign`, `lhs` is an lvalue and the calc operates at its type; result type is the lhs type.
static cir_expr_t *c_compile2_arith_op(
    c_compiler_t *cc, pos_t pos, cir_calc_op_t op, bool is_assign, cir_expr_t *lhs, cir_expr_t *rhs
) {
    bool is_bool_result = (op >= CIR_CALC_LAND && op <= CIR_CALC_GE);

    c_type_t const *lhs_type_data = lhs->common.type_rc->data;
    c_type_t const *rhs_type_data = rhs->common.type_rc->data;
    c_prim_t        lhs_prim      = lhs_type_data->primitive;
    c_prim_t        rhs_prim      = rhs_type_data->primitive;

    rc_t result_rc;
    if (is_assign) {
        // Compound assignment: the calc operates at the lhs's type. Cast rhs in if needed.
        // The lhs is left as-is — codegen will load it via the assign semantics of the calc.
        if (rhs_prim != lhs_prim && lhs_prim < C_N_PRIM && rhs_prim < C_N_PRIM) {
            rhs = c_compile2_cast_to_prim(cc, rhs, lhs_prim);
        }
        result_rc = rc_share(lhs->common.type_rc);
    } else if (lhs_prim == C_COMP_POINTER || rhs_prim == C_COMP_POINTER) {
        // Pointer compares produce `int`; the calc itself operates at the size type.
        result_rc = is_bool_result ? rc_share(&cc->prim_rcs[C_PRIM_SINT])
                                   : rc_share(&cc->prim_rcs[cc->options.size_type]);
    } else {
        c_prim_t result_prim = c_prim_promote(lhs_prim, rhs_prim);
        lhs                  = c_compile2_cast_to_prim(cc, lhs, result_prim);
        rhs                  = c_compile2_cast_to_prim(cc, rhs, result_prim);
        result_rc            = is_bool_result ? rc_share(&cc->prim_rcs[C_PRIM_SINT])
                                              : rc_share(&cc->prim_rcs[result_prim]);
    }

    return cir_expr_create_calc(cir_calc_create(
        (cir_expr_common_t){
            .pos          = pos,
            .is_lvalue    = false,
            .allow_addrof = false,
            .type_rc      = result_rc,
        },
        op,
        is_assign,
        lhs,
        rhs
    ));
}

// Compile a `+` or `-` where one or both operands may be pointers.
// Consumes `lhs` and `rhs`. Returns NULL on error (after deleting them).
// If `is_assign`, the result is stored back to `lhs`; `ptr - ptr` assignment is not valid here.
static cir_expr_t *c_compile2_ptr_arith(
    c_compiler_t *cc, pos_t pos, cir_calc_op_t op, bool is_assign, cir_expr_t *lhs, cir_expr_t *rhs
) {
    c_type_t const *lhs_type = lhs->common.type_rc->data;
    c_type_t const *rhs_type = rhs->common.type_rc->data;
    bool            lhs_ptr  = lhs_type->primitive == C_COMP_POINTER;
    bool            rhs_ptr  = rhs_type->primitive == C_COMP_POINTER;

    if (lhs_ptr && rhs_ptr) {
        // Only `ptr - ptr` is valid here; the caller has already validated the operator.
        assert(op == CIR_CALC_SUB);
        assert(!is_assign);
        cir_expr_t *sub = cir_expr_create_calc(cir_calc_create(
            (cir_expr_common_t){
                .pos          = pos,
                .is_lvalue    = false,
                .allow_addrof = false,
                .type_rc      = rc_share(&cc->prim_rcs[cc->options.size_type]),
            },
            CIR_CALC_SUB,
            false,
            lhs,
            rhs
        ));
        return c_compile2_ptr_premul(cc, sub, lhs_type, true);
    }

    // `int + ptr`: canonicalize to `ptr + int`. Cannot happen for compound assignment.
    if (!lhs_ptr) {
        assert(op == CIR_CALC_ADD);
        assert(!is_assign);
        cir_expr_t *tmp = lhs;
        lhs             = rhs;
        rhs             = tmp;
        lhs_type        = lhs->common.type_rc->data;
    }

    // `ptr +/- int`: scale int by sizeof(*ptr), then add/sub.
    rhs = c_compile2_ptr_premul(cc, rhs, lhs_type, false);
    if (!rhs) {
        cir_expr_delete(lhs);
        return NULL;
    }
    return cir_expr_create_calc(cir_calc_create(
        (cir_expr_common_t){
            .pos          = pos,
            .is_lvalue    = false,
            .allow_addrof = false,
            .type_rc      = rc_share(lhs->common.type_rc),
        },
        op,
        is_assign,
        lhs,
        rhs
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

    // Plain assignment: type-check, then emit an assign node.
    if (expr->oper == C_TKN_ASSIGN) {
        if (!lhs->common.is_lvalue) {
            cctx_diagnostic(cc->cctx, expr->oper_pos, DIAG_ERR, "Left operand of = is not an lvalue");
            goto error;
        }
        rhs = c_compile2_promotion(cc, rhs, false);
        if (!c_type_is_castable(cc, lhs->common.type_rc->data, rhs->common.type_rc->data)) {
            cctx_diagnostic(cc->cctx, expr->oper_pos, DIAG_ERR, "Incompatible types in assignment");
            goto error;
        }
        // Insert an implicit cast if the RHS type differs.
        if (!c_type_is_identical(cc, lhs->common.type_rc->data, rhs->common.type_rc->data, false)) {
            rhs = cir_expr_create_cast(cir_cast_create(
                (cir_expr_common_t){
                    .pos          = rhs->common.pos,
                    .is_lvalue    = false,
                    .allow_addrof = false,
                    .type_rc      = rc_share(lhs->common.type_rc),
                },
                rhs
            ));
        }
        return cir_expr_create_assign(cir_assign_create(
            (cir_expr_common_t){
                .pos          = expr->pos,
                .is_lvalue    = false,
                .allow_addrof = false,
                .type_rc      = rc_share(lhs->common.type_rc),
            },
            lhs,
            rhs
        ));
    }

    cir_calc_op_t op;
    if (!tkn_to_calc_op(expr->oper, &op)) {
        fprintf(stderr, "BUG: Unhandled infix operator %s\n", c_token_name[expr->oper]);
        abort();
    }

    bool compound = is_compound_assign(expr->oper);
    if (compound && !lhs->common.is_lvalue) {
        cctx_diagnostic(
            cc->cctx, expr->oper_pos, DIAG_ERR, "Left operand of %s is not an lvalue", c_token_name[expr->oper]
        );
        goto error;
    }

    // Apply usual promotions / array decay. For compound assignment, leave the LHS as its
    // native lvalue type so the calc node can describe the load+store in one place.
    if (!compound) {
        lhs = c_compile2_promotion(cc, lhs, true);
    }
    rhs                  = c_compile2_promotion(cc, rhs, true);
    c_type_t const *ltyp = lhs->common.type_rc->data;
    c_type_t const *rtyp = rhs->common.type_rc->data;

    // Validate that the operand types are compatible with this operator.
    if (!c_type_arith_compatible(cc, ltyp, rtyp, expr->oper, expr->oper_pos)) {
        goto error;
    }

    cir_expr_t *result;
    if ((op == CIR_CALC_ADD || op == CIR_CALC_SUB)
        && (ltyp->primitive == C_COMP_POINTER || rtyp->primitive == C_COMP_POINTER)) {
        result = c_compile2_ptr_arith(cc, expr->pos, op, compound, lhs, rhs);
        lhs    = NULL;
        rhs    = NULL;
        if (!result) {
            goto error;
        }
    } else {
        result = c_compile2_arith_op(cc, expr->pos, op, compound, lhs, rhs);
        lhs    = NULL;
        rhs    = NULL;
    }
    return result;

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
    c_type_t const *type = val->common.type_rc->data;
    if (
        type->primitive >= C_PRIM_VOID && type->primitive != C_COMP_POINTER
        && expr->oper != C_TKN_AND // The addrof operator allows *any* type.
        && !(
            (expr->oper == C_TKN_INC || expr->oper == C_TKN_DEC) && type->primitive == C_COMP_POINTER
        ) // The inc/dec operators also allow pointer types.
        && !(
            (expr->oper == C_TKN_MUL || expr->oper == C_TKN_LNOT)
            && (type->primitive == C_COMP_ARRAY || type->primitive == C_COMP_POINTER)
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
    val  = c_compile2_promotion(cc, val, expr->oper != C_TKN_AND);
    type = val->common.type_rc->data;

    c_prim_t prim = type->primitive < C_N_PRIM ? type->primitive : cc->options.size_type;

    cir_expr_common_t common = {
        .pos          = expr->pos,
        .is_lvalue    = false,
        .allow_addrof = false,
        .type_rc      = rc_share(val->common.type_rc),
    };

    switch (expr->oper) {
        case C_TKN_LNOT: { // Logical NOT `!expr`
            cir_expr_t *zero = c_compile2_synth_iconst(cc, expr->pos, prim, int128(0, 0));
            return cir_expr_create_calc(cir_calc_create(common, CIR_CALC_EQ, false, val, zero));
        }
        case C_TKN_NOT: { // Bitwise NOT `~expr`
            cir_expr_t *mask = c_compile2_synth_iconst(cc, expr->pos, prim, int128(UINT64_MAX, UINT64_MAX));
            return cir_expr_create_calc(cir_calc_create(common, CIR_CALC_BXOR, false, val, mask));
        }
        case C_TKN_INC:   // Pre-increment `++expr`
        case C_TKN_DEC: { // Pre-decrement `--expr`
            int64_t inc = 1;
            if (type->primitive == C_COMP_POINTER) {
                uint64_t size, align;
                if (!c_type_get_size(cc, type, &size, &align)) {
                    cctx_diagnostic(
                        cc->cctx,
                        expr->pos,
                        DIAG_ERR,
                        "Cannot do pointer arithmetic with incomplete inner type"
                    );
                    goto err1;
                }
                inc = (int64_t)size;
            }
            if (expr->oper == C_TKN_DEC) {
                inc = -inc;
            }
            return cir_expr_create_inc(cir_inc_create(common, val, true, inc));
        }
        case C_TKN_ADD: // Passthru `+`.
            rc_delete(common.type_rc);
            return val;
        case C_TKN_SUB: { // Arithmetic negate `-`.
            cir_expr_t *zero = c_compile2_synth_iconst(cc, expr->pos, prim, int128(0, 0));
            return cir_expr_create_calc(cir_calc_create(common, CIR_CALC_SUB, false, zero, val));
        }
        case C_TKN_AND: { // Address-of `&`
            if (!val->common.allow_addrof) {
                cctx_diagnostic(cc->cctx, expr->oper_pos, DIAG_ERR, "Cannot take the address of this rvalue");
                goto err1;
            }
            if (type->primitive == C_COMP_FUNCTION) {
                // Similar to their funny deref semantics, doing addrof on a function is a no-op.
                // The function instead decays implicitly into a function pointer as needed.
                return val;
            }
            rc_delete(common.type_rc);
            common.type_rc = c_type_to_pointer(cc, rc_share(val->common.type_rc));
            return cir_expr_create_deref(cir_deref_create(common, val));
        }
        case C_TKN_MUL: { // Dereference `*expr`
            if (type->primitive != C_COMP_POINTER) {
                goto err1;
            }
            c_type_t const *inner = type->inner->data;
            if (inner->primitive == C_COMP_FUNCTION) {
                // Funcptr types have funny semantics that mean deref doesn't actually do anything.
                return val;
            }
            rc_delete(common.type_rc);
            common.type_rc = rc_share(type->inner);
            return cir_expr_create_deref(cir_deref_create(common, val));
        }
        default: fprintf(stderr, "BUG: Unhandled prefix operator\n"); abort();
    }

err1:
    rc_delete(common.type_rc);
err0:
    cir_expr_delete(val);
    return NULL;
}

// Compile a suffix expression.
cir_expr_t *c_compile2_expr_suffix(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_suffix_t const *expr) {
    fprintf(stderr, "TODO: c_compile2_expr_suffix\n");
    abort();
}

// Compile a cast expression.
cir_expr_t *c_compile2_expr_cast(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_cast_t const *cast) {
    fprintf(stderr, "TODO: c_compile2_expr_cast\n");
    abort();
}

// Compile a call expression.
cir_expr_t *c_compile2_expr_call(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_call_t const *call) {
    fprintf(stderr, "TODO: c_compile2_expr_call\n");
    abort();
}

// Compile an identifier as part of an expression.
cir_expr_t *c_compile2_expr_ident(c_compiler_t *cc, cir_scope_t *scope, c_ast_ident_t const *ident) {
    fprintf(stderr, "TODO: c_compile2_expr_ident\n");
    abort();
}

// Compile an integer constant as part of an expression.
cir_expr_t *c_compile2_expr_iconst(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_iconst_t const *iconst) {
    fprintf(stderr, "TODO: c_compile2_expr_iconst\n");
    abort();
}

// Compile a string constant as part of an expression.
cir_expr_t *c_compile2_expr_sconst(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_sconst_t const *sconst) {
    fprintf(stderr, "TODO: c_compile2_expr_sconst\n");
    abort();
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
            .type_rc      = rc_share(&cc->prim_rcs[C_PRIM_VOID]),
        };
    }

    return cir_expr_create_exprs(cir_exprs_create(common, out));
}


// Compile a compound literal/initializer given a known target type.
cir_value_t *c_compile2_compinit(c_compiler_t *cc, cir_scope_t *scope, c_ast_init_list_t const *init) {
    fprintf(stderr, "TODO: c_compile2_compinit\n");
    abort();
}


// Performs type promotions and array decay on an expression.
cir_expr_t *c_compile2_promotion(c_compiler_t *cc, cir_expr_t *value, bool promote_to_int) {
    c_type_t const *type = value->common.type_rc->data;

    if (promote_to_int && type->primitive != C_PRIM_VOID && type->primitive < C_PRIM_SINT) {
        rc_t prim_rc = rc_share(&cc->prim_rcs[C_PRIM_SINT]);
        return cir_expr_create_cast(cir_cast_create(
            (cir_expr_common_t){
                .pos          = value->common.pos,
                .allow_addrof = false,
                .is_lvalue    = false,
                .type_rc      = prim_rc,
            },
            value
        ));
    }

    if (type->primitive != C_COMP_ARRAY) {
        return value;
    }

    pos_t pos        = value->common.pos;
    rc_t  arr_ptr_rc = c_type_to_pointer(cc, rc_share(value->common.type_rc));
    rc_t  ptr_rc     = c_type_to_pointer(cc, rc_share(type->inner));

    cir_expr_t *addrof = cir_expr_create_addrof(cir_addrof_create(
        (cir_expr_common_t){
            .pos          = pos,
            .is_lvalue    = false,
            .allow_addrof = false,
            .type_rc      = arr_ptr_rc,
        },
        value
    ));

    return cir_expr_create_cast(cir_cast_create(
        (cir_expr_common_t){
            .pos          = pos,
            .allow_addrof = false,
            .is_lvalue    = false,
            .type_rc      = ptr_rc,
        },
        addrof
    ));
}

// Multiply/divide a value by the size of the inner type of a given pointer type.
// Compile error if the inner type of the pointer is an incomplete type.
// Needed because pointer arithmetic in the C IR does not respect the inner type's size.
cir_expr_t *c_compile2_ptr_premul(c_compiler_t *cc, cir_expr_t *value, c_type_t const *ptr_type, bool is_division) {
    assert(ptr_type->primitive == C_COMP_POINTER || ptr_type->primitive == C_COMP_ARRAY);
    uint64_t size, align;
    if (!c_type_get_size(cc, ptr_type->inner->data, &size, &align)) {
        cctx_diagnostic(
            cc->cctx,
            value->common.pos,
            DIAG_ERR,
            "Cannot do pointer arithmetic with incomplete inner type"
        );
        return NULL;
    }

    cir_expr_t *iconst = c_compile2_synth_iconst(cc, value->common.pos, cc->options.size_type, int128(0, size));

    return cir_expr_create_calc(cir_calc_create(
        (cir_expr_common_t){
            .pos          = value->common.pos,
            .is_lvalue    = false,
            .allow_addrof = false,
            .type_rc      = rc_share(value->common.type_rc),
        },
        is_division ? CIR_CALC_DIV : CIR_CALC_MUL,
        false,
        value,
        iconst
    ));
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
            .type_rc      = rc_share(&cc->prim_rcs[prim]),
        },
        cir_const_create(pos, prim, ir_const)
    );

    return cir_expr_create_value(cir_value);
}
