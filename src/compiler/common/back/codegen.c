
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "codegen.h"

#include "backend.h"
#include "ir.h"
#include "ir/ir_serialization.h"
#include "ir_interpreter.h"
#include "ir_types.h"
#include "list.h"
#include "set.h"

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
    assert(code->func->enforce_ssa);
    code->func->enforce_ssa = false;
    ir_insn_t *cur          = container_of(code->insns.tail, ir_insn_t, node);
    while (cur) {
        if (cur->type != IR_INSN_MACHINE && cur->type != IR_INSN_COMBINATOR) {
            ir_insn_t *res = profile->backend->isel(profile, cur);
            if (!res) {
                fprintf(stderr, "[BUG] Backend cannot select an instruction for `");
                ir_insn_serialize(cur, profile, stderr);
                fprintf(stderr, "`\n");
                set_t vars = PTR_SET_EMPTY;
                for (size_t i = 0; i < cur->returns_len; i++) {
                    set_add(&vars, cur->returns[i]);
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
        cur = container_of(cur->node.previous, ir_insn_t, node);
    }
    code->func->enforce_ssa = true;
}

// Replace arithmetic that is not supported with function calls.
static void cg_functionize_expr2(backend_profile_t *profile, ir_insn_t *insn) {
    assert(insn->type == IR_INSN_EXPR2);
    assert(insn->returns_len == 1);
    assert(insn->operands_len == 2);
    ir_prim_t const prim = insn->returns[0]->prim_type;
    bool softfloat       = (prim == IR_PRIM_f32 && !profile->has_f32) || (prim == IR_PRIM_f64 && !profile->has_f64);

    insn->code->func->enforce_ssa = false;
    if (insn->op2 == IR_OP2_add && softfloat) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_add_%s", ir_prim_names[ir_prim_as_unsigned(prim)]);
        ir_add_call(IR_AFTER_INSN(insn), IR_MEMREF(IR_PRIM_u8, IR_BADDR_SYM(buf)), 1, insn->returns, 2, insn->operands);
        ir_insn_delete(insn);
    } else if (insn->op2 == IR_OP2_sub && softfloat) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_sub_%s", ir_prim_names[ir_prim_as_unsigned(prim)]);
        ir_add_call(IR_AFTER_INSN(insn), IR_MEMREF(IR_PRIM_u8, IR_BADDR_SYM(buf)), 1, insn->returns, 2, insn->operands);
        ir_insn_delete(insn);
    } else if (insn->op2 == IR_OP2_mul && (softfloat || !profile->has_mul)) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_mul_%s", ir_prim_names[ir_prim_as_unsigned(prim)]);
        ir_add_call(IR_AFTER_INSN(insn), IR_MEMREF(IR_PRIM_u8, IR_BADDR_SYM(buf)), 1, insn->returns, 2, insn->operands);
        ir_insn_delete(insn);
    } else if (insn->op2 == IR_OP2_div && (softfloat || !profile->has_div)) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_div_%s", ir_prim_names[prim]);
        ir_add_call(IR_AFTER_INSN(insn), IR_MEMREF(IR_PRIM_u8, IR_BADDR_SYM(buf)), 1, insn->returns, 2, insn->operands);
        ir_insn_delete(insn);
    } else if (insn->op2 == IR_OP2_rem && (softfloat || !profile->has_rem)) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_rem_%s", ir_prim_names[prim]);
        ir_add_call(IR_AFTER_INSN(insn), IR_MEMREF(IR_PRIM_u8, IR_BADDR_SYM(buf)), 1, insn->returns, 2, insn->operands);
        ir_insn_delete(insn);
    } else if (insn->op2 == IR_OP2_shr && insn->operands[1].type != IR_OPERAND_TYPE_CONST && !profile->has_var_shift) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_shr_%s", ir_prim_names[prim]);
        ir_var_t *tmp = ir_var_create(insn->code->func, IR_PRIM_u8, NULL); // __lily_shr_* uses u8 as shift amount
        ir_add_expr1(IR_AFTER_INSN(insn), tmp, IR_OP1_mov, insn->operands[1]);
        ir_add_call(
            IR_AFTER_INSN(insn),
            IR_MEMREF(IR_PRIM_u8, IR_BADDR_SYM(buf)),
            1,
            insn->returns,
            2,
            (ir_operand_t const[]){insn->operands[0], IR_OPERAND_VAR(tmp)}
        );
        ir_insn_delete(insn);
    } else if (insn->op2 == IR_OP2_shr && insn->operands[1].type != IR_OPERAND_TYPE_CONST && !profile->has_var_shift) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_shl_%s", ir_prim_names[ir_prim_as_unsigned(prim)]);
        ir_var_t *tmp = ir_var_create(insn->code->func, IR_PRIM_u8, NULL); // __lily_shl_u* uses u8 as shift amount
        ir_add_expr1(IR_AFTER_INSN(insn), tmp, IR_OP1_mov, insn->operands[1]);
        ir_add_call(
            IR_AFTER_INSN(insn),
            IR_MEMREF(IR_PRIM_u8, IR_BADDR_SYM(buf)),
            1,
            insn->returns,
            2,
            (ir_operand_t const[]){insn->operands[0], IR_OPERAND_VAR(tmp)}
        );
        ir_insn_delete(insn);
    }
    // TODO: Float comparisons.
    insn->code->func->enforce_ssa = true;
}

// Replace arithmetic that is not supported with function calls.
static void cg_functionize_expr1(backend_profile_t *profile, ir_insn_t *insn) {
    assert(insn->type == IR_INSN_EXPR1);
    assert(insn->returns_len == 1);
    assert(insn->operands_len == 1);
    ir_prim_t const prim     = ir_operand_prim(insn->operands[0]);
    ir_prim_t const ret_prim = insn->returns[0]->prim_type;
    bool            softfloat
        = ((prim == IR_PRIM_f32 && !profile->has_f32) || (prim == IR_PRIM_f64 && !profile->has_f64))
          || ((ret_prim == IR_PRIM_f32 && !profile->has_f32) || (ret_prim == IR_PRIM_f64 && !profile->has_f64));

    insn->code->func->enforce_ssa = false;
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
        ir_add_call(IR_AFTER_INSN(insn), IR_MEMREF(IR_PRIM_u8, IR_BADDR_SYM(buf)), 1, insn->returns, 1, insn->operands);
        ir_insn_delete(insn);
    } else if (insn->op1 == IR_OP1_neg && softfloat) {
        char buf[32];
        snprintf(buf, sizeof(buf) - 1, "__lily_neg_%s", ir_prim_names[ir_prim_as_unsigned(prim)]);
        ir_add_call(IR_AFTER_INSN(insn), IR_MEMREF(IR_PRIM_u8, IR_BADDR_SYM(buf)), 1, insn->returns, 1, insn->operands);
        ir_insn_delete(insn);
    }
    // TODO: Float comparisons.
    insn->code->func->enforce_ssa = true;
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

// Convert an SSA-form IR function completely into executable machine code.
// All IR instructions are replaced, code order is decided by potentially re-ordering the code blocks from the
// functions, and unnecessary jumps are removed. When finished, the code blocks and instructions therein will be in
// order as written to the eventual executable file.
void codegen(backend_profile_t *profile, ir_func_t *func) {
    assert(func->enforce_ssa);

    // Remove jumps that go the the next code block linearly.
    cg_remove_jumps(func);

    // Replace arithmetic that is not supported with function calls.
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        dlist_foreach_node(ir_insn_t, insn, &code->insns) {
            cg_functionize_exprs(profile, insn);
        }
    }

    // Normalize operand order of instructions, if possible.
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        dlist_foreach_node(ir_insn_t, insn, &code->insns) {
            cg_normalize_op_order(insn);
        }
    }

    if (profile->backend->pre_isel_pass) {
        profile->backend->pre_isel_pass(profile, func);
    }

    // Select machine instructions.
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        cg_isel(profile, code);
    }

    if (profile->backend->post_isel_pass) {
        profile->backend->post_isel_pass(profile, func);
    }
}
