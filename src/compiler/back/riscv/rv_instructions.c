
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "rv_instructions.h"



// clang-format off

// Define a register-immediate instruction.
#define DEF_RI(name, encoding, tree, bits, sign)            \
    insn_proto_t const rv_insn_##name = {                   \
        .cookie = encoding,                                 \
        .return_kinds = {                                   \
            .uint = 1,                                      \
            .sint = 1,                                      \
        },                                                  \
        .operands_len = 2,                                  \
        .operands     = ri_operand_rules##immbits##_##sign, \
    };

// Define a register-register instruction.
#define DEF_RR(name, encoding, tree, immbits, sign)            \
    insn_proto_t const rv_insn_##name = {                   \
        .cookie = encoding,                                 \
        .return_kinds = {                                   \
            .uint = 1,                                      \
            .sint = 1,                                      \
        },                                                  \
        .operands_len = 2,                                  \
        .operands     = rr_operand_rules##immbits##_##sign, \
    };

// Define an ALU instruction.
#define DEF_ALU_RI32(name, isa, immbits, sign, funct3, ir_op2) \
    expr_tree_t const rv_itree_##name = NODE_EXPR2(ir_op2, &NODE_OPERAND_0, &NODE_OPERAND_1); \
    DEF_RI(name##i,  NULL, &rv_itree_##name, immbits, sign) \
    DEF_RI(name##iw, NULL, &rv_itree_##name, immbits, sign) \
    DEF_RR(name,     NULL, &rv_itree_##name, immbits, sign) \
    DEF_RR(name##w,  NULL, &rv_itree_##name, immbits, sign)

// Define an ALU instruction.
#define DEF_ALU_R32(name, isa, immbits, sign, funct3, ir_op2) \
    expr_tree_t const rv_itree_##name = NODE_EXPR2(ir_op2, &NODE_OPERAND_0, &NODE_OPERAND_1); \
    DEF_RR(name,     NULL, &rv_itree_##name, immbits, sign) \
    DEF_RR(name##w,  NULL, &rv_itree_##name, immbits, sign)

// Define an ALU instruction.
#define DEF_ALU_R32(name, isa, immbits, sign, funct3, ir_op2) \
    expr_tree_t const rv_itree_##name = NODE_EXPR2(ir_op2, &NODE_OPERAND_0, &NODE_OPERAND_1); \
    DEF_RI(name,     NULL, &rv_itree_##name, immbits, sign) \
    DEF_RI(name##w,  NULL, &rv_itree_##name, immbits, sign)

// Define two-operand instruction rules.
#define DEF_INSN_RULES(name, op2_is_imm, immbits, allow_s, allow_u) \
    static operand_rule_t const name[2] = { \
        {                                   \
            .location_kinds.reg = 1,        \
            .operand_kinds = {              \
                .uint = 1,                  \
                .sint = 1,                  \
            },                              \
        }, {                                \
            .const_bits = immbits,          \
            .location_kinds = {             \
                .reg = !op2_is_imm,         \
                .imm =  op2_is_imm,         \
            },                              \
            .operand_kinds = {              \
                .uint = 1,                  \
                .sint = 1,                  \
            },                              \
        },                                  \
    };

// clang-format on



// Operand rules table.
DEF_INSN_RULES(ri_operand_rules5_s, 1, 5, 1, 0)
DEF_INSN_RULES(ri_operand_rules5_su, 1, 5, 1, 1)
DEF_INSN_RULES(ri_operand_rules5_u, 1, 5, 0, 1)
DEF_INSN_RULES(ri_operand_rules6_s, 1, 6, 1, 0)
DEF_INSN_RULES(ri_operand_rules6_su, 1, 6, 1, 1)
DEF_INSN_RULES(ri_operand_rules6_u, 1, 6, 0, 1)
DEF_INSN_RULES(ri_operand_rules12_s, 1, 12, 1, 0)
DEF_INSN_RULES(ri_operand_rules12_su, 1, 12, 1, 1)
DEF_INSN_RULES(ri_operand_rules12_u, 1, 12, 0, 1)
DEF_INSN_RULES(rr_operand_rules0_s, 0, 0, 1, 0)
DEF_INSN_RULES(rr_operand_rules0_su, 0, 0, 1, 1)
DEF_INSN_RULES(rr_operand_rules0_u, 0, 0, 0, 1)
#define rr_operand_rules5_s   rr_operand_rules0_s
#define rr_operand_rules5_su  rr_operand_rules0_su
#define rr_operand_rules5_u   rr_operand_rules0_u
#define rr_operand_rules6_s   rr_operand_rules0_s
#define rr_operand_rules6_su  rr_operand_rules0_su
#define rr_operand_rules6_u   rr_operand_rules0_u
#define rr_operand_rules12_s  rr_operand_rules0_s
#define rr_operand_rules12_su rr_operand_rules0_su
#define rr_operand_rules12_u  rr_operand_rules0_u



#include "rv_instructions.inc"
