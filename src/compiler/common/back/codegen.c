
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "codegen.h"

#include "arith128.h"
#include "backend.h"
#include "ir.h"
#include "ir/ir_serialization.h"
#include "ir_interpreter.h"
#include "ir_optimizer.h"
#include "ir_types.h"
#include "list.h"
#include "regalloc.h"
#include "set.h"
#include "unreachable.h"

#include <assert.h>
#include <stdio.h>
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

// Select machine instructions for all IR instructions.
static void cg_isel(backend_profile_t *profile, ir_code_t *code) {
    ir_insn_t *cur = container_of(code->insns.tail, ir_insn_t, node);
    while (cur) {
        if (cur->type != IR_INSN_MACHINE && cur->type != IR_INSN_COMBINATOR && cur->type != IR_INSN_CLOBBER) {
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
                    fprintf(stderr, "Note: %%%s is %s\n", var->name, ir_prim_names[var->prim_type]);
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
                    fprintf(stderr, "Note: %%%s is %s\n", var->name, ir_prim_names[var->prim_type]);
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
        snprintf(buf, sizeof(buf) - 1, "__lily_add_%s", ir_prim_names[ir_prim_as_unsigned(prim)]);
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
        snprintf(buf, sizeof(buf) - 1, "__lily_sub_%s", ir_prim_names[ir_prim_as_unsigned(prim)]);
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
        snprintf(buf, sizeof(buf) - 1, "__lily_mul_%s", ir_prim_names[ir_prim_as_unsigned(prim)]);
        ir_add_call(
            IR_AFTER_INSN(insn),
            IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM(buf)),
            true,
            insn->returns[0],
            2,
            insn->operands
        );
        ir_insn_delete(insn);
    } else if (insn->op2 == IR_OP2_div && (softfloat || !profile->has_div)) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_div_%s", ir_prim_names[prim]);
        ir_add_call(
            IR_AFTER_INSN(insn),
            IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM(buf)),
            true,
            insn->returns[0],
            2,
            insn->operands
        );
        ir_insn_delete(insn);
    } else if (insn->op2 == IR_OP2_rem && (softfloat || !profile->has_rem)) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_rem_%s", ir_prim_names[prim]);
        ir_add_call(
            IR_AFTER_INSN(insn),
            IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM(buf)),
            true,
            insn->returns[0],
            2,
            insn->operands
        );
        ir_insn_delete(insn);
    } else if (insn->op2 == IR_OP2_shr && insn->operands[1].type != IR_OPERAND_TYPE_CONST && !profile->has_var_shift) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_shr_%s", ir_prim_names[prim]);
        ir_var_t *tmp = ir_var_create(insn->code->func, IR_PRIM_u8, NULL); // __lily_shr_* uses u8 as shift amount
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
    } else if (insn->op2 == IR_OP2_shr && insn->operands[1].type != IR_OPERAND_TYPE_CONST && !profile->has_var_shift) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_shl_%s", ir_prim_names[ir_prim_as_unsigned(prim)]);
        ir_var_t *tmp = ir_var_create(insn->code->func, IR_PRIM_u8, NULL); // __lily_shl_u* uses u8 as shift amount
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
            snprintf(buf, sizeof(buf) - 1, "__lily_itof_%s_%s", ir_prim_names[ret_prim], ir_prim_names[prim]);
        } else {
            // Float to int.
            snprintf(buf, sizeof(buf) - 1, "__lily_ftoi_%s_%s", ir_prim_names[prim], ir_prim_names[ret_prim]);
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
        snprintf(buf, sizeof(buf) - 1, "__lily_neg_%s", ir_prim_names[ir_prim_as_unsigned(prim)]);
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
        case IR_OP2_sgt: commuted_op2 = IR_OP2_slt; break;
        case IR_OP2_sle: commuted_op2 = IR_OP2_sge; break;
        case IR_OP2_slt: commuted_op2 = IR_OP2_sgt; break;
        case IR_OP2_sge: commuted_op2 = IR_OP2_sle; break;

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
    assert(insn->type == IR_INSN_COMBINATOR);

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

        ir_add_expr1(loc, insn->returns[0], IR_OP1_mov, insn->combinators[i].bind);
    }

    ir_insn_delete(insn);
}

// Promote IR variable to at least as wide as the minimum register size.
static void cg_promote_var_0(backend_profile_t *profile, ir_var_t *var) {
    var->orig_prim_type = var->prim_type;
    ir_prim_t min_uint  = profile->arith_min_bits * 2 + IR_PRIM_u8;
    ir_prim_t min_sint  = profile->arith_min_bits * 2 + IR_PRIM_s8;
    if (var->prim_type == IR_PRIM_bool) {
        var->prim_type = min_uint;
    } else if (!ir_prim_is_integer(var->prim_type)) {
        return;
    } else if (ir_prim_is_signed(var->prim_type)) {
        if (var->prim_type < min_sint) {
            var->prim_type = min_sint;
        }
    } else {
        if (var->prim_type < min_uint) {
            var->prim_type = min_uint;
        }
    }
}

// Insert additional computations as needded to clamp promoted variable back in range.
static void cg_promote_var_1(backend_profile_t *profile, ir_var_t *orig) {
    (void)profile;
    assert(orig->assigned_at.len <= 1);
    if (orig->orig_prim_type == orig->prim_type || orig->assigned_at.len == 0 || orig->arg_index >= 0) {
        return;
    }
    ir_insn_t *assignment = set_next(&orig->assigned_at, NULL)->value;
    assert(orig->prim_type < IR_PRIM_f32);

    // TODO: Use IR var ranges to optimize out truncations.

    set_foreach(ir_insn_t, insn, &orig->used_at) {
        if (insn->flags & IR_INSN_FLAG_CLAMPING) {
            continue;
        }
        switch (insn->type) {
            case IR_INSN_EXPR1:
                switch (insn->op1) {
                    case IR_OP1_seqz:
                    case IR_OP1_snez:
                    case IR_OP1_mov:
                    case IR_OP1_bitcast: goto needs_clamp;
                    case IR_OP1_neg:
                    case IR_OP1_bneg:
                    case IR_N_OP1: break;
                }
                break;
            case IR_INSN_EXPR2:
                switch (insn->op2) {
                    case IR_OP2_shr:
                    case IR_OP2_div:
                    case IR_OP2_rem:
                    case IR_OP2_sgt:
                    case IR_OP2_sle:
                    case IR_OP2_slt:
                    case IR_OP2_sge:
                    case IR_OP2_seq:
                    case IR_OP2_sne: goto needs_clamp;
                    case IR_OP2_add:
                    case IR_OP2_sub:
                    case IR_OP2_mul:
                    case IR_OP2_shl:
                    case IR_OP2_band:
                    case IR_OP2_bor:
                    case IR_OP2_bxor:
                    case IR_N_OP2: break;
                }
                break;
            case IR_INSN_JUMP:
            case IR_INSN_BRANCH:
            case IR_INSN_STORE:
            case IR_INSN_LOAD:
            case IR_INSN_LEA: goto needs_clamp;
            case IR_INSN_MARK_USED: break; // Out of range doesn't matter here.
            case IR_INSN_COMBINATOR:
            case IR_INSN_CALL:
            case IR_INSN_RETURN:
            case IR_INSN_MEMCPY:
            case IR_INSN_MEMSET:
            case IR_INSN_ALLOCA: goto needs_clamp;
            case IR_INSN_CLOBBER:
            case IR_INSN_MACHINE:
            case IR_INSN_CALLFRAME_ENTER:
            case IR_INSN_CALLFRAME_EXIT: UNREACHABLE(); // We're before the stage that these would be added.
        }
    }
    return;

needs_clamp:;
    i128_t const prim_min = ir_prim_min(orig->orig_prim_type);
    i128_t const prim_max = ir_prim_max(orig->orig_prim_type);
    if (ir_prim_is_signed(orig->orig_prim_type)) {
        if (cmp128s(prim_min, orig->range_min) <= 0 && cmp128s(prim_max, orig->range_max) >= 0) {
            // Already in range.
            return;
        }
    } else {
        if (cmp128u(prim_min, orig->range_min) <= 0 && cmp128u(prim_max, orig->range_max) >= 0) {
            // Already in range.
            return;
        }
    }

    int       bits  = ir_prim_bits(orig->orig_prim_type);
    ir_var_t *dirty = ir_var_create(orig->func, orig->prim_type, NULL);
    ir_insn_set_return(assignment, 0, IR_RETVAL_VAR(dirty));

    // Don't forget to skip additional combinator nodes; IR assumes they come first.
    ir_insnloc_t loc = IR_AFTER_INSN(assignment);
    while (loc.type == IR_INSNLOC_AFTER_INSN && loc.insn->node.next) {
        ir_insn_t *next = container_of(loc.insn->node.next, ir_insn_t, node);
        if (next->type != IR_INSN_COMBINATOR) {
            break;
        }
        loc.insn = next;
    }

    if (ir_prim_is_signed(orig->orig_prim_type)) {
        // Signed values truncated by shifting left then right.
        ir_const_t shamt = {
            .prim_type = orig->prim_type,
            .const128  = ui128(ir_prim_bits(orig->prim_type) - bits),
        };
        ir_var_t  *shl_v = ir_var_create(orig->func, orig->prim_type, NULL);
        ir_insn_t *shl_i
            = ir_add_expr2(loc, IR_RETVAL_VAR(shl_v), IR_OP2_shl, IR_OPERAND_VAR(orig), IR_OPERAND_CONST(shamt));
        ir_add_expr2(
            IR_AFTER_INSN(shl_i),
            IR_RETVAL_VAR(orig),
            IR_OP2_shr,
            IR_OPERAND_VAR(shl_v),
            IR_OPERAND_CONST(shamt)
        );
    } else {
        // Unsigned values truncated by bitwise AND.
        ir_const_t iconst = {
            .prim_type = orig->prim_type,
            .const128  = sub128(shl128(ui128(1), bits), ui128(1)),
        };
        ir_add_expr2(loc, IR_RETVAL_VAR(orig), IR_OP2_band, IR_OPERAND_VAR(dirty), IR_OPERAND_CONST(iconst));
    }
}

// Convert an SSA-form IR function completely into executable machine code.
// All IR instructions are replaced, code order is decided by potentially re-ordering the code blocks from the
// functions, and unnecessary jumps are removed. When finished, the code blocks and instructions therein will be in
// order as written to the eventual executable file.
void codegen(backend_profile_t *profile, ir_func_t *func) {
    ir_func_to_ssa(func);

    // Perform optimizations.
    // TODO: Make this configurable.
    ir_optimize(func);

    // Replace arithmetic that is not supported with function calls.
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        ir_insn_t *insn = (ir_insn_t *)code->insns.head;
        while (insn) {
            ir_insn_t *next = (ir_insn_t *)insn->node.next;
            cg_functionize_exprs(profile, insn);
            insn = next;
        }
    }

    // TODO: Deduplication of return code.

    // Expand the ABI definition.
    assert(func->enforce_ssa);
    func->enforce_ssa = false;
    profile->backend->xabi_entry(profile, func);
    func->enforce_ssa = true;
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        cg_xabi(profile, code);
    }

    // Post-ABI optimization pass.
    // TODO: We'd need markers for vars to keep here, as ABI lowering doesn't mark them yet.
    // ir_optimize(func);

    // Remove jumps that go the the next code block linearly.
    cg_remove_jumps(func);

    // TODO: Convert operations into ones which fit in the CPU's registers.

    // Normalize operand order of instructions, if possible.
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        dlist_foreach_node(ir_insn_t, insn, &code->insns) {
            cg_normalize_op_order(insn);
        }
    }

    if (profile->backend->pre_isel_pass) {
        profile->backend->pre_isel_pass(profile, func);
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
        while (code->insns.head) {
            ir_insn_t *insn = container_of(code->insns.head, ir_insn_t, node);
            if (insn->type != IR_INSN_COMBINATOR) {
                break;
            }
            cg_expand_comb(insn);
        }
    }

    // Select machine instructions.
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        cg_isel(profile, code);
    }

    if (profile->backend->post_isel_pass) {
        profile->backend->post_isel_pass(profile, func);
    }

    // TODO: Stack offsets and what happens when they go out of range for immediate offsets.

    regalloc(profile, func);

    // Stip meta-instructions.
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
                case IR_INSN_MARK_USED: ir_insn_delete(insn); break;
                default: UNREACHABLE();
            }
            insn = next;
        }
    }
}
