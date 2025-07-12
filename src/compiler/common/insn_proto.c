
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "insn_proto.h"

#include "ir.h"
#include "ir_types.h"
#include "list.h"

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
    switch (tree->expr.insn_type) {
        case IR_INSN_EXPR: return expr_tree_size(tree->expr.expr.lhs) + expr_tree_size(tree->expr.expr.rhs);
        case IR_INSN_FLOW: return expr_tree_size(tree->expr.flow.value) + 1;
        case IR_INSN_MEM: return expr_tree_size(tree->expr.mem.value) + expr_tree_size(tree->expr.mem.ptr);
        case IR_INSN_MACHINE: printf("[BUG] Machine instruction is present in instruction prototype\n"); break;
    }
    abort();
}


// Helper function that deletes IR instructions for `insn_proto_substitute`.
static void insn_proto_substitute_del(expr_tree_t const *tree, ir_insn_t *ir_insn);

// Helper function that deletes IR instructions for `insn_proto_substitute`.
static void insn_proto_substitute_del2(expr_tree_t const *tree, ir_operand_t oper) {
    if (tree->type == EXPR_TREE_IR_INSN) {
        assert(!oper.is_const);
        insn_proto_substitute_del(tree, &container_of(oper.var->assigned_at.head, ir_expr_t, dest_node)->base);
    }
}

// Helper function that deletes IR instructions for `insn_proto_substitute`.
static void insn_proto_substitute_del(expr_tree_t const *tree, ir_insn_t *ir_insn) {
    switch (tree->expr.insn_type) {
        case IR_INSN_EXPR: {
            switch (tree->expr.expr.type) {
                default: abort();
                case IR_EXPR_UNARY:
                    insn_proto_substitute_del2(tree->expr.expr.lhs, ((ir_expr_t *)ir_insn)->e_unary.value);
                    break;
                case IR_EXPR_BINARY:
                    insn_proto_substitute_del2(tree->expr.expr.lhs, ((ir_expr_t *)ir_insn)->e_binary.lhs);
                    insn_proto_substitute_del2(tree->expr.expr.rhs, ((ir_expr_t *)ir_insn)->e_binary.rhs);
                    break;
            }
        } break;
        case IR_INSN_FLOW: {
            insn_proto_substitute_del2(tree->expr.flow.value, ((ir_flow_t *)ir_insn)->f_branch.cond);
        } break;
        case IR_INSN_MEM: {
            switch (tree->expr.mem.type) {
                case IR_MEM_LEA_STACK: break;
                case IR_MEM_LEA_SYMBOL: break;
                case IR_MEM_LOAD:
                    insn_proto_substitute_del2(tree->expr.mem.ptr, ((ir_mem_t *)ir_insn)->m_load.addr);
                    break;
                case IR_MEM_STORE:
                    insn_proto_substitute_del2(tree->expr.mem.ptr, ((ir_mem_t *)ir_insn)->m_store.addr);
                    insn_proto_substitute_del2(tree->expr.mem.ptr, ((ir_mem_t *)ir_insn)->m_store.src);
                    break;
            }
        } break;
        case IR_INSN_MACHINE: abort();
    }
    ir_insn_delete(ir_insn);
}

// Substitute a set of IR instructions with a machine instruction, assuming the instructions match the given prototype.
ir_mach_insn_t *insn_proto_substitute(insn_proto_t const *proto, ir_insn_t *ir_insn, ir_operand_t const *operands) {
    assert(ir_insn->parent->func->enforce_ssa);
    ir_insn->parent->func->enforce_ssa = false;
    ir_mach_insn_t *mach = ir_add_mach_insn(IR_AFTER_INSN(ir_insn), ir_insn_get_dest(ir_insn), proto, operands);
    insn_proto_substitute_del(proto->tree, ir_insn);
    ir_insn->parent->func->enforce_ssa = true;
    return mach;
}
