
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "match_tree.h"



// Shorthand for NODE_OPERAND(0).
match_tree_t const NODE_OPERAND_0 = NODE_OPERAND(0);
// Shorthand for NODE_OPERAND(1).
match_tree_t const NODE_OPERAND_1 = NODE_OPERAND(1);
// Shorthand for NODE_OPERAND(2).
match_tree_t const NODE_OPERAND_2 = NODE_OPERAND(2);
// Shorthand for NODE_OPERAND(3).
match_tree_t const NODE_OPERAND_3 = NODE_OPERAND(3);



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
