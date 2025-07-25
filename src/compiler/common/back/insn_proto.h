
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "ir_types.h"
#include "match_tree.h"



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
    match_tree_t const   *tree;
};



// Substitute a set of IR instructions with a machine instruction, assuming the instructions match the given prototype.
ir_insn_t *insn_proto_substitute(insn_proto_t const *proto, ir_insn_t *ir_insn, ir_operand_t const *operands);
