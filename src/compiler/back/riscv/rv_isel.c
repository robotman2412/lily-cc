
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "rv_isel.h"

#include "ir_interpreter.h"
#include "ir_types.h"
#include "match_tree.h"
#include "rv_backend.h"
#include "rv_instructions.h"
#include "strong_malloc.h"
#include "sub_tree.h"

#include <stdio.h>
#include <stdlib.h>



// Register-register mov.
static isel_t rv_isel_rr_mov(ir_operand_t operand) {
    if (ir_operand_prim(operand) == IR_PRIM_f32 || ir_operand_prim(operand) == IR_PRIM_f64) {
        fprintf(stderr, "[TODO] Emit register copy for FP\n");
    }
    abort();
}

// Match tree for a `mov`.
static match_tree_t const rv_mov_match_tree = NODE_EXPR1(IR_OP1_mov, &NODE_OPERAND_0);

// Instruction substitution for `lui` + `addi`.
static insn_sub_t const rv_insn_sub_li_lui_addi = {
    .operands_len = 2,
    .sub_tree     = &SUB_TREE(
        &rv_insn_addi, SUBTREE_OPERAND(&rv_insn_lui, IR_PRIM_s32, SUB_OPERAND_MATCHED(0)), SUB_OPERAND_MATCHED(1)
    ),
    .match_tree = &rv_mov_match_tree,
};

// Instruction substitution for `lui`.
static insn_sub_t const rv_insn_sub_li_lui = {
    .operands_len = 1,
    .sub_tree     = &SUB_TREE(&rv_insn_lui, SUB_OPERAND_MATCHED(0)),
    .match_tree   = &rv_mov_match_tree,
};

// Instruction substitution for `addi`.
static insn_sub_t const rv_insn_sub_li_addi = {
    .operands_len = 1,
    .sub_tree     = &SUB_TREE(&rv_insn_addi, SUB_OPERAND_CONST(IR_CONST_S32(0)), SUB_OPERAND_MATCHED(0)),
    .match_tree   = &rv_mov_match_tree,
};

// Constant mov.
static isel_t rv_isel_const_mov(ir_const_t iconst) {
    iconst = ir_trim_const(iconst);

    if (iconst.prim_type == IR_PRIM_f32 || iconst.prim_type == IR_PRIM_f64) {
        fprintf(stderr, "[TODO] Emit constant for FP\n");
        abort();
    } else if (iconst.prim_type >= IR_PRIM_s64 && iconst.prim_type <= IR_PRIM_u128) {
        fprintf(stderr, "[TODO] Emit constant for 64-bit or larger\n");
        abort();
    }

    // Determine the operands to the `lui` and `addi` instructions.
    int32_t lui  = iconst.constl >> 12;
    int16_t addi = (int16_t)(iconst.constl << 4) >> 4;
    if (addi < 0) {
        lui++;
    }
    lui &= 0x000fffff;

    isel_t res = {0};

    // Determine whether to use `addi`, `lui` or both.
    if (lui && addi) {
        // Use `lui` followed by `addi`.
        res.sub          = &rv_insn_sub_li_lui_addi;
        res.operands     = strong_calloc(2, sizeof(ir_operand_t));
        res.operand_regs = strong_calloc(2, sizeof(bool));
        res.operands[0]  = IR_OPERAND_CONST(IR_CONST_S32(lui));
        res.operands[1]  = IR_OPERAND_CONST(IR_CONST_S16(addi));

    } else if (lui) {
        // Use just `lui`.
        res.sub          = &rv_insn_sub_li_lui;
        res.operands     = strong_calloc(1, sizeof(ir_operand_t));
        res.operand_regs = strong_calloc(1, sizeof(bool));
        res.operands[0]  = IR_OPERAND_CONST(IR_CONST_S32(lui));

    } else {
        // Use just `addi`.
        res.sub          = &rv_insn_sub_li_addi;
        res.operands     = strong_calloc(1, sizeof(ir_operand_t));
        res.operand_regs = strong_calloc(1, sizeof(bool));
        res.operands[0]  = IR_OPERAND_CONST(IR_CONST_S16(addi));
    }

    return res;
}

// Perform instruction selection for expressions, memory access and branches.
isel_t rv_isel(backend_profile_t *base_profile, ir_insn_t const *ir_insn) {
    rv_profile_t *profile = (void *)base_profile;

    if (ir_insn->type == IR_INSN_EXPR1 && ir_insn->op1 == IR_OP1_mov) {
        if (ir_insn->operands[0].type == IR_OPERAND_TYPE_CONST) {
            // Constant mov.
            return rv_isel_const_mov(ir_insn->operands[0].iconst);
        } else {
            // Register-register mov.
            return rv_isel_rr_mov(ir_insn->operands[0]);
        }
    }

    // Remaining insn patterns are done with tree isel.
    return cand_tree_isel(base_profile, profile->cand_tree, ir_insn, 4);
}
