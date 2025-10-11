
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "match_tree.h"

#include "ir.h"

#include <assert.h>



// Shorthand for NODE_OPERAND(0).
match_tree_t const NODE_OPERAND_0 = NODE_OPERAND(0);
// Shorthand for NODE_OPERAND(1).
match_tree_t const NODE_OPERAND_1 = NODE_OPERAND(1);
// Shorthand for NODE_OPERAND(2).
match_tree_t const NODE_OPERAND_2 = NODE_OPERAND(2);
// Shorthand for NODE_OPERAND(3).
match_tree_t const NODE_OPERAND_3 = NODE_OPERAND(3);


// Helper function that deletes IR instructions matching `tree`.
static void match_tree_del2(match_tree_t const *tree, ir_operand_t oper) {
    if (tree->type == EXPR_TREE_IR_INSN) {
        assert(oper.type == IR_OPERAND_TYPE_VAR);
        set_ent_t const *ent = set_next(&oper.var->assigned_at, NULL);
        match_tree_del(tree, ent->value);
    }
}

// Helper function that deletes IR instructions matching `tree`.
void match_tree_del(match_tree_t const *tree, ir_insn_t *ir_insn) {
    assert(tree->insn.type == ir_insn->type);
    if (tree->insn.type == IR_INSN_CALL) {
        assert(tree->insn.children_len == 1);
        assert(ir_insn->operands_len >= 1);
    } else if (tree->insn.type == IR_INSN_RETURN) {
        assert(tree->insn.children_len == 0);
    } else {
        assert(tree->insn.children_len == ir_insn->operands_len);
    }

    for (size_t i = 0; i < tree->insn.children_len; i++) {
        match_tree_del2(tree->insn.children[i], ir_insn->operands[i]);
    }

    ir_insn_delete(ir_insn);
}



// Calculate the number of nodes in an `expr_tree_t`.
size_t match_tree_size(match_tree_t const *tree) {
    if (!tree) {
        return 0;
    }
    if (tree->type == EXPR_TREE_ICONST || tree->type == EXPR_TREE_OPERAND) {
        return 1;
    }
    size_t size = 1;
    for (size_t i = 0; i < tree->insn.children_len; i++) {
        size += match_tree_size(tree->insn.children[i]);
    }
    return size;
}
