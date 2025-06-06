
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "rv_instructions.h"



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

// Define operand rules for store instructions.
#define RV_OP_RULES_S(membits, allow_s, allow_u) \
    (operand_rule_t const []) {              \
        {                                    \
            .location_kinds.imm = 1,         \
            .operand_kinds.sint = 1,         \
            .const_bits         = 12,        \
        }, {                                 \
            .location_kinds.reg    = 1,      \
            .operand_kinds.uint    = 1,      \
            .operand_sizes.sizeptr = 1,      \
        }, {                                 \
            .location_kinds.reg    = 1,      \
            .operand_kinds = {               \
                .uint = allow_u,             \
                .sint = allow_s,             \
            },                               \
            .operand_sizes = {               \
                .size8   = (membits) == 8,   \
                .size16  = (membits) == 16,  \
                .size32  = (membits) == 32,  \
                .size64  = (membits) == 64,  \
                .size128 = (membits) == 128, \
            },                               \
        }                                    \
    }

// Define operand rules for load instructions.
operand_rule_t const RV_OP_RULES_L[] = {
    {
        .location_kinds.imm = 1,
        .operand_kinds.sint = 1,
        .const_bits         = 12,
    }, {
        .location_kinds.reg    = 1,
        .operand_kinds.uint    = 1,
        .operand_sizes.sizeptr = 1,
    }
};

// Define some other intstruction not common enough to have a dedicated macro.
#define RV_INSN_MISC(name, ext, op_maj, funct3, funct7, funct12, allow_s, allow_u, encoding, _operands_len, _operands, _tree) \
    insn_proto_t const rv_insn_##name = {                                          \
        .cookie       = RV_COOKIE(ext, op_maj, encoding, funct3, funct7, funct12), \
        .return_kinds = {                                                          \
            .uint = allow_u,                                                       \
            .sint = allow_s,                                                       \
        },                                                                         \
        .operands_len = _operands_len,                                             \
        .operands     = _operands,                                                 \
        .tree         = _tree,                                                     \
    };

// Define an ALU instruction.
#define RV_INSN_ALU(name, ext, op_maj, funct3, funct7, ir_op2, immbits, is_ri, allow_s, allow_u) \
    insn_proto_t const rv_insn_##name = {                                                       \
        .cookie       = RV_COOKIE(ext, op_maj, is_ri ? RV_ENC_I : RV_ENC_R, funct3, funct7, 0), \
        .return_kinds = {                                                                       \
            .uint = 1,                                                                          \
            .sint = 1,                                                                          \
        },                                                                                      \
        .operands_len = 2,                                                                      \
        .operands     = RV_OP_RULES2(immbits, is_ri, allow_s, allow_u, op_maj & 2),             \
        .tree         = &NODE_EXPR2(ir_op2, &NODE_OPERAND_0, &NODE_OPERAND_1),                  \
    };

// Define a register-immediate ALU instruction.
#define RV_INSN_ALU_RI(name, ext, op_maj, funct3, ir_op2, immbits, allow_s, allow_u) \
    RV_INSN_ALU(name, ext, op_maj, funct3, 0, ir_op2, immbits, 1, allow_s, allow_u)

// Define a register-register ALU instruction.
#define RV_INSN_ALU_RR(name, ext, op_maj, funct3, funct7, ir_op2, allow_s, allow_u) \
    RV_INSN_ALU(name, ext, op_maj, funct3, funct7, ir_op2, 0, 0, allow_s, allow_u)

// Define an ALU comparison instruction.
#define RV_INSN_ALU_CMP(name, ext, op_maj, funct3, ir_op2, immbits, is_ri, allow_s, allow_u) \
    insn_proto_t const rv_insn_##name = {                                                               \
        .cookie       = RV_COOKIE(ext, op_maj, is_ri ? RV_ENC_I : RV_ENC_R, funct3, 0, 0),              \
        .return_kinds = {                                                                               \
            .uint = 1,                                                                                  \
            .sint = 1,                                                                                  \
        },                                                                                              \
        .operands_len = 2,                                                                              \
        .operands     = RV_OP_RULES2(immbits, is_ri, allow_s, allow_u, op_maj & 2),                     \
        .tree         = &NODE_EXPR1(IR_OP1_mov, &NODE_EXPR2(ir_op2, &NODE_OPERAND_0, &NODE_OPERAND_1)), \
    };

// Define a branch instruction.
#define RV_INSN_BRANCH(name, ext, op_maj, funct3, ir_op2, allow_s, allow_u) \
    insn_proto_t const rv_insn_##name = {                                                    \
        .cookie       = RV_COOKIE(ext, op_maj, RV_ENC_B, funct3, 0, 0),                      \
        .operands_len = 2,                                                                   \
        .operands     = RV_OP_RULES2(12, 0, allow_s, allow_u, 0),                            \
        .tree         = &NODE_BRANCH(&NODE_EXPR2(ir_op2, &NODE_OPERAND_0, &NODE_OPERAND_1)), \
    };

// Define a store instruction.
#define RV_INSN_STORE(name, ext, op_maj, funct3, membits, allow_s, allow_u) \
    insn_proto_t const rv_insn_##name = {                                                                        \
        .cookie       = RV_COOKIE(ext, op_maj, RV_ENC_S, funct3, 0, 0),                                          \
        .operands_len = 3,                                                                                       \
        .operands     = RV_OP_RULES_S(membits, allow_s, allow_u),                                                \
        .tree         = &NODE_STORE(&NODE_EXPR2(IR_OP2_add, &NODE_OPERAND_0, &NODE_OPERAND_1), &NODE_OPERAND_2), \
    };

// Define a load instruction.
#define RV_INSN_LOAD(name, ext, op_maj, funct3, membits, allow_s, allow_u) \
    insn_proto_t const rv_insn_##name = {                                                      \
        .cookie       = RV_COOKIE(ext, op_maj, RV_ENC_I, funct3, 0, 0),                        \
        .return_kinds = {                                                                      \
            .uint = allow_u,                                                                   \
            .sint = allow_s,                                                                   \
        },                                                                                     \
        .return_sizes = {                                                                      \
            .size8   = (membits) == 8,                                                         \
            .size16  = (membits) == 16,                                                        \
            .size32  = (membits) == 32,                                                        \
            .size64  = (membits) == 64,                                                        \
            .size128 = (membits) == 128,                                                       \
        },                                                                                     \
        .operands_len = 2,                                                                     \
        .operands     = RV_OP_RULES_L,                                                         \
        .tree         = &NODE_LOAD(&NODE_EXPR2(IR_OP2_add, &NODE_OPERAND_0, &NODE_OPERAND_1)), \
    };

// clang-format on



operand_rule_t const rv_jal_rules[] = {{
    .location_kinds.imm = 1,
    .operand_kinds.sint = 1,
    .const_bits         = 21,
}};
expr_tree_t const    rv_jal_tree    = NODE_CALL_PTR(&NODE_OPERAND_0);

operand_rule_t const rv_jalr_rules[] = {
    {
        .location_kinds.imm = 1,
        .operand_kinds.sint = 1,
        .const_bits         = 12,
    },
    {
        .location_kinds.reg    = 1,
        .operand_kinds.uint    = 1,
        .operand_kinds.sint    = 1,
        .operand_sizes.sizeptr = 1,
    },
};
expr_tree_t const rv_jalr_tree = NODE_CALL_PTR(&NODE_EXPR2(IR_OP2_add, &NODE_OPERAND_0, &NODE_OPERAND_1));

operand_rule_t const rv_li_rules[] = {{
    .location_kinds.imm = 1,
    .operand_kinds.sint = 1,
    .const_bits         = 32,
}};
expr_tree_t const    rv_li_tree    = NODE_EXPR1(IR_OP1_mov, &NODE_OPERAND_0);

expr_tree_t const rv_ret_tree = {
    .type = EXPR_TREE_IR_INSN,
    .expr = {
        .insn_type = IR_INSN_FLOW,
        .flow = {
            .type = IR_FLOW_RETURN,
        },
    },
};



#include "rv_instructions.inc"



// clang-format off

// Table of supported RISC-V instructions.
insn_proto_t const *const riscv_insns[] = {
    #define RV_INSN_MISC(name, ...) \
        &rv_insn_##name,
#include "rv_instructions.inc"
};

// clang-format on
