
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "insn_proto.h"
#include "ir_types.h"



// IR instruction in a `cand_tree_t`.
typedef struct isel_node isel_node_t;
// Tree structure used to make instruction selection decisions.
typedef struct cand_tree cand_tree_t;



// IR instruction in a `cand_tree_t`.
struct isel_node {
    // IR instruction category.
    ir_insn_type_t insn_type;
    union {
        struct {
            // Expression subtype.
            // Allowed: IR_EXPR_BINARY, IR_EXPR_UNARY.
            ir_expr_type_t type;
            // Expression operator.
            union {
                ir_op1_type_t op1;
                ir_op2_type_t op2;
            };
            // Left-hand side subtree.
            // Also the operand for unary expressions.
            cand_tree_t *lhs;
            // Right-hand side subtree.
            cand_tree_t *rhs;
        } expr;
        struct {
            // Branch condition; no other types of flow-control are allowed here.
            cand_tree_t *value;
        } flow;
        struct {
            // Memory subtype.
            // Allowed: all.
            ir_mem_type_t type;
            // Value to store.
            cand_tree_t  *value;
            // Load / store pointer.
            cand_tree_t  *ptr;
        } mem;
    };
};

// Tree structure used to make instruction selection decisions.
struct cand_tree {
    // Next decision node, if any.
    cand_tree_t     *next;
    // Type of structure this matches against.
    expr_tree_type_t type;
    union {
        // Expression node that describes the operation performed.
        isel_node_t expr;
        struct {
            // IR constant value to match.
            ir_const_t           iconst;
            // Number of possible instruction prototypes.
            size_t               protos_len;
            // Possible instruction prototypes.
            insn_proto_t const **protos;
        };
    };
};



// Generate an instruction selection tree given an instruction set.
cand_tree_t        *cand_tree_generate(size_t protos_len, insn_proto_t const *const *protos);
// Match an instruction selection tree against IR in SSA form.
insn_proto_t const *cand_tree_isel(cand_tree_t const *tree, ir_insn_t const *ir_insn, ir_operand_t *operands_out);
