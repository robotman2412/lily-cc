
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
    ir_insn_type_t type;
    union {
        // Expression operator.
        ir_op1_type_t op1;
        // Expression operator.
        ir_op2_type_t op2;
    };
    // Child nodes of the tree.
    size_t        children_len;
    // Child nodes of the tree.
    cand_tree_t **children;
};

// Tree structure used to make instruction selection decisions.
struct cand_tree {
    // Next decision node, if any.
    cand_tree_t     *next;
    // Type of structure this matches against.
    expr_tree_type_t type;
    union {
        // Expression node that describes the operation performed.
        isel_node_t insn;
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
