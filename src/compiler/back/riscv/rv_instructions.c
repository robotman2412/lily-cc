
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

// Define a generic RISC-V instruction.
#define RV_INSN_BASE(_name, ext, op_maj, funct3, funct7, funct12, encoding) \
    insn_proto_t const rv_insn_##_name = {                                         \
        .name         = #_name,                                                    \
        .cookie       = RV_COOKIE(ext, op_maj, encoding, funct3, funct7, funct12), \
    };

// Define some other instruction not common enough to have a dedicated macro.
#define RV_INSN_MISC(_name, ext, op_maj, funct3, funct7, funct12, allow_s, allow_u, encoding, _operands_len, _operands, _match_tree, _sub_tree) \
    RV_INSN_BASE(_name, ext, op_maj, funct3, funct7, funct12, encoding)    

// Define an ALU instruction.
#define RV_INSN_ALU(_name, ext, op_maj, funct3, funct7, ir_op2, immbits, is_ri, allow_s, allow_u) \
    RV_INSN_BASE(_name, ext, op_maj, funct3, funct7, 0, is_ri ? RV_ENC_I : RV_ENC_R) 

// Define a register-immediate ALU instruction.
#define RV_INSN_ALU_RI(name, ext, op_maj, funct3, funct7, ir_op2, immbits, allow_s, allow_u) \
    RV_INSN_ALU(name, ext, op_maj, funct3, 0, ir_op2, immbits, 1, allow_s, allow_u)

// Define a register-register ALU instruction.
#define RV_INSN_ALU_RR(name, ext, op_maj, funct3, funct7, ir_op2, allow_s, allow_u) \
    RV_INSN_ALU(name, ext, op_maj, funct3, funct7, ir_op2, 0, 0, allow_s, allow_u)

// Define a branch instruction.
#define RV_INSN_BRANCH(_name, ext, op_maj, funct3, ir_op2, allow_s, allow_u) \
    RV_INSN_BASE(_name, ext, op_maj, funct3, 0, 0, RV_ENC_B)

// Define a store instruction.
#define RV_INSN_STORE(_name, ext, op_maj, funct3, membits, allow_s, allow_u) \
    RV_INSN_BASE(_name, ext, op_maj, funct3, 0, 0, RV_ENC_S)

// Define a load instruction.
#define RV_INSN_LOAD(_name, ext, op_maj, funct3, membits, allow_s, allow_u) \
    RV_INSN_BASE(_name, ext, op_maj, funct3, 0, 0, RV_ENC_I)

// clang-format on

#include "rv_instructions.inc"



// Table of supported RISC-V instructions.
insn_proto_t const *const rv_insns[] = {
#define RV_INSN_BASE(name, ...) &rv_insn_##name,
#include "rv_instructions.inc"
};

// Number of supported RISC-V instructions.
size_t const rv_insns_len = sizeof(rv_insns) / sizeof(insn_proto_t const *);
