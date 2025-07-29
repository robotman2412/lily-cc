
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

// Types of substituted operand.
typedef enum {
    // Use operand from match tree by index.
    SUB_OPERAND_TYPE_MATCHED,
    // Use IR constant operand.
    SUB_OPERAND_TYPE_CONST,
    // Use return value of subtree.
    SUB_OPERAND_TYPE_SUBTREE,
} sub_operand_type_t;

// Bitset of kinds of operand.
typedef union operand_kinds  operand_kinds_t;
// Bitset of kinds of operand storage locations.
typedef union location_kinds location_kinds_t;
// Bitset of possible operand sizes, fixed and relative to current arch.
typedef union operand_sizes  operand_sizes_t;
// Describes the constraints on a single instruction operand.
typedef struct operand_rule  operand_rule_t;
// Instruction operand specifier for `sub_tree_t`.
typedef struct sub_operand   sub_operand_t;
// Tree that specifies what machine instructions to use in a `insn_sub_t`.
typedef struct sub_tree      sub_tree_t;
// Describes a possible instruction substitution.
typedef struct insn_sub      insn_sub_t;

#include "ir.h"
#include "match_tree.h"



// Bitset of kinds of operand.
union operand_kinds {
    struct {
        // Includes unsigned int operands.
        uint8_t uint  : 1;
        // Includes signed int operands.
        // Implicitly includes unsigned constants one bit smaller.
        uint8_t sint  : 1;
        // Includes 32-bit float constants.
        uint8_t f32   : 1;
        // Includes 64-bit float constants.
        uint8_t f64   : 1;
        // Includes booleans.
        uint8_t bool_ : 1;
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

// Instruction operand specifier for `sub_tree_t`.
struct sub_operand {
    // Type of operand that this is.
    sub_operand_type_t type;
    union {
        // Index of matched operand to use.
        size_t     matched;
        // IR constant to use.
        ir_const_t iconst;
        struct {
            // Subtree to generate.
            sub_tree_t const *tree;
            // IR primitive type to go through.
            ir_prim_t         prim;
        } subtree;
    };
};

// Tree that specifies what machine instructions to use in a `insn_sub_t`.
struct sub_tree {
    // Machine instruction to place.
    insn_proto_t const  *proto;
    // Number of operands to use here.
    size_t               operands_len;
    // Operands to use here.
    sub_operand_t const *operands;
};

// Describes a possible instruction substitution.
struct insn_sub {
    // What kinds of value this instruction may return.
    operand_kinds_t       return_kinds;
    // What sizes of value can be returned.
    operand_sizes_t       return_sizes;
    // Number of operands in this prototype.
    size_t                operands_len;
    // Constraints on the operands.
    operand_rule_t const *operands;
    // IR matching tree that this substitution describes.
    match_tree_t const   *match_tree;
    // What machine instructions to replace this with.
    sub_tree_t const     *sub_tree;
};



// clang-format off

#define SUB_OPERAND_MATCHED(matched_)        \
    (sub_operand_t const) {                  \
        .type    = SUB_OPERAND_TYPE_MATCHED, \
        .matched = matched_,                 \
    }

#define SUB_OPERAND_CONST(iconst_)           \
    (sub_operand_t const) {                  \
        .type   = SUB_OPERAND_TYPE_CONST,    \
        .iconst = iconst_,                   \
    }

#define SUB_OPERAND_SUBTREE(subtree_, prim_) \
    (sub_operand_t const) {                  \
        .type    = SUB_OPERAND_TYPE_SUBTREE, \
        .subtree = {                         \
            .tree = subtree_,                \
            .prim = prim_,                   \
        }                                    \
    }

#define SUB_TREE(proto_, ...)                                                                 \
    (sub_tree_t const) {                                                                      \
        .proto = proto_,                                                                      \
        .operands_len = sizeof((sub_operand_t const[]){__VA_ARGS__}) / sizeof(sub_operand_t), \
        .operands = (sub_operand_t const[]) {__VA_ARGS__},                                    \
    }

#define SUBTREE_OPERAND(proto_, prim_, ...) SUB_OPERAND_SUBTREE(&SUB_TREE(proto_, __VA_ARGS__), prim_)

// clang-format on



// Build a substitution using gathered match tree operands.
// It is assumed that the caller checks enough operands are available.
ir_insn_t *insn_sub_insert(ir_insnloc_t loc, ir_var_t *dest, sub_tree_t const *sub_tree, ir_operand_t const *operands);
