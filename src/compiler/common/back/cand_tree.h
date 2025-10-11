
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

// IR instruction in a `cand_tree_t`.
typedef struct isel_node isel_node_t;
// Tree structure used to make instruction selection decisions.
typedef struct cand_tree cand_tree_t;

#include "backend.h"
#include "ir_types.h"
#include "sub_tree.h"



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
    cand_tree_t      *next;
    // Type of structure this matches against.
    match_tree_type_t type;
    union {
        // Expression node that describes the operation performed.
        isel_node_t insn;
        struct {
            // IR constant value to match.
            ir_const_t         iconst;
            // Number of possible instruction substitutions.
            size_t             subs_len;
            // Possible instruction substitutions.
            insn_sub_t const **subs;
        };
    };
};



// Generate an instruction selection tree given an instruction set.
cand_tree_t *cand_tree_generate(size_t rules_len, insn_sub_t const *const *rules);
// Delete an instruction selection tree.
void         cand_tree_delete(cand_tree_t *tree);
// Match an instruction selection tree against IR in SSA form.
ir_insn_t *
    cand_tree_isel(backend_profile_t *backend, cand_tree_t const *tree, ir_insn_t const *ir_insn, size_t operands_cap);
