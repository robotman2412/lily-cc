
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "insn_proto.h"

#include "ir_types.h"



// Shorthand for NODE_OPERAND(0).
expr_tree_t const NODE_OPERAND_0 = NODE_OPERAND(0);
// Shorthand for NODE_OPERAND(1).
expr_tree_t const NODE_OPERAND_1 = NODE_OPERAND(1);
// Shorthand for NODE_OPERAND(2).
expr_tree_t const NODE_OPERAND_2 = NODE_OPERAND(2);
// Shorthand for NODE_OPERAND(3).
expr_tree_t const NODE_OPERAND_3 = NODE_OPERAND(3);



// Calculate the maximum depth of an `expr_tree_t`.
size_t expr_tree_size(expr_tree_t const *tree) {
    if (!tree) {
        return 0;
    }
    if (tree->type == EXPR_TREE_ICONST || tree->type == EXPR_TREE_OPERAND) {
        return 1;
    }
    switch (tree->expr.insn_type) {
        case IR_INSN_EXPR: return expr_tree_size(tree->expr.expr.lhs) + expr_tree_size(tree->expr.expr.rhs);
        case IR_INSN_FLOW: return expr_tree_size(tree->expr.flow.value) + 1;
        case IR_INSN_MEM: return expr_tree_size(tree->expr.mem.value) + expr_tree_size(tree->expr.mem.ptr);
    }
}

// Test whether a prototype can be applied to a certain piece of IR in SSA form.
bool insn_proto_match(insn_proto_t const *proto, ir_insn_t const *ir_insn) {
}
