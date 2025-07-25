
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "insn_proto.h"

#include "ir.h"
#include "ir_types.h"
#include "set.h"

#include <assert.h>
#include <stdlib.h>



// Helper function that deletes IR instructions for `insn_proto_substitute`.
static void insn_proto_substitute_del(match_tree_t const *tree, ir_insn_t *ir_insn);

// Helper function that deletes IR instructions for `insn_proto_substitute`.
static void insn_proto_substitute_del2(match_tree_t const *tree, ir_operand_t oper) {
    if (tree->type == EXPR_TREE_IR_INSN) {
        assert(oper.type == IR_OPERAND_TYPE_VAR);
        set_ent_t const *ent = set_next(&oper.var->assigned_at, NULL);
        insn_proto_substitute_del(tree, ent->value);
    }
}

// Helper function that deletes IR instructions for `insn_proto_substitute`.
static void insn_proto_substitute_del(match_tree_t const *tree, ir_insn_t *ir_insn) {
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
    ir_func_t *func   = ir_insn->parent->func;
    func->enforce_ssa = false;
    ir_insn_t *mach   = ir_add_mach_insn(IR_AFTER_INSN(ir_insn), ir_insn_get_dest(ir_insn), proto, operands);
    insn_proto_substitute_del(proto->tree, ir_insn);
    func->enforce_ssa = true;
    return mach;
}
