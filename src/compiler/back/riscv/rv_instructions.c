
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "rv_instructions.h"

#include "asm_print.h"
#include "insn_proto.h"
#include "ir_types.h"
#include "rv_backend.h"
#include "unreachable.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>



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

// Print instruction for the assembler.
void rv_asm_print_insn(backend_profile_t *profile, ir_insn_t const *insn, FILE *to) {
    (void)profile;
    insn_proto_t const  *proto = insn->prototype;
    rv_encoding_t const *enc   = insn->prototype->cookie;

    bool force_ret;
    switch (enc->enc_type) {
        case RV_ENC_R:
        case RV_ENC_I:
        case RV_ENC_U:
        case RV_ENC_J:
        case RV_ENC_PSEUDO_MV:
        case RV_ENC_PSEUDO_LI: force_ret = true; break;
        case RV_ENC_S:
        case RV_ENC_B:
        case RV_ENC_BITS:
        case RV_ENC_PSEUDO_RET:
        case RV_ENC_PSEUDO_J:
        case RV_ENC_PSEUDO_JR: force_ret = false; break;
    }
    bool is_branchy = proto == &rv_insn_j || proto == &rv_insn_jr || proto == &rv_insn_jal || proto == &rv_insn_jalr
                      || enc->enc_type == RV_ENC_B;

    fputs(proto->name, to);

    bool delim = false;
    if (insn->returns_len || force_ret) {
        fputc(' ', to);
        delim = true;
        if (insn->returns_len) {
            assert(insn->returns_len == 1);
            assert(insn->returns[0].type == IR_RETVAL_TYPE_REG);
            fputs(rv_reg_names[insn->returns[0].dest_regno], to);
        } else {
            fputs("zero", to);
        }
    }

    for (size_t i = 0; i < insn->operands_len; i++) {
        fputs(delim ? ", " : " ", to);
        delim = true;

        ir_operand_t operand = insn->operands[i];
        if (operand.type == IR_OPERAND_TYPE_MEM) {
            switch (operand.mem.base_type) {
                case IR_MEMBASE_ABS: fprintf(to, "%" PRId64, operand.mem.offset); break;
                case IR_MEMBASE_SYM: fputs(operand.mem.base_sym, to); break;
                case IR_MEMBASE_FRAME:
                    assert(!is_branchy);
                    // TODO: Compute proper offsets.
                    fprintf(to, "%" PRId64 "(sp)", operand.mem.offset + operand.mem.base_frame->offset);
                    break;
                case IR_MEMBASE_CODE: asm_print_code_label(operand.mem.base_code, to); break;
                case IR_MEMBASE_VAR: UNREACHABLE();
                case IR_MEMBASE_REG:
                    if (is_branchy) {
                        assert(operand.mem.offset == 0);
                        fputs(rv_reg_names[operand.mem.base_regno], to);
                    } else {
                        fprintf(to, "%" PRId64 "(%s)", operand.mem.offset, rv_reg_names[operand.mem.base_regno]);
                    }
                    break;
            }
        } else if (operand.type == IR_OPERAND_TYPE_CONST) {
            ir_const_t iconst = operand.iconst;
            switch (iconst.prim_type) {
                case IR_PRIM_bool:
                case IR_PRIM_u8:
                case IR_PRIM_u16:
                case IR_PRIM_u32:
                case IR_PRIM_u64: fprintf(to, "%" PRIu64, iconst.constl); break;

                case IR_PRIM_s8:
                case IR_PRIM_s16:
                case IR_PRIM_s32:
                case IR_PRIM_s64: fprintf(to, "%" PRId64, (int64_t)iconst.constl); break;

                case IR_PRIM_f32: // TODO.
                case IR_PRIM_f64: // TODO.
                case IR_PRIM_s128:
                case IR_PRIM_u128:
                case IR_N_PRIM: UNREACHABLE();
            }
        } else {
            assert(operand.type == IR_OPERAND_TYPE_REG);
            fputs(rv_reg_names[operand.regno], to);
        }
    }
}
