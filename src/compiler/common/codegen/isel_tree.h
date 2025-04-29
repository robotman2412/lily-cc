
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "insn_proto.h"



// IR instruction in a `isel_tree_t`.
typedef struct isel_node isel_node_t;
// Tree structure used to make instruction selection decisions.
typedef struct isel_tree isel_tree_t;



// IR instruction in a `isel_tree_t`.
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
            isel_tree_t *lhs;
            // Right-hand side subtree.
            isel_tree_t *rhs;
        } expr;
        struct {
            // Control-flow subtype.
            // Allowed: all.
            ir_flow_type_t type;
            // Jump / call pointer subtree or branch condition.
            isel_tree_t   *value;
        } flow;
        struct {
            // Memory subtype.
            // Allowed: all.
            ir_mem_type_t type;
            // Value to store.
            isel_tree_t  *value;
            // Load / store pointer.
            isel_tree_t  *ptr;
        } mem;
    };
};

// Tree structure used to make instruction selection decisions.
struct isel_tree {
    // Next decision node, if any.
    isel_tree_t     *next;
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
isel_tree_t *isel_tree_generate(size_t protos_len, insn_proto_t const *const *protos);

// Match an instruction selection tree against IR in SSA form.
insn_proto_t const *isel(isel_tree_t const *tree, ir_insn_t const *ir_insn);
