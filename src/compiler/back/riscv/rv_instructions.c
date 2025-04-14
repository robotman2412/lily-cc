
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "rv_instructions.h"



// clang-format off

// Define a register-immediate instruction.
#define DEF_RI(name, encoding, tree)            \
    insn_proto_t const rv_insn_##name = {       \
        .cookie = encoding,                     \
        .return_kinds = {                       \
            .uint = 1,                          \
            .sint = 1,                          \
        },                                      \
        .operands_len = 2,                      \
        .operands     = ri_operand_rules,       \
    };

// Define a register-register instruction.
#define DEF_RR(name, encoding, tree)            \
    insn_proto_t const rv_insn_##name = {       \
        .cookie = encoding,                     \
        .return_kinds = {                       \
            .uint = 1,                          \
            .sint = 1,                          \
        },                                      \
        .operands_len = 2,                      \
        .operands     = ri_operand_rules,       \
    };

// Define an ALU instruction.
#define DEF_ALU_RI32(name, ir_op2) \
    expr_tree_t const rv_itree_##name = NODE_EXPR2(ir_op2, &NODE_OPERAND_0, &NODE_OPERAND_1); \
    DEF_RI(name##i,  NULL, &rv_itree_##name) \
    DEF_RI(name##iw, NULL, &rv_itree_##name) \
    DEF_RR(name,     NULL, &rv_itree_##name) \
    DEF_RR(name##w,  NULL, &rv_itree_##name)

// Define an ALU instruction.
#define DEF_ALU_R32(name, ir_op2) \
    expr_tree_t const rv_itree_##name = NODE_EXPR2(ir_op2, &NODE_OPERAND_0, &NODE_OPERAND_1); \
    DEF_RR(name,     NULL, &rv_itree_##name) \
    DEF_RR(name##w,  NULL, &rv_itree_##name)

// Define an ALU instruction.
#define DEF_ALU_R32(name, ir_op2) \
    expr_tree_t const rv_itree_##name = NODE_EXPR2(ir_op2, &NODE_OPERAND_0, &NODE_OPERAND_1); \
    DEF_RI(name,     NULL, &rv_itree_##name) \
    DEF_RI(name##w,  NULL, &rv_itree_##name)

// clang-format on



// Operand rules for register-immediate instructions.
operand_rule_t const ri_operand_rules[2] = {
    {
        .location_kinds.reg = 1,
        .operand_kinds = {
            .uint = 1,
            .sint = 1,
        },
    }, {
        .const_bits = 12,
        .location_kinds.imm = 1,
        .operand_kinds = {
            .uint = 1,
            .sint = 1,
        },
    },
};



#include "rv_instructions.inc"
