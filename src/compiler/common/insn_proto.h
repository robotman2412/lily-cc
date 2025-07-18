
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
} expr_tree_type_t;



// IR instruction in an `expr_tree_t`.
typedef struct expr_node     expr_node_t;
// Describes a tree of IR expressions and constants.
typedef struct expr_tree     expr_tree_t;
// Bitset of kinds of operand.
typedef union operand_kinds  operand_kinds_t;
// Bitset of kinds of operand storage locations.
typedef union location_kinds location_kinds_t;
// Bitset of possible operand sizes, fixed and relative to current arch.
typedef union operand_sizes  operand_sizes_t;
// Describes the constraints on a single instruction operand.
typedef struct operand_rule  operand_rule_t;
// Defines how a machine instruction behaves in terms of IR expressions.
typedef struct insn_proto    insn_proto_t;



// Node in an `expr_tree_t`.
struct expr_node {
    // IR instruction type.
    ir_insn_type_t type;
    union {
        // Expression operator.
        ir_op1_type_t op1;
        // Expression operator.
        ir_op2_type_t op2;
    };
    // Child nodes of the tree.
    size_t                    children_len;
    // Child nodes of the tree.
    expr_tree_t const *const *children;
};

// Describes a tree of IR expressions.
struct expr_tree {
    // What kind of node this is.
    expr_tree_type_t type;
    union {
        // Operand index in the parent instruction.
        // Multiple tree endpoints may have the same index.
        size_t      operand_index;
        // Expression node that describes what operation is done for this value.
        expr_node_t insn;
        // Constant encoded into the instruction itself.
        ir_const_t  iconst;
    };
};

// Bitset of kinds of operand.
union operand_kinds {
    struct {
        // Includes unsigned int operands.
        uint8_t uint : 1;
        // Includes signed int operands.
        // Implicitly includes unsigned operands one bit smaller.
        uint8_t sint : 1;
        // Includes 32-bit float constants.
        uint8_t f32  : 1;
        // Includes 64-bit float constants.
        uint8_t f64  : 1;
    };
    uint8_t val;
};

// Bitset of kinds of operand storage locations.
union location_kinds {
    struct {
        // Includes immediate values.
        uint8_t imm           : 1;
        // Includes registers.
        uint8_t reg           : 1;
        // Includes absolute memory locations.
        uint8_t mem_abs       : 1;
        // Includes PC-relative memory locations.
        uint8_t mem_pcrel     : 1;
        // Includes register-relative memory locations.
        uint8_t mem_regrel    : 1;
        // Allows an index register (according to architecture rules).
        uint8_t mem_index     : 1;
        // Allows non-pointer memory access.
        uint8_t mem_access    : 1;
        // Allows pointer memory access.
        uint8_t mem_ptraccess : 1;
    };
    uint8_t val;
};

// Bitset of possible operand sizes, fixed and relative to current arch.
union operand_sizes {
    struct {
        // 8-bit operand.
        uint8_t size8    : 1;
        // 16-bit operand.
        uint8_t size16   : 1;
        // 32-bit operand.
        uint8_t size32   : 1;
        // 64-bit operand.
        uint8_t size64   : 1;
        // 128-bit operand.
        uint8_t size128  : 1;
        // Pointer-sized operand.
        uint8_t sizeptr  : 1;
        // Machine word-sized operand.
        uint8_t sizeword : 1;
    };
    uint8_t val;
};

// Describes the constraints on a single instruction operand.
struct operand_rule {
    // Maximum no. bits for constant integers.
    // To disable const ints, set `uint` and `sint` of `const_kinds` to 0, not just this variable.
    uint8_t          const_bits;
    // What kinds of operands are allowed.
    operand_kinds_t  operand_kinds;
    // What kinds of operand storage is allowed.
    location_kinds_t location_kinds;
    // What sizes of operands are allowed for registers.
    operand_sizes_t  operand_sizes;
};

// Defines how a machine instruction behaves in terms of IR expressions.
struct insn_proto {
    // Human-readable instruction name.
    char const           *name;
    // Backend-specific extra information.
    void const           *cookie;
    // What kinds of value this instruction may return.
    operand_kinds_t       return_kinds;
    // What sizes of value can be returned.
    operand_sizes_t       return_sizes;
    // Number of operands in this prototype.
    size_t                operands_len;
    // Constraints on the operands.
    operand_rule_t const *operands;
    // The tree of IR expressions that describe this instruction.
    expr_tree_t const    *tree;
};



// Shorthand for NODE_OPERAND(0).
extern expr_tree_t const NODE_OPERAND_0;
// Shorthand for NODE_OPERAND(1).
extern expr_tree_t const NODE_OPERAND_1;
// Shorthand for NODE_OPERAND(2).
extern expr_tree_t const NODE_OPERAND_2;
// Shorthand for NODE_OPERAND(3).
extern expr_tree_t const NODE_OPERAND_3;

// clang-format off

#define NODE_CONST(iconst_)                     \
    (expr_tree_t const) {                       \
        .type = EXPR_TREE_ICONST,               \
        .iconst = iconst_,                      \
    }

#define NODE_OPERAND(operand_index_)            \
    (expr_tree_t const) {                       \
        .type = EXPR_TREE_OPERAND,              \
        .operand_index = operand_index_,        \
    }

#define NODE_EXPR1(op1_, operand_)              \
    (expr_tree_t const) {                       \
        .type = EXPR_TREE_IR_INSN,              \
        .insn = {                               \
            .type         = IR_INSN_EXPR1,      \
            .op1          = (op1_),             \
            .children_len = 1,                  \
            .children     =                     \
            (expr_tree_t const *const[]) {      \
                (operand_)                      \
            },                                  \
        },                                      \
    }

#define NODE_EXPR2(op2_, lhs_, rhs_)            \
    (expr_tree_t const) {                       \
        .type = EXPR_TREE_IR_INSN,              \
        .insn = {                               \
            .type         = IR_INSN_EXPR2,      \
            .op2          = (op2_),             \
            .children_len = 2,                  \
            .children     =                     \
            (expr_tree_t const *const[]) {      \
                (lhs_), (rhs_)                  \
            },                                  \
        },                                      \
    }

#define NODE_LOAD(ptr_)                         \
    (expr_tree_t const) {                       \
        .type = EXPR_TREE_IR_INSN,              \
        .insn = {                               \
            .type         = IR_INSN_LOAD,       \
            .children_len = 1,                  \
            .children     =                     \
            (expr_tree_t const *const[]) {      \
                (ptr_)                          \
            },                                  \
        },                                      \
    }

#define NODE_STORE(ptr_, value_)                \
    (expr_tree_t const) {                       \
        .type = EXPR_TREE_IR_INSN,              \
        .insn = {                               \
            .type         = IR_INSN_STORE,      \
            .children_len = 2,                  \
            .children     =                     \
            (expr_tree_t const *const[]) {      \
                (ptr_), (value_)                \
            },                                  \
        },                                      \
    }

#define NODE_BRANCH(target_, cond_)             \
    (expr_tree_t const) {                       \
        .type = EXPR_TREE_IR_INSN,              \
        .insn = {                               \
            .type         = IR_INSN_BRANCH,     \
            .children_len = 2,                  \
            .children     =                     \
            (expr_tree_t const *const[]) {      \
                (target_), (cond_)              \
            },                                  \
        },                                      \
    }

#define NODE_CALL(target_)                      \
    (expr_tree_t const) {                       \
        .type = EXPR_TREE_IR_INSN,              \
        .insn = {                               \
            .type         = IR_INSN_CALL,       \
            .children_len = 1,                  \
            .children     =                     \
            (expr_tree_t const *const[]) {      \
                (target_)                       \
            },                                  \
        },                                      \
    }


// clang-format on



// Calculate the number of nodes in an `expr_tree_t`.
size_t expr_tree_size(expr_tree_t const *tree);

// Substitute a set of IR instructions with a machine instruction, assuming the instructions match the given prototype.
ir_insn_t *insn_proto_substitute(insn_proto_t const *proto, ir_insn_t *ir_insn, ir_operand_t const *operands);
