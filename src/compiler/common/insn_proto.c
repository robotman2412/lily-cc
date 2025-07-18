
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "insn_proto.h"

#include "ir.h"
#include "ir_types.h"
#include "set.h"

#include <assert.h>
#include <stdlib.h>



// Shorthand for NODE_OPERAND(0).
expr_tree_t const NODE_OPERAND_0 = NODE_OPERAND(0);
// Shorthand for NODE_OPERAND(1).
expr_tree_t const NODE_OPERAND_1 = NODE_OPERAND(1);
// Shorthand for NODE_OPERAND(2).
expr_tree_t const NODE_OPERAND_2 = NODE_OPERAND(2);
// Shorthand for NODE_OPERAND(3).
expr_tree_t const NODE_OPERAND_3 = NODE_OPERAND(3);



// Calculate the number of nodes in an `expr_tree_t`.
size_t expr_tree_size(expr_tree_t const *tree) {
    if (!tree) {
        return 0;
    }
    if (tree->type == EXPR_TREE_ICONST || tree->type == EXPR_TREE_OPERAND) {
        return 1;
    }
    size_t size = 1;
    for (size_t i = 0; i < tree->insn.children_len; i++) {
        size += expr_tree_size(tree->insn.children[i]);
    }
    return size;
}


// Helper function that deletes IR instructions for `insn_proto_substitute`.
static void insn_proto_substitute_del(expr_tree_t const *tree, ir_insn_t *ir_insn);

// Helper function that deletes IR instructions for `insn_proto_substitute`.
static void insn_proto_substitute_del2(expr_tree_t const *tree, ir_operand_t oper) {
    if (tree->type == EXPR_TREE_IR_INSN) {
        assert(oper.type == IR_OPERAND_TYPE_VAR);
        set_ent_t const *ent = set_next(&oper.var->assigned_at, NULL);
        insn_proto_substitute_del(tree, ent->value);
    }
}

// Helper function that deletes IR instructions for `insn_proto_substitute`.
static void insn_proto_substitute_del(expr_tree_t const *tree, ir_insn_t *ir_insn) {
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
        insn_proto_substitute_del2(tree->insn.children[i], ir_insn->operands[i]);
    }

    ir_insn_delete(ir_insn);
}

// Substitute a set of IR instructions with a machine instruction, assuming the instructions match the given prototype.
ir_insn_t *insn_proto_substitute(insn_proto_t const *proto, ir_insn_t *ir_insn, ir_operand_t const *operands) {
    assert(ir_insn->parent->func->enforce_ssa);
    ir_insn->parent->func->enforce_ssa = false;
    ir_insn_t *mach = ir_add_mach_insn(IR_AFTER_INSN(ir_insn), ir_insn_get_dest(ir_insn), proto, operands);
    insn_proto_substitute_del(proto->tree, ir_insn);
    ir_insn->parent->func->enforce_ssa = true;
    return mach;
}
