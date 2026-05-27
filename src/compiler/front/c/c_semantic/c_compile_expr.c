
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
    cir_expr_t *lhs = NULL;
    cir_expr_t *rhs = NULL;
    lhs             = c_compile2_expr(cc, scope, expr->lhs);
    lhs             = c_compile2_expr(cc, scope, expr->rhs);
    if (!lhs || !rhs) {
        goto error;
    }

    // Exactly one operand must be of pointer type.
    if (c_type_is_pointer(lhs->common.type_rc->data) == c_type_is_pointer(rhs->common.type_rc->data)) {
        cctx_diagnostic(
            cc->cctx,
            expr->pos,
            DIAG_ERR,
            "Exactly one operand to index expression must be of pointer type"
        );
        goto error;
    }

    rc_t ptr_rc;
    if (c_type_is_pointer(lhs->common.type_rc->data)) {
        ptr_rc = rc_share(lhs->common.type_rc);
    } else {
        ptr_rc = rc_share(rhs->common.type_rc);
    }
    c_type_t const *ptr_type   = ptr_rc->data;
    rc_t            inner_rc   = rc_share(ptr_type->inner);
    c_type_t const *inner_type = inner_rc->data;

    // Assert inner type is valid for dereference.
    uint64_t size, align;
    if (!c_type_get_size(cc, inner_type, &size, &align)) {
        rc_delete(inner_rc);
        rc_delete(ptr_rc);
        cctx_diagnostic(cc->cctx, expr->pos, DIAG_ERR, "Subscript of pointer with incomplete type");
        goto error;
    }

    // Pure pointer type without qualifiers, also converts array to pointer.
    rc_t pure_ptr_rc = c_type_to_pointer(cc, rc_share(inner_rc));
    rc_delete(ptr_rc);

    // Valid expression, emit CIR.
    cir_expr_t *add   = cir_expr_create_calc(cir_calc_create(
        (cir_expr_common_t){
            .pos          = expr->pos,
            .type_rc      = pure_ptr_rc,
            .is_lvalue    = false,
            .allow_addrof = false,
        },
        CIR_CALC_ADD,
        lhs,
        rhs
    ));
    cir_expr_t *deref = cir_expr_create_deref(cir_deref_create(
        (cir_expr_common_t){
            .pos          = expr->pos,
            .type_rc      = inner_rc,
            .is_lvalue    = true,
            .allow_addrof = true,
        },
        add
    ));
    return deref;

error:
    if (lhs) {
        cir_expr_delete(lhs);
    }
    if (rhs) {
        cir_expr_delete(rhs);
    }
    return NULL;
}

// Compile an infix expression.
cir_expr_t *c_compile2_expr_infix(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_infix_t const *expr) {
    fprintf(stderr, "TODO: c_compile2_expr_infix\n");
    abort();
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
            return cir_expr_create_calc(cir_calc_create(common, CIR_CALC_EQ, val, zero));
        }
        case C_TKN_NOT: { // Bitwise NOT `~expr`
            cir_expr_t *mask = c_compile2_synth_iconst(cc, expr->pos, prim, int128(UINT64_MAX, UINT64_MAX));
            return cir_expr_create_calc(cir_calc_create(common, CIR_CALC_BXOR, val, mask));
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
            return cir_expr_create_calc(cir_calc_create(common, CIR_CALC_SUB, zero, val));
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
