
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "codegen.h"

#include "arith128.h"
#include "backend.h"
#include "ir.h"
#include "ir_interpreter.h"
#include "ir_optimizer.h"
#include "ir_serialization.h"
#include "ir_types.h"
#include "list.h"
#include "regalloc.h"
#include "set.h"
#include "unreachable.h"
#include "vec.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



// Remove jumps that go the the next code block linearly.
static void cg_remove_jumps(ir_func_t *func) {
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        ir_insn_t *last_insn = (ir_insn_t *)code->insns.tail;
        if (last_insn && last_insn->type == IR_INSN_JUMP
            && last_insn->operands[0].mem.base_code == (void *)code->node.next) {
            ir_insn_delete((ir_insn_t *)last_insn);
        }
    }
}

static inline bool can_isel(ir_insn_t *insn) {
    switch (insn->type) {
        case IR_INSN_EXPR2:
        case IR_INSN_EXPR1:
        case IR_INSN_JUMP:
        case IR_INSN_BRANCH:
        case IR_INSN_LEA:
        case IR_INSN_LOAD:
        case IR_INSN_STORE:
        case IR_INSN_ALLOCA: return true;
        case IR_INSN_CALL:
        case IR_INSN_RETURN:
        case IR_INSN_MEMCPY:
        case IR_INSN_MEMSET: UNREACHABLE(); // Should already be eliminated.
        case IR_INSN_CLOBBER:
        case IR_INSN_MACHINE:
        case IR_INSN_COMBINATOR:
        case IR_INSN_CALLFRAME_ENTER:
        case IR_INSN_CALLFRAME_EXIT:
        case IR_INSN_MARK_USED:
        case IR_INSN_EPILOGUE: return false;
    }
    UNREACHABLE();
}

// Select machine instructions for all IR instructions.
static void cg_isel(backend_profile_t *profile, ir_code_t *code) {
    ir_insn_t *cur = container_of(code->insns.tail, ir_insn_t, node);
    while (cur) {
        if (can_isel(cur)) {
            ir_insn_t *res = profile->backend->isel(profile, cur);
            if (!res) {
                fprintf(stderr, "BUG: Backend cannot select an instruction for `");
                ir_insn_serialize(cur, profile, stderr);
                fprintf(stderr, "`\n");
                set_t vars = PTR_SET_EMPTY;
                for (size_t i = 0; i < cur->returns_len; i++) {
                    set_add(&vars, cur->returns[i].dest_var);
                }
                for (size_t i = 0; i < cur->operands_len; i++) {
                    if (cur->operands[i].type == IR_OPERAND_TYPE_VAR) {
                        set_add(&vars, cur->operands[i].var);
                    }
                }
                set_foreach(ir_var_t, var, &vars) {
                    fprintf(stderr, "Note: %%%s is %s\n", var->name, ir_prim_name[var->prim_type]);
                }
                set_clear(&vars);
                fflush(stderr);
                abort();
            }
            cur = res;
        }
        cur = container_of(cur->node.prev, ir_insn_t, node);
    }
}

// Expand the ABI definitions of calls and returns.
static void cg_xabi(backend_profile_t *profile, ir_code_t *code) {
    assert(code->func->enforce_ssa);
    code->func->enforce_ssa = false;
    ir_insn_t *cur          = container_of(code->insns.tail, ir_insn_t, node);
    while (cur) {
        if (cur->type == IR_INSN_RETURN || cur->type == IR_INSN_CALL) {
            ir_insn_t *res;
            if (cur->type == IR_INSN_RETURN) {
                res = profile->backend->xabi_return(profile, cur);
            } else {
                res = profile->backend->xabi_call(profile, cur);
            }
            if (!res) {
                fprintf(stderr, "BUG: Backend cannot implement the ABI for `");
                ir_insn_serialize(cur, profile, stderr);
                fprintf(stderr, "`\n");
                set_t vars = PTR_SET_EMPTY;
                for (size_t i = 0; i < cur->returns_len; i++) {
                    set_add(&vars, cur->returns[i].dest_var);
                }
                for (size_t i = 0; i < cur->operands_len; i++) {
                    if (cur->operands[i].type == IR_OPERAND_TYPE_VAR) {
                        set_add(&vars, cur->operands[i].var);
                    }
                }
                set_foreach(ir_var_t, var, &vars) {
                    fprintf(stderr, "Note: %%%s is %s\n", var->name, ir_prim_name[var->prim_type]);
                }
                set_clear(&vars);
                fflush(stderr);
                abort();
            }
            cur = res;
        }
        cur = container_of(cur->node.prev, ir_insn_t, node);
    }
    code->func->enforce_ssa = true;
}

// Replace arithmetic that is not supported with function calls.
static void cg_functionize_expr2(backend_profile_t *profile, ir_insn_t *insn) {
    ir_func_t *func = insn->code->func;
    assert(insn->type == IR_INSN_EXPR2);
    assert(insn->returns_len == 1);
    assert(insn->operands_len == 2);
    ir_prim_t const prim = insn->returns[0].dest_var->prim_type;
    bool softfloat       = (prim == IR_PRIM_f32 && !profile->has_f32) || (prim == IR_PRIM_f64 && !profile->has_f64);

    func->enforce_ssa = false;
    if (insn->op2 == IR_OP2_add && softfloat) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_add_%s", ir_prim_name[prim]);
        ir_add_call(
            IR_AFTER_INSN(insn),
            IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM(buf)),
            true,
            insn->returns[0],
            2,
            insn->operands
        );
        ir_insn_delete(insn);
    } else if (insn->op2 == IR_OP2_sub && softfloat) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_sub_%s", ir_prim_name[prim]);
        ir_add_call(
            IR_AFTER_INSN(insn),
            IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM(buf)),
            true,
            insn->returns[0],
            2,
            insn->operands
        );
        ir_insn_delete(insn);
    } else if (insn->op2 == IR_OP2_mul && (softfloat || !profile->has_mul)) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_mul_%s", ir_prim_name[prim]);
        ir_add_call(
            IR_AFTER_INSN(insn),
            IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM(buf)),
            true,
            insn->returns[0],
            2,
            insn->operands
        );
        ir_insn_delete(insn);
    } else if (insn->op2 == IR_OP2_divu && (softfloat || !profile->has_div)) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_divu_%s", ir_prim_name[prim]);
        ir_add_call(
            IR_AFTER_INSN(insn),
            IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM(buf)),
            true,
            insn->returns[0],
            2,
            insn->operands
        );
        ir_insn_delete(insn);
    } else if (insn->op2 == IR_OP2_remu && (softfloat || !profile->has_rem)) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_remu_%s", ir_prim_name[prim]);
        ir_add_call(
            IR_AFTER_INSN(insn),
            IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM(buf)),
            true,
            insn->returns[0],
            2,
            insn->operands
        );
        ir_insn_delete(insn);
    } else if (insn->op2 == IR_OP2_divs && (softfloat || !profile->has_div)) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_divs_%s", ir_prim_name[prim]);
        ir_add_call(
            IR_AFTER_INSN(insn),
            IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM(buf)),
            true,
            insn->returns[0],
            2,
            insn->operands
        );
        ir_insn_delete(insn);
    } else if (insn->op2 == IR_OP2_rems && (softfloat || !profile->has_rem)) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_rems_%s", ir_prim_name[prim]);
        ir_add_call(
            IR_AFTER_INSN(insn),
            IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM(buf)),
            true,
            insn->returns[0],
            2,
            insn->operands
        );
        ir_insn_delete(insn);
    } else if (insn->op2 == IR_OP2_shru && insn->operands[1].type != IR_OPERAND_TYPE_CONST && !profile->has_var_shift) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_shru_%s", ir_prim_name[prim]);
        ir_var_t *tmp = ir_var_create(insn->code->func, IR_PRIM_i8, NULL); // __lily_shr_* uses u8 as shift amount
        ir_add_expr1(IR_AFTER_INSN(insn), IR_RETVAL_VAR(tmp), IR_OP1_mov, insn->operands[1]);
        ir_add_call(
            IR_AFTER_INSN(insn),
            IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM(buf)),
            true,
            insn->returns[0],
            2,
            (ir_operand_t const[]){insn->operands[0], IR_OPERAND_VAR(tmp)}
        );
        ir_insn_delete(insn);
    } else if (insn->op2 == IR_OP2_shrs && insn->operands[1].type != IR_OPERAND_TYPE_CONST && !profile->has_var_shift) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_shrs_%s", ir_prim_name[prim]);
        ir_var_t *tmp = ir_var_create(insn->code->func, IR_PRIM_i8, NULL); // __lily_shr_* uses u8 as shift amount
        ir_add_expr1(IR_AFTER_INSN(insn), IR_RETVAL_VAR(tmp), IR_OP1_mov, insn->operands[1]);
        ir_add_call(
            IR_AFTER_INSN(insn),
            IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM(buf)),
            true,
            insn->returns[0],
            2,
            (ir_operand_t const[]){insn->operands[0], IR_OPERAND_VAR(tmp)}
        );
        ir_insn_delete(insn);
    } else if (insn->op2 == IR_OP2_shl && insn->operands[1].type != IR_OPERAND_TYPE_CONST && !profile->has_var_shift) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_shl_%s", ir_prim_name[prim]);
        ir_var_t *tmp = ir_var_create(insn->code->func, IR_PRIM_i8, NULL); // __lily_shl_u* uses u8 as shift amount
        ir_add_expr1(IR_AFTER_INSN(insn), IR_RETVAL_VAR(tmp), IR_OP1_mov, insn->operands[1]);
        ir_add_call(
            IR_AFTER_INSN(insn),
            IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM(buf)),
            true,
            insn->returns[0],
            2,
            (ir_operand_t const[]){insn->operands[0], IR_OPERAND_VAR(tmp)}
        );
        ir_insn_delete(insn);
    }
    // TODO: Float comparisons.
    func->enforce_ssa = true;
}

// Replace arithmetic that is not supported with function calls.
static void cg_functionize_expr1(backend_profile_t *profile, ir_insn_t *insn) {
    ir_func_t *func = insn->code->func;
    assert(insn->type == IR_INSN_EXPR1);
    assert(insn->returns_len == 1);
    assert(insn->operands_len == 1);
    ir_prim_t const prim     = ir_operand_prim(insn->operands[0]);
    ir_prim_t const ret_prim = insn->returns[0].dest_var->prim_type;
    bool            softfloat
        = ((prim == IR_PRIM_f32 && !profile->has_f32) || (prim == IR_PRIM_f64 && !profile->has_f64))
          || ((ret_prim == IR_PRIM_f32 && !profile->has_f32) || (ret_prim == IR_PRIM_f64 && !profile->has_f64));

    func->enforce_ssa = false;
    if (insn->op1 == IR_OP1_mov && ret_prim != prim && softfloat) {
        char buf[32];
        if (ret_prim == IR_PRIM_f64 && prim == IR_PRIM_f32) {
            // Float to float.
            strcpy(buf, "__lily_fconv_f64");
        } else if (ret_prim == IR_PRIM_f32 && prim == IR_PRIM_f64) {
            // Float to float.
            strcpy(buf, "__lily_fconv_f32");
        } else if (ret_prim == IR_PRIM_f32 || ret_prim == IR_PRIM_f64) {
            // Int to float.
            snprintf(buf, sizeof(buf) - 1, "__lily_itof_%s_%s", ir_prim_name[ret_prim], ir_prim_name[prim]);
        } else {
            // Float to int.
            snprintf(buf, sizeof(buf) - 1, "__lily_ftoi_%s_%s", ir_prim_name[prim], ir_prim_name[ret_prim]);
        }
        ir_add_call(
            IR_AFTER_INSN(insn),
            IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM(buf)),
            true,
            insn->returns[0],
            1,
            insn->operands
        );
        ir_insn_delete(insn);
    } else if (insn->op1 == IR_OP1_neg && softfloat) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_neg_%s", ir_prim_name[prim]);
        ir_add_call(
            IR_AFTER_INSN(insn),
            IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM(buf)),
            true,
            insn->returns[0],
            1,
            insn->operands
        );
        ir_insn_delete(insn);
    }
    // TODO: Float comparisons.
    func->enforce_ssa = true;
}

// Replace arithmetic that is not supported with function calls.
static void cg_functionize_exprs(backend_profile_t *profile, ir_insn_t *insn) {
    assert(insn->code->func->enforce_ssa);
    // TODO: Implement for future abs/sqrt/clz/ctz/popcnt IR op1.

    if (insn->type == IR_INSN_EXPR2) {
        cg_functionize_expr2(profile, insn);
    } else if (insn->type == IR_INSN_EXPR1) {
        cg_functionize_expr1(profile, insn);
    }
}

// Normalize operand order of instructions, if possible.
static void cg_normalize_op_order(ir_insn_t *insn) {
    // This only deals with expr2 type instructions.
    if (insn->type != IR_INSN_EXPR2) {
        return;
    }

    // Convert subtraction of a constant into addition of the negative of that constant.
    if (insn->op2 == IR_OP2_sub && insn->operands[1].type == IR_OPERAND_TYPE_CONST) {
        insn->operands[1].iconst = ir_calc1(IR_OP1_neg, insn->operands[1].iconst);
        insn->op2                = IR_OP2_add;
        // No need to commute since the second operand is not a reg operand.
        return;
    }

    // Determine whether it's possible to commute arguments; return if not.
    ir_op2_type_t commuted_op2 = insn->op2;
    switch (insn->op2) {
        // Commutable with replacement operator.
        case IR_OP2_sgtu: commuted_op2 = IR_OP2_sltu; break;
        case IR_OP2_sleu: commuted_op2 = IR_OP2_sgeu; break;
        case IR_OP2_sltu: commuted_op2 = IR_OP2_sgtu; break;
        case IR_OP2_sgeu: commuted_op2 = IR_OP2_sleu; break;
        case IR_OP2_sgts: commuted_op2 = IR_OP2_slts; break;
        case IR_OP2_sles: commuted_op2 = IR_OP2_sges; break;
        case IR_OP2_slts: commuted_op2 = IR_OP2_sgts; break;
        case IR_OP2_sges: commuted_op2 = IR_OP2_sles; break;

        // Commutable with same operator.
        case IR_OP2_seq:
        case IR_OP2_sne:
        case IR_OP2_band:
        case IR_OP2_bor:
        case IR_OP2_bxor:
        case IR_OP2_add:
        case IR_OP2_mul: break;

        // Not commutable.
        default: return;
    }

    // Commute the register to be the first operand if the other operand is not a register.
    if (insn->operands[0].type != IR_OPERAND_TYPE_VAR && insn->operands[1].type == IR_OPERAND_TYPE_VAR) {
        ir_operand_t tmp  = insn->operands[0];
        insn->operands[0] = insn->operands[1];
        insn->operands[1] = tmp;
        insn->op2         = commuted_op2;
    }
}

// Expand combinators back into separate assignments from predecessor code blocks.
static void cg_expand_comb(ir_insn_t *insn) {
    if (insn->type != IR_INSN_COMBINATOR) {
        return;
    }
    assert(insn->returns[0].type == IR_RETVAL_TYPE_VAR);

    ir_var_t *tmp = ir_var_create(insn->code->func, insn->returns[0].dest_var->prim_type, NULL);
    for (size_t i = 0; i < insn->combinators_len; i++) {
        ir_code_t *pred = insn->combinators[i].pred;

        // Find the last place in the predecessor before any control flow instructions.
        ir_insnloc_t loc = IR_PREPEND(pred);
        dlist_foreach_node_rev(ir_insn_t, insn, &pred->insns) {
            if (insn->type != IR_INSN_JUMP && insn->type != IR_INSN_BRANCH) {
                loc = IR_AFTER_INSN(insn);
                break;
            }
        }

        ir_add_expr1(loc, IR_RETVAL_VAR(tmp), IR_OP1_mov, insn->combinators[i].bind);
    }

    ir_add_expr1(IR_BEFORE_INSN(insn), insn->returns[0], IR_OP1_mov, IR_OPERAND_VAR(tmp));
    ir_insn_delete(insn);
}

// Promote IR variable to at least as wide as the minimum register size.
static void cg_promote_var_0(backend_profile_t *profile, ir_var_t *var) {
    var->orig_prim_type = var->prim_type;
    ir_prim_t min_uint  = (ir_prim_t)(profile->arith_min_bits - LILY_8_BITS) + IR_PRIM_i8;
    if (var->prim_type == IR_PRIM_bool) {
        var->prim_type = min_uint;
    } else if (!ir_prim_is_integer(var->prim_type)) {
        return;
    } else {
        if (var->prim_type < min_uint) {
            var->prim_type = min_uint;
        }
    }
}

// Tracks sign/zero-extension information for `cg_promote_var_1`.
typedef struct {
    ir_insn_t *insn;
    size_t     index;
    bool       use_sign_ext; // As opposed to zero-extension.
} sign_tag_t;
VEC_TYPE_DEF(vec_sign_tag_t, sign_tag_t);

// Tracks sign/zero-extension information for `cg_promote_var_1`.
typedef struct {
    vec_sign_tag_t operand;
    ir_var_t      *target;
    bool           need_sign_ext;
    bool           need_zero_ext;
} sign_tag_list_t;

// Insert one entry into the sign tag list for `cg_promote_var_1`.
static void cg_add_sign_tag(sign_tag_list_t *list, ir_insn_t *insn, size_t index, ir_sign_t sign) {
    if (insn->operands[index].type != IR_OPERAND_TYPE_VAR || insn->operands[index].var != list->target
        || sign == IR_SIGN_AGNOSTIC) {
        return;
    }
    sign_tag_t tag = {
        .insn         = insn,
        .index        = index,
        .use_sign_ext = sign == IR_SIGN_SIGNEXT,
    };
    vec_push(&list->operand, tag);
    if (sign == IR_SIGN_SIGNEXT) {
        list->need_sign_ext = true;
    } else {
        list->need_zero_ext = true;
    }
}

// Insert additional computations as needded to clamp promoted variable back in range.
static void cg_promote_var_1(backend_profile_t *profile, ir_var_t *orig) {
    ir_insn_t *assignment = set_next(&orig->assigned_at, NULL)->value;
    assert(ir_prim_is_integer(orig->prim_type));

    sign_tag_list_t tags = {
        .operand       = VEC_INIT,
        .target        = orig,
        .need_sign_ext = false,
        .need_zero_ext = false,
    };

    // For every instruction operand that uses this value, determine whether it needs any widening and what kind.
    set_foreach(ir_insn_t, insn, &orig->used_at) {
#define tag(index, sign) cg_add_sign_tag(&tags, insn, index, sign)

        if (insn->flags & IR_INSN_FLAG_CLAMPING) {
            continue;
        }
        switch (insn->type) {
            case IR_INSN_EXPR2: // Depends on operation.
                switch (insn->op2) {
                    case IR_OP2_seq:  // Widening type must be consistent across all equality comparisons;
                    case IR_OP2_sne:  // choose unsigned widening because it is generally cheaper.
                    case IR_OP2_sgtu: // Unsigned operations.
                    case IR_OP2_sleu:
                    case IR_OP2_sltu:
                    case IR_OP2_sgeu:
                    case IR_OP2_divu:
                    case IR_OP2_remu:
                        tag(0, IR_SIGN_ZEROEXT);
                        tag(1, IR_SIGN_ZEROEXT);
                        break;
                    case IR_OP2_sgts: // Signed operations.
                    case IR_OP2_sles:
                    case IR_OP2_slts:
                    case IR_OP2_sges:
                    case IR_OP2_divs:
                    case IR_OP2_rems:
                        tag(0, IR_SIGN_SIGNEXT);
                        tag(1, IR_SIGN_SIGNEXT);
                        break;
                    case IR_OP2_shru: tag(0, IR_SIGN_ZEROEXT); break; // RHS being out of range is UB so not widened.
                    case IR_OP2_shrs: tag(0, IR_SIGN_SIGNEXT); break;
                    case IR_OP2_add: // The rest are MSB-agnostic arithmetic operations.
                    case IR_OP2_sub:
                    case IR_OP2_mul:
                    case IR_OP2_shl:
                    case IR_OP2_band:
                    case IR_OP2_bor:
                    case IR_OP2_bxor: break;
                    case IR_N_OP2: UNREACHABLE();
                }
                break;
            case IR_INSN_EXPR1: // Depends on operation.
                switch (insn->op1) {
                    case IR_OP1_bitcast: // Bit-cast assumes correct bit pattern; cut off the top bits.
                    case IR_OP1_seqz:    // Comparison against zero needs widening; zero-extension considered cheaper.
                    case IR_OP1_snez: tag(0, IR_SIGN_ZEROEXT); break;
                    case IR_OP1_mov:     // Literal copy.
                    case IR_OP1_zeroext: // Explicitly widening instructions.
                    case IR_OP1_signext:
                    case IR_OP1_trunc: // Truncation.
                    case IR_OP1_neg:   // MSB-agnostic arithmetic.
                    case IR_OP1_bneg: break;
                    case IR_N_OP1: UNREACHABLE();
                }
                break;
            case IR_INSN_CALL: // Depends per operand on sign tag.
                for (size_t i = 0; i < insn->operands_len; i++) {
                    tag(i, insn->operands[i].sign);
                }
                break;
            case IR_INSN_RETURN: // Depends on function sign tag.
                if (insn->returns_len) {
                    tag(0, orig->func->rettype.sign);
                }
                break;
            case IR_INSN_LOAD: // Agnostic.
            case IR_INSN_STORE:
            case IR_INSN_COMBINATOR: break;
            case IR_INSN_LEA: // Would be UB due to using arguments that aren't pointer-sized.
            case IR_INSN_MEMCPY:
            case IR_INSN_MEMSET:
            case IR_INSN_ALLOCA:
            case IR_INSN_CALLFRAME_ENTER: // Impossible; do not have (integer) operands.
            case IR_INSN_CALLFRAME_EXIT:
            case IR_INSN_JUMP:
            case IR_INSN_BRANCH:
            case IR_INSN_MACHINE: // These instructions should not exist at this stage.
            case IR_INSN_CLOBBER:
            case IR_INSN_MARK_USED:
            case IR_INSN_EPILOGUE: UNREACHABLE();
        }

#undef tag
    }

    ir_var_t *sign_extended = NULL;
    ir_var_t *zero_extended = NULL;
    if (tags.need_sign_ext) {
        sign_extended = ir_var_create(orig->func, orig->prim_type, NULL);
        ir_add_expr1(IR_AFTER_INSN(assignment), IR_RETVAL_VAR(sign_extended), IR_OP1_signext, IR_OPERAND_VAR(orig));
    }
    if (tags.need_zero_ext) {
        zero_extended = ir_var_create(orig->func, orig->prim_type, NULL);
        ir_add_expr1(IR_AFTER_INSN(assignment), IR_RETVAL_VAR(zero_extended), IR_OP1_zeroext, IR_OPERAND_VAR(orig));
    }

    for (size_t i = 0; i < tags.operand.len; i++) {
        sign_tag_t const tag = tags.operand.arr[i];
        ir_insn_set_operand(tag.insn, tag.index, IR_OPERAND_VAR(tag.use_sign_ext ? sign_extended : zero_extended));
    }
    vec_clear(&tags.operand);
}

static void cg_alloc_one_frame(ir_func_t *func, ir_frame_t *frame) {
    if (func->stack_footprint % frame->align) {
        func->stack_footprint += frame->align - func->stack_footprint % frame->align;
    }
    func->stack_footprint += frame->size;
    frame->offset          = func->stack_footprint;
}

// Rather naive stack frame allocator; sort most-aligned frame first, call frame always at the end.
static void cg_alloc_frames(backend_profile_t *profile, ir_func_t *func) {
    if (func->link_frame) {
        cg_alloc_one_frame(func, func->link_frame);
    }
    dlist_foreach_node(ir_frame_t, frame, &func->frames_list) {
        if (frame != func->call_frame && frame != func->link_frame) {
            cg_alloc_one_frame(func, frame); // TODO: coalesce frames not live at the same time.
        }
    }
    if (func->call_frame) {
        cg_alloc_one_frame(func, func->call_frame);
    }

    // Ensure function's stack usage preserves alignment.
    if (func->stack_footprint % profile->stack_align) {
        func->stack_footprint += profile->stack_align - (func->stack_footprint % profile->stack_align);
    }
}

// Convert an SSA-form IR function completely into executable machine code.
// All IR instructions are replaced, code order is decided by potentially re-ordering the code blocks from the
// functions, and unnecessary jumps are removed. When finished, the code blocks and instructions therein will be in
// order as written to the eventual executable file.
void codegen(backend_profile_t *profile, ir_func_t *func) {
    ir_func_to_ssa(func);
    bool changed = true;
    while (changed) {
        changed  = opt_dead_code(func);
        changed |= opt_unused_vars(func);
    }

    // Replace arithmetic that is not supported with function calls.
    func->is_leaf = true; // We also check whether it is a leaf function while we're iterating all instructions.
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        ir_insn_t *insn = (ir_insn_t *)code->insns.head;
        while (insn) {
            ir_insn_t *next = (ir_insn_t *)insn->node.next;
            if (insn->type == IR_INSN_CALL) {
                func->is_leaf = false;
            } else {
                cg_functionize_exprs(profile, insn);
            }
            insn = next;
        }
    }
    func->use_frame_ptr = !func->is_leaf;

    // TODO: Deduplication of return code.

    // Expand the ABI definition.
    assert(func->enforce_ssa);
    func->enforce_ssa = false;
    profile->backend->xabi_entry(profile, func);
    func->enforce_ssa = true;
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        cg_xabi(profile, code);
    }

    // Remove jumps that go the the next code block linearly.
    cg_remove_jumps(func);

    // TODO: Convert operations into ones which fit in the CPU's registers.

    // Normalize operand order of instructions, if possible.
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        dlist_foreach_node(ir_insn_t, insn, &code->insns) {
            cg_normalize_op_order(insn);
        }
    }

    // Promote IR variables smaller than minimum register size to be at least as wide.
    func->enforce_cmp_bool = false;
    dlist_foreach_node(ir_var_t, var, &func->vars_list) {
        cg_promote_var_0(profile, var);
    }
    ir_calc_all_ranges(func);
    dlist_foreach_node(ir_var_t, var, &func->vars_list) {
        cg_promote_var_1(profile, var);
    }

    // Convert from strict SSA to non-strict SSA by converting combinators into moves.
    func->enforce_ssa = false;
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        ir_insn_t *insn = (ir_insn_t *)code->insns.head;
        while (insn) {
            ir_insn_t *next = (ir_insn_t *)insn->node.next;
            cg_expand_comb(insn);
            insn = next;
        }
    }

    // Select machine instructions.
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        cg_isel(profile, code);
    }

    regalloc(profile, func);

    if (profile->backend->postreg_pass) {
        profile->backend->postreg_pass(profile, func);
    }

    opt_unused_frames(func);
    cg_alloc_frames(profile, func);

    if (profile->backend->postproc_pass) {
        profile->backend->postproc_pass(profile, func);
    }

    // Strip meta-instructions.
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        ir_insn_t *insn = (ir_insn_t *)code->insns.head;
        while (insn) {
            ir_insn_t *next = (ir_insn_t *)insn->node.next;
            switch (insn->type) {
                case IR_INSN_MACHINE: break;
                case IR_INSN_CLOBBER:
                case IR_INSN_ALLOCA:
                case IR_INSN_CALLFRAME_ENTER:
                case IR_INSN_CALLFRAME_EXIT:
                case IR_INSN_MARK_USED:
                case IR_INSN_EPILOGUE: ir_insn_delete(insn); break;
                default: UNREACHABLE();
            }
            insn = next;
        }
    }
}
