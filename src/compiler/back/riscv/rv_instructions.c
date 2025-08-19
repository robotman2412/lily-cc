
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "rv_instructions.h"

#include "match_tree.h"
#include "sub_tree.h"



// clang-format off

// Define RISC-V encoding cookie.
#define RV_COOKIE(_ext, _opcode, _enc_type, _funct3, _funct7, _funct12) \
    &(rv_encoding_t const) {    \
        .ext       = _ext,      \
        .opcode    = _opcode,   \
        .enc_type  = _enc_type, \
        .funct3    = _funct3,   \
        .funct7    = _funct7,   \
        .funct12   = _funct12,  \
    }

// Define two-operand instruction rules.
#define RV_OP_RULES2(immbits, op2_is_imm, allow_s, allow_u, is_op_32) \
    (operand_rule_t const []) {          \
        {                                \
            .location_kinds.reg = 1,     \
            .operand_kinds = {           \
                .uint = allow_u,         \
                .sint = allow_s,         \
            },                           \
            .operand_sizes = {           \
                .size32 = is_op_32,      \
                .sizeword = !(is_op_32), \
            },                           \
        }, {                             \
            .const_bits = immbits,       \
            .location_kinds = {          \
                .reg = !(op2_is_imm),    \
                .imm =  op2_is_imm,      \
            },                           \
            .operand_kinds = {           \
                .uint = allow_u,         \
                .sint = allow_s,         \
            },                           \
            .operand_sizes = {           \
                .size32 = is_op_32,      \
                .sizeword = !(is_op_32), \
            },                           \
        },                               \
    }

// Base instruction that does not come with a substitution and match tree.
#define RV_INSN_BASE(_name, ext, op_maj, funct3, funct7, funct12, encoding) \
    insn_proto_t const rv_insn_##_name = {                                         \
        .name         = #_name,                                                    \
        .cookie       = RV_COOKIE(ext, op_maj, encoding, funct3, funct7, funct12), \
    };

// Define some other instruction not common enough to have a dedicated macro.
#define RV_INSN_MISC(_name, ext, op_maj, funct3, funct7, funct12, allow_s, allow_u, encoding, _operands_len, _operands, _match_tree, _sub_tree) \
    RV_INSN_BASE(_name, ext, op_maj, funct3, funct7, funct12, encoding)            \
    insn_sub_t const rv_insn_sub_##_name = {                                       \
        .return_kinds = {                                                          \
            .uint = allow_u,                                                       \
            .sint = allow_s,                                                       \
        },                                                                         \
        .operands_len = _operands_len,                                             \
        .operands     = _operands,                                                 \
        .match_tree   = _match_tree,                                               \
        .sub_tree     = _sub_tree,                                                 \
    };

// Define an ALU instruction.
#define RV_INSN_ALU(_name, ext, op_maj, funct3, funct7, ir_op2, immbits, is_ri, allow_s, allow_u) \
    RV_INSN_BASE(_name, ext, op_maj, funct3, funct7, 0, is_ri ? RV_ENC_I : RV_ENC_R)                        \
    insn_sub_t const rv_insn_sub_##_name = {                                                                \
        .return_kinds = {                                                                                   \
            .uint = 1,                                                                                      \
            .sint = 1,                                                                                      \
        },                                                                                                  \
        .operands_len = 2,                                                                                  \
        .operands     = RV_OP_RULES2(immbits, is_ri, allow_s, allow_u, op_maj & 2),                         \
        .match_tree   = &NODE_EXPR2(ir_op2, &NODE_OPERAND_0, &NODE_OPERAND_1),                              \
        .sub_tree     = &SUB_TREE(&rv_insn_##_name, SUB_OPERAND_MATCHED(0), SUB_OPERAND_MATCHED(1)), \
    };

// Define a register-immediate ALU instruction.
#define RV_INSN_ALU_RI(name, ext, op_maj, funct3, ir_op2, immbits, allow_s, allow_u) \
    RV_INSN_ALU(name, ext, op_maj, funct3, 0, ir_op2, immbits, 1, allow_s, allow_u)

// Define a register-register ALU instruction.
#define RV_INSN_ALU_RR(name, ext, op_maj, funct3, funct7, ir_op2, allow_s, allow_u) \
    RV_INSN_ALU(name, ext, op_maj, funct3, funct7, ir_op2, 0, 0, allow_s, allow_u)

// Define an ALU comparison instruction.
#define RV_INSN_ALU_CMP(_name, ext, op_maj, funct3, ir_op2, immbits, is_ri, allow_s, allow_u) \
    RV_INSN_BASE(_name, ext, op_maj, funct3, 0, 0, is_ri ? RV_ENC_I : RV_ENC_R)                         \
    insn_sub_t const rv_insn_sub_##_name = {                                                            \
        .return_kinds = {                                                                               \
            .uint = 1,                                                                                  \
            .sint = 1,                                                                                  \
        },                                                                                              \
        .operands_len = 2,                                                                              \
        .operands     = RV_OP_RULES2(immbits, is_ri, allow_s, allow_u, op_maj & 2),                     \
        .match_tree   = &NODE_EXPR1(IR_OP1_mov, &NODE_EXPR2(ir_op2, &NODE_OPERAND_0, &NODE_OPERAND_1)), \
        .sub_tree     = &SUB_TREE(&rv_insn_##_name, SUB_OPERAND_MATCHED(0), SUB_OPERAND_MATCHED(1)),    \
    };

// Define a branch instruction.
#define RV_INSN_BRANCH(_name, ext, op_maj, funct3, ir_op2, allow_s, allow_u) \
    RV_INSN_BASE(_name, ext, op_maj, funct3, 0, 0, RV_ENC_B)

// Define a store instruction.
#define RV_INSN_STORE(_name, ext, op_maj, funct3, membits, allow_s, allow_u) \
    RV_INSN_BASE(_name, ext, op_maj, funct3, 0, 0, RV_ENC_S)            \
    insn_sub_t const rv_insn_sub_##_name = {                            \
        .return_kinds = {0},                                            \
        .return_sizes = {0},                                            \
        .operands_len = 3,                                              \
        .operands     = (operand_rule_t const []) {                     \
            {                                                           \
                .location_kinds.reg    = 1,                             \
                .operand_kinds = {                                      \
                    .uint = 1,                                          \
                    .sint = 1,                                          \
                },                                                      \
                .operand_sizes.sizeptr = 1,                             \
            }, {                                                        \
                .location_kinds.imm = 1,                                \
                .operand_kinds.sint = 1,                                \
                .const_bits         = 12,                               \
            }, {                                                        \
                .location_kinds.reg    = 1,                             \
                .operand_kinds = {                                      \
                    .uint = allow_u,                                    \
                    .sint = allow_s,                                    \
                },                                                      \
                .operand_sizes = {                                      \
                    .size8   = (membits) == 8,                          \
                    .size16  = (membits) == 16,                         \
                    .size32  = (membits) == 32,                         \
                    .size64  = (membits) == 64,                         \
                    .size128 = (membits) == 128,                        \
                },                                                      \
            }                                                           \
        },                                                              \
        .match_tree   = &NODE_STORE(                                    \
            &NODE_EXPR2(IR_OP2_add, &NODE_OPERAND_0, &NODE_OPERAND_1),  \
            &NODE_OPERAND_2                                             \
        ),                                                              \
        .sub_tree     = &SUB_TREE(                                      \
            &rv_insn_##_name,                                           \
            SUB_OPERAND_MATCHED(1),                                     \
            SUB_OPERAND_MATCHED(0),                                     \
            SUB_OPERAND_MATCHED(2)                                      \
        ),                                                              \
    };                                                                  \
    insn_sub_t const rv_insn_sub_##_name##_noadd = {                    \
        .return_kinds = {0},                                            \
        .return_sizes = {0},                                            \
        .operands_len = 2,                                              \
        .operands     = (operand_rule_t const []) {                     \
            {                                                           \
                .location_kinds.reg    = 1,                             \
                .operand_kinds = {                                      \
                    .uint = 1,                                          \
                    .sint = 1,                                          \
                },                                                      \
                .operand_sizes.sizeptr = 1,                             \
            }, {                                                        \
                .location_kinds.reg    = 1,                             \
                .operand_kinds = {                                      \
                    .uint = allow_u,                                    \
                    .sint = allow_s,                                    \
                },                                                      \
                .operand_sizes = {                                      \
                    .size8   = (membits) == 8,                          \
                    .size16  = (membits) == 16,                         \
                    .size32  = (membits) == 32,                         \
                    .size64  = (membits) == 64,                         \
                    .size128 = (membits) == 128,                        \
                },                                                      \
            }                                                           \
        },                                                              \
        .match_tree   = &NODE_STORE(&NODE_OPERAND_0, &NODE_OPERAND_1),  \
        .sub_tree     = &SUB_TREE(                                      \
            &rv_insn_##_name,                                           \
            SUB_OPERAND_CONST(IR_CONST_S16(0)),                         \
            SUB_OPERAND_MATCHED(0),                                     \
            SUB_OPERAND_MATCHED(1)                                      \
        ),                                                              \
    };

// Define a load instruction.
#define RV_INSN_LOAD(_name, ext, op_maj, funct3, membits, allow_s, allow_u) \
    RV_INSN_BASE(_name, ext, op_maj, funct3, 0, 0, RV_ENC_I)            \
    insn_sub_t const rv_insn_sub_##_name = {                            \
        .return_kinds = {                                               \
            .uint = allow_u,                                            \
            .sint = allow_s,                                            \
        },                                                              \
        .return_sizes = {                                               \
            .size8   = (membits) == 8,                                  \
            .size16  = (membits) == 16,                                 \
            .size32  = (membits) == 32,                                 \
            .size64  = (membits) == 64,                                 \
            .size128 = (membits) == 128,                                \
        },                                                              \
        .operands_len = 2,                                              \
        .operands     = (operand_rule_t const[]) {                      \
            {                                                           \
                .location_kinds.reg    = 1,                             \
                .operand_kinds = {                                      \
                    .uint = 1,                                          \
                    .sint = 1,                                          \
                },                                                      \
                .operand_sizes.sizeptr = 1,                             \
            }, {                                                        \
                .const_bits            = 12,                            \
                .operand_kinds.sint    = 1,                             \
                .location_kinds.imm    = 1,                             \
            }                                                           \
        },                                                              \
        .match_tree   = &NODE_LOAD(                                     \
            &NODE_EXPR2(IR_OP2_add, &NODE_OPERAND_0, &NODE_OPERAND_1)   \
        ),                                                              \
        .sub_tree     = &SUB_TREE(                                      \
            &rv_insn_##_name,                                           \
            SUB_OPERAND_MATCHED(1),                                     \
            SUB_OPERAND_MATCHED(0)                                      \
        ),                                                              \
    };                                                                  \
    insn_sub_t const rv_insn_sub_##_name##_noadd = {                    \
        .return_kinds = {                                               \
            .uint = allow_u,                                            \
            .sint = allow_s,                                            \
        },                                                              \
        .return_sizes = {                                               \
            .size8   = (membits) == 8,                                  \
            .size16  = (membits) == 16,                                 \
            .size32  = (membits) == 32,                                 \
            .size64  = (membits) == 64,                                 \
            .size128 = (membits) == 128,                                \
        },                                                              \
        .operands_len = 1,                                              \
        .operands     = (operand_rule_t const[]) {                      \
            {                                                           \
                .location_kinds.reg    = 1,                             \
                .operand_kinds = {                                      \
                    .uint = 1,                                          \
                    .sint = 1,                                          \
                },                                                      \
                .operand_sizes.sizeptr = 1,                             \
            }                                                           \
        },                                                              \
        .match_tree   = &NODE_LOAD(&NODE_OPERAND_0),                    \
        .sub_tree     = &SUB_TREE(                                      \
            &rv_insn_##_name,                                           \
            SUB_OPERAND_CONST(IR_CONST_S16(0)),                         \
            SUB_OPERAND_MATCHED(0)                                      \
        ),                                                              \
    };

#define RV_INSN_SUB_BRANCH(_name, _insn, ir_op2, _1, _2) \
    insn_sub_t const rv_insn_sub_##_name = {                                                                                   \
        .operands_len = 3,                                                                                                     \
        .operands     = (operand_rule_t const []) {                                                                            \
            {                                                                                                                  \
                .location_kinds.mem_abs    = 1,                                                                                \
                .location_kinds.mem_regrel = 1,                                                                                \
                .operand_kinds.val         = -1,                                                                               \
                .operand_sizes.sizeword    = -1,                                                                               \
            }, {                                                                                                               \
                .location_kinds.reg     = 1,                                                                                   \
                .operand_kinds.sint     = 1,                                                                                   \
                .operand_sizes.sizeword = 1,                                                                                   \
            }, {                                                                                                               \
                .location_kinds.reg     = 1,                                                                                   \
                .operand_kinds.sint     = 1,                                                                                   \
                .operand_sizes.sizeword = 1,                                                                                   \
            },                                                                                                                 \
        },                                                                                                                     \
        .match_tree   = &NODE_BRANCH(&NODE_OPERAND_0, &NODE_EXPR2(ir_op2, &NODE_OPERAND_1, &NODE_OPERAND_2)),                  \
        .sub_tree     = &SUB_TREE(&rv_insn_##_insn, SUB_OPERAND_MATCHED(0), SUB_OPERAND_MATCHED(_1), SUB_OPERAND_MATCHED(_2)), \
    };

// clang-format on

#include "rv_instructions.inc"

insn_sub_t const rv_insn_sub_branch = {
    .operands_len = 2,
    .operands     = (operand_rule_t const[]){{
            .operand_kinds.bool_    = 1,
            .location_kinds.reg     = 1,
            .operand_sizes.sizeword = 1,
    }},
    .match_tree   = &NODE_BRANCH(&NODE_OPERAND_0, &NODE_OPERAND_1),
    .sub_tree
    = &SUB_TREE(&rv_insn_bne, SUB_OPERAND_MATCHED(0), SUB_OPERAND_MATCHED(1), SUB_OPERAND_CONST(IR_CONST_S32(0))),
};

RV_INSN_SUB_BRANCH(beq, beq, IR_OP2_seq, 1, 2)
RV_INSN_SUB_BRANCH(bne, bne, IR_OP2_sne, 1, 2)
RV_INSN_SUB_BRANCH(blt, blt, IR_OP2_slt, 1, 2)
RV_INSN_SUB_BRANCH(bge, bge, IR_OP2_sge, 1, 2)
RV_INSN_SUB_BRANCH(bgt, blt, IR_OP2_sgt, 2, 1)
RV_INSN_SUB_BRANCH(ble, bge, IR_OP2_sle, 2, 1)



// Table of supported RISC-V instructions.
insn_proto_t const *const rv_insns[] = {
#define RV_INSN_BASE(name, ...) &rv_insn_##name,
#include "rv_instructions.inc"
};

// Number of supported RISC-V instructions.
size_t const rv_insns_len = sizeof(rv_insns) / sizeof(insn_proto_t const *);

// Table of supported RISC-V substitution patterns.
insn_sub_t const *const rv_insn_subs[] = {
#define RV_INSN_MISC(name, ...)  &rv_insn_sub_##name,
#define RV_INSN_LOAD(name, ...)  &rv_insn_sub_##name, &rv_insn_sub_##name##_noadd,
#define RV_INSN_STORE(name, ...) &rv_insn_sub_##name, &rv_insn_sub_##name##_noadd,
#define RV_INSN_BRANCH(...)
#include "rv_instructions.inc"
    &rv_insn_sub_branch,
    &rv_insn_sub_beq,
    &rv_insn_sub_bne,
    &rv_insn_sub_blt,
    &rv_insn_sub_bge,
    &rv_insn_sub_bgt,
    &rv_insn_sub_ble,
};

// Number of supported RISC-V substitution patterns.
size_t const rv_insn_subs_len = sizeof(rv_insn_subs) / sizeof(insn_sub_t const *);
