
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "ir_types.h"



// Types of expression tree.
typedef enum {
    // IR instruction; `expr_node_t`.
    EXPR_TREE_IR_INSN,
    // Operand; an input to the tree from an IR operand.
    EXPR_TREE_OPERAND,
    // IR constant that must exactly match.
    EXPR_TREE_ICONST,
} match_tree_type_t;



// IR instruction in an `match_tree_t.
typedef struct match_node match_node_t;
// Describes a tree of IR expressions and constants.
typedef struct match_tree match_tree_t;



// Node in an `match_tree_t.
struct match_node {
    // IR instruction type.
    ir_insn_type_t type;
    union {
        // Expression operator.
        ir_op1_type_t op1;
        // Expression operator.
        ir_op2_type_t op2;
    };
    // Child nodes of the tree.
    size_t                     children_len;
    // Child nodes of the tree.
    match_tree_t const *const *children;
};

// Describes a tree of IR expressions.
struct match_tree {
    // What kind of node this is.
    match_tree_type_t type;
    union {
        // Operand index in the parent instruction.
        // Multiple tree endpoints may have the same index.
        size_t       operand_index;
        // Expression node that describes what operation is done for this value.
        match_node_t insn;
        // Constant encoded into the instruction itself.
        ir_const_t   iconst;
    };
};


// Shorthand for NODE_OPERAND(0).
extern match_tree_t const NODE_OPERAND_0;
// Shorthand for NODE_OPERAND(1).
extern match_tree_t const NODE_OPERAND_1;
// Shorthand for NODE_OPERAND(2).
extern match_tree_t const NODE_OPERAND_2;
// Shorthand for NODE_OPERAND(3).
extern match_tree_t const NODE_OPERAND_3;

// clang-format off

#define NODE_CONST(iconst_)                     \
    (match_tree_t const) {                      \
        .type = EXPR_TREE_ICONST,               \
        .iconst = iconst_,                      \
    }

#define NODE_OPERAND(operand_index_)            \
    (match_tree_t const) {                      \
        .type = EXPR_TREE_OPERAND,              \
        .operand_index = operand_index_,        \
    }

#define NODE_EXPR1(op1_, operand_)              \
    (match_tree_t const) {                      \
        .type = EXPR_TREE_IR_INSN,              \
        .insn = {                               \
            .type         = IR_INSN_EXPR1,      \
            .op1          = (op1_),             \
            .children_len = 1,                  \
            .children     =                     \
            (match_tree_t const *const[]) {     \
                (operand_)                      \
            },                                  \
        },                                      \
    }

#define NODE_EXPR2(op2_, lhs_, rhs_)            \
    (match_tree_t const) {                      \
        .type = EXPR_TREE_IR_INSN,              \
        .insn = {                               \
            .type         = IR_INSN_EXPR2,      \
            .op2          = (op2_),             \
            .children_len = 2,                  \
            .children     =                     \
            (match_tree_t const *const[]) {     \
                (lhs_), (rhs_)                  \
            },                                  \
        },                                      \
    }

#define NODE_LOAD(ptr_)                         \
    (match_tree_t const) {                      \
        .type = EXPR_TREE_IR_INSN,              \
        .insn = {                               \
            .type         = IR_INSN_LOAD,       \
            .children_len = 1,                  \
            .children     =                     \
            (match_tree_t const *const[]) {     \
                (ptr_)                          \
            },                                  \
        },                                      \
    }

#define NODE_STORE(ptr_, value_)                \
    (match_tree_t const) {                      \
        .type = EXPR_TREE_IR_INSN,              \
        .insn = {                               \
            .type         = IR_INSN_STORE,      \
            .children_len = 2,                  \
            .children     =                     \
            (match_tree_t const *const[]) {     \
                (ptr_), (value_)                \
            },                                  \
        },                                      \
    }

#define NODE_BRANCH(target_, cond_)             \
    (match_tree_t const) {                      \
        .type = EXPR_TREE_IR_INSN,              \
        .insn = {                               \
            .type         = IR_INSN_BRANCH,     \
            .children_len = 2,                  \
            .children     =                     \
            (match_tree_t const *const[]) {     \
                (target_), (cond_)              \
            },                                  \
        },                                      \
    }

#define NODE_CALL(target_)                      \
    (match_tree_t const) {                      \
        .type = EXPR_TREE_IR_INSN,              \
        .insn = {                               \
            .type         = IR_INSN_CALL,       \
            .children_len = 1,                  \
            .children     =                     \
            (match_tree_t const *const[]) {     \
                (target_)                       \
            },                                  \
        },                                      \
    }


// clang-format on



// Calculate the number of nodes in an `match_tree_t.
size_t match_tree_size(match_tree_t const *tree);
