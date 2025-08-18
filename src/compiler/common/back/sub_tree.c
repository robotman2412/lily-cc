
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "sub_tree.h"

#include "ir.h"
#include "strong_malloc.h"

#include <assert.h>



// Build a substitution using gathered match tree operands.
// It is assumed that the caller checks enough operands are available.
ir_insn_t *
    insn_sub_insert(ir_insnloc_t loc, ir_var_t *dest, sub_tree_t const *tree, ir_operand_t const *matched_operands) {
    ir_func_t *const func     = ir_insnloc_code(loc)->func;
    ir_operand_t    *operands = strong_calloc(tree->operands_len, sizeof(ir_operand_t));

    // Generate operands to the instruction.
    for (size_t i = 0; i < tree->operands_len; i++) {
        switch (tree->operands[i].type) {
            case SUB_OPERAND_TYPE_MATCHED: operands[i] = matched_operands[tree->operands[i].matched]; break;
            case SUB_OPERAND_TYPE_CONST: operands[i] = IR_OPERAND_CONST(tree->operands[i].iconst); break;
            case SUB_OPERAND_TYPE_SUBTREE:
                operands[i] = IR_OPERAND_VAR(ir_var_create(func, tree->operands[i].subtree.prim, NULL));
                break;
        }
    }

    // Insert this node's instruction
    ir_insn_t *insn = ir_add_mach_insn(loc, dest, tree->proto, tree->operands_len, operands);

    // Generate subtrees as needed.
    for (size_t i = 0; i < tree->operands_len; i++) {
        if (tree->operands[i].type == SUB_OPERAND_TYPE_SUBTREE) {
            insn_sub_insert(IR_BEFORE_INSN(insn), operands[i].var, tree->operands[i].subtree.tree, matched_operands);
        }
    }

    free(operands);
    return insn;
}
