
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "backend.h"
#include "insn_proto.h"
#include "ir.h"
#include "ir_types.h"
#include "rv_misc.h"

#include <stddef.h>
#include <stdint.h>



// RISC-V major opcodes.
typedef enum {
    /* clang-format off */
    RV_OP_LOAD,   RV_OP_LOAD_FP,  RV_OP__cust0, RV_OP_MISC_MEM, RV_OP_OP_IMM, RV_OP_AUIPC, RV_OP_OP_IMM_32, RV_OP__48b,
    RV_OP_STORE,  RV_OP_STORE_FP, RV_OP__cust1, RV_OP_AMO,      RV_OP_OP,     RV_OP_LUI,   RV_OP_OP_32,     RV_OP__64b,
    RV_OP_MADD,   RV_OP_MSUB,     RV_OP_NMSUB,  RV_OP_NMADD,    RV_OP_OP_FP,  RV_OP_OP_V,  RV_OP__cust2,    RV_OP__48b_2,
    RV_OP_BRANCH, RV_OP_JALR,     RV_OP__resv0, RV_OP_JAL,      RV_OP_SYSTEM, RV_OP_OP_VE, RV_OP__cust3,    RV_OP__80b,
    /* clang-format on */
    RV_OP_INVALID,
} rv_opcode_t;

// Types of RISC-V instruction encoding.
typedef enum {
    // R-type instruction.
    RV_ENC_R,
    // I-type instruction.
    RV_ENC_I,
    // S-type instruction.
    RV_ENC_S,
    // B-type instruction.
    RV_ENC_B,
    // U-type instruction.
    RV_ENC_U,
    // J-type instruction.
    RV_ENC_J,
    // Instructions without registers; for some reason the RISC-V spec doesn't give a name to this.
    RV_ENC_BITS,
    // The `mv` pseudo-instruction.
    RV_ENC_PSEUDO_MV,
    // The `li` pseudo-instruction.
    RV_ENC_PSEUDO_LI,
    // The `ret` pseudo-instruction.
    RV_ENC_PSEUDO_RET,
    // The `j` pseudo-instruction.
    RV_ENC_PSEUDO_J,
    // The `jr` pseudo-instruction.
    RV_ENC_PSEUDO_JR,
    // The `la` pseudo-instruction.
    RV_ENC_PSEUDO_LA,
} rv_enc_type_t;



// RISC-V instruction encoding.
typedef struct rv_encoding rv_encoding_t;



// RISC-V instruction encoding.
struct rv_encoding {
    // Major opcode.
    rv_opcode_t   opcode;
    // Instruction encoding type.
    rv_enc_type_t enc_type;
    // Function bits to set.
    uint8_t       funct3, funct7;
    // Function bits to set.
    uint16_t      funct12;
};



// Table of supported RISC-V instructions.
extern insn_proto_t const *const rv_insns[];
// Number of supported RISC-V instructions.
extern size_t const              rv_insns_len;

// clang-format off
#define RV_INSN_BASE(name, ...) \
    extern insn_proto_t const rv_insn_##name;
#include "rv_instructions.inc"
// clang-format on



#define RV_DEFINE_ADD_RI(name)                                                                                         \
    static inline ir_insn_t *rv_add_##name(ir_insnloc_t loc, ir_retval_t dest, ir_operand_t lhs, int rhs) {            \
        return ir_add_mach_insn(                                                                                       \
            loc,                                                                                                       \
            true,                                                                                                      \
            dest,                                                                                                      \
            &rv_insn_##name,                                                                                           \
            2,                                                                                                         \
            (ir_operand_t const[]){lhs, IR_OPERAND_CONST(IR_CONST_S16(rhs))}                                           \
        );                                                                                                             \
    }
#define RV_DEFINE_ADD_RR(name)                                                                                         \
    static inline ir_insn_t *rv_add_##name(ir_insnloc_t loc, ir_retval_t dest, ir_operand_t lhs, ir_operand_t rhs) {   \
        return ir_add_mach_insn(loc, true, dest, &rv_insn_##name, 2, (ir_operand_t const[]){lhs, rhs});                \
    }
#define RV_DEFINE_ADD_I(name)                                                                                          \
    static inline ir_insn_t *rv_add_##name(ir_insnloc_t loc, ir_retval_t dest, int val) {                              \
        return ir_add_mach_insn(                                                                                       \
            loc,                                                                                                       \
            true,                                                                                                      \
            dest,                                                                                                      \
            &rv_insn_##name,                                                                                           \
            1,                                                                                                         \
            (ir_operand_t const[]){IR_OPERAND_CONST(IR_CONST_S64(val))}                                                \
        );                                                                                                             \
    }
#define RV_DEFINE_ADD_R(name)                                                                                          \
    static inline ir_insn_t *rv_add_##name(ir_insnloc_t loc, ir_retval_t dest, ir_operand_t val) {                     \
        return ir_add_mach_insn(loc, true, dest, &rv_insn_##name, 1, (ir_operand_t const[]){val});                     \
    }

RV_DEFINE_ADD_R(mv)
RV_DEFINE_ADD_I(li)

#define RV_INSN_ALU_RI(name, op_maj, funct3, funct7) RV_DEFINE_ADD_RI(name)
#define RV_INSN_ALU_RR(name, op_maj, funct3, funct7) RV_DEFINE_ADD_RR(name)
#include "rv_instructions.inc"

// Print instruction for the assembler.
void rv_asm_print_insn(backend_profile_t *profile, ir_insn_t const *insn, FILE *to);
