
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

// clang-format off



#ifndef RV_INSN_MISC
// Define some other intstruction not common enough to have a dedicated macro.
#define RV_INSN_MISC(name, ext, op_maj, funct3, funct7, funct12, allow_s, allow_u, encoding, n_operand, operand_rules, tree)
#endif

#ifndef RV_INSN_PSEUDO
// Define a pseudo-instruction.
#define RV_INSN_PSEUDO(name, ext, allow_s, allow_u, encoding, n_operand, operand_rules, tree) \
    RV_INSN_MISC(name, ext, 0, 0, 0, 0, allow_s, allow_u, encoding, n_operand, operand_rules, tree)
#endif

#ifndef RV_INSN_NOIR
// Define an instruction without an IR representation.
#define RV_INSN_NOIR(name, ext, op_maj, funct3, funct7, funct12, encoding) \
    RV_INSN_MISC(name, ext, op_maj, funct3, funct7, funct12, 0, 0, encoding, 0, NULL, NULL)
#endif

#ifndef RV_INSN_ALU_RI
// Define a register-immediate ALU instruction.
#define RV_INSN_ALU_RI(name, ext, op_maj, funct3, ir_op2, immbits, allow_s, allow_u) \
    RV_INSN_MISC(name, ext, op_maj, funct3, 0, 0, allow_s, allow_u, RV_ENC_I, 0, NULL, NULL)
#endif

#ifndef RV_INSN_ALU_RR
// Define a register-register ALU instruction.
#define RV_INSN_ALU_RR(name, ext, op_maj, funct3, funct7, ir_op2, allow_s, allow_u) \
    RV_INSN_MISC(name, ext, op_maj, funct3, funct7, 0, allow_s, allow_u, RV_ENV_R, 0, NULL, NULL)
#endif

#ifndef RV_INSN_ALU
// Define an ALU instruction.
#define RV_INSN_ALU(name, ext, op_maj, funct3, funct7, ir_op2, immbits, is_ri, allow_s, allow_u) \
    RV_INSN_MISC(name, ext, op_maj, funct3, funct7, 0, allow_s, allow_u, (op_maj) & 2 ? RV_ENC_I : RV_ENV_R, 0, NULL, NULL)
#endif

#ifndef RV_INSN_ALU_CMP
// Define an ALU comparison instruction.
#define RV_INSN_ALU_CMP(name, ext, op_maj, funct3, ir_op2, immbits, is_ri, allow_s, allow_u) \
    RV_INSN_MISC(name, ext, op_maj, funct3, 0, 0, allow_s, allow_u, (op_maj) & 2 ? RV_ENC_I : RV_ENV_R, 0, NULL, NULL)
#endif

#ifndef RV_INSN_BRANCH
// Define a branch instruction.
#define RV_INSN_BRANCH(name, ext, op_maj, funct3, ir_op2, allow_s, allow_u) \
    RV_INSN_MISC(name, ext, op_maj, funct3, 0, 0, allow_s, allow_u, RV_ENC_I, 0, NULL, NULL)
#endif

#ifndef RV_INSN_STORE
// Define a store instruction.
#define RV_INSN_STORE(name, ext, op_maj, funct3, bits, allow_s, allow_u) \
    RV_INSN_MISC(name, ext, op_maj, funct3, 0, 0, allow_s, allow_u, RV_ENC_S, 0, NULL, NULL)
#endif

#ifndef RV_INSN_LOAD
// Define a load instruction.
#define RV_INSN_LOAD(name, ext, op_maj, funct3, membits, allow_s, allow_u) \
    RV_INSN_MISC(name, ext, op_maj, funct3, 0, 0, allow_s, allow_u, RV_ENC_I, 0, NULL, NULL)
#endif



/* ==== Pseudo-instructions ==== */

//              [ name ]  [ ext ]  [   opcode   ]  [f3 ]  [funct7 ]  [ ir op  ]      imm  ri s  u  [   encoding    ]
RV_INSN_PSEUDO( li,       RV_BASE,                                                           1, 1, RV_ENC_PSEUDO_LI,  1, rv_li_rules,   &rv_li_tree)
RV_INSN_PSEUDO( ret,      RV_BASE,                                                           0, 0, RV_ENC_PSEUDO_RET, 0, NULL,          &rv_ret_tree)
RV_INSN_PSEUDO( j,        RV_BASE,                                                           0, 0, RV_ENC_PSEUDO_J,   1, rv_jal_rules,  &rv_jal_tree)
RV_INSN_PSEUDO( jr,       RV_BASE,                                                           0, 0, RV_ENC_PSEUDO_JR,  1, rv_jalr_rules, &rv_jalr_tree)

/* ==== RV32I instructions ==== */

//              [ name ]  [ ext ]  [   opcode   ]  [f3 ]  [funct7 ]  [ ir op  ]      imm  ri s  u  [   encoding    ]
RV_INSN_NOIR(   lui,      RV_BASE, RV_OP_LUI,      0b000, 0b0000000, 0b000000000000,               RV_ENC_U)
RV_INSN_NOIR(   auipc,    RV_BASE, RV_OP_LUI,      0b000, 0b0000000, 0b000000000000,               RV_ENC_U)
RV_INSN_MISC(   jal,      RV_BASE, RV_OP_JAL,      0b000, 0b0000000, 0b000000000000,         1, 0, RV_ENC_J,          1, rv_jal_rules,  &rv_jal_tree)
RV_INSN_MISC(   jalr,     RV_BASE, RV_OP_JALR,     0b000, 0b0000000, 0b000000000000,         1, 0, RV_ENC_I,          2, rv_jalr_rules, &rv_jalr_tree)
RV_INSN_LOAD(   lb,       RV_BASE, RV_OP_LOAD,     0b000,                              8,    1, 0)
RV_INSN_LOAD(   lh,       RV_BASE, RV_OP_LOAD,     0b000,                             16,    1, 0)
RV_INSN_LOAD(   lw,       RV_BASE, RV_OP_LOAD,     0b000,                             32,    1, 0)
RV_INSN_LOAD(   lbu,      RV_BASE, RV_OP_LOAD,     0b000,                              8,    0, 1)
RV_INSN_LOAD(   lhu,      RV_BASE, RV_OP_LOAD,     0b000,                             16,    0, 1)
RV_INSN_STORE(  sb,       RV_BASE, RV_OP_STORE,    0b000,                              8,    1, 1)
RV_INSN_STORE(  sh,       RV_BASE, RV_OP_STORE,    0b001,                             16,    1, 1)
RV_INSN_STORE(  sw,       RV_BASE, RV_OP_STORE,    0b010,                             32,    1, 1)
RV_INSN_BRANCH( beq,      RV_BASE, RV_OP_BRANCH,   0b000,            IR_OP2_seq,             1, 1)
RV_INSN_BRANCH( bne,      RV_BASE, RV_OP_BRANCH,   0b001,            IR_OP2_seq,             1, 1)
RV_INSN_BRANCH( blt,      RV_BASE, RV_OP_BRANCH,   0b100,            IR_OP2_slt,             1, 0)
RV_INSN_BRANCH( bge,      RV_BASE, RV_OP_BRANCH,   0b101,            IR_OP2_slt,             1, 0)
RV_INSN_BRANCH( bltu,     RV_BASE, RV_OP_BRANCH,   0b110,            IR_OP2_slt,             0, 1)
RV_INSN_BRANCH( bgeu,     RV_BASE, RV_OP_BRANCH,   0b111,            IR_OP2_slt,             0, 1)
RV_INSN_ALU_RI( addi,     RV_BASE, RV_OP_OP_IMM,   0b000,            IR_OP2_add,      12,    1, 1)
RV_INSN_ALU_CMP(slti,     RV_BASE, RV_OP_OP_IMM,   0b010,            IR_OP2_slt,      12, 1, 1, 0)
RV_INSN_ALU_CMP(sltiu,    RV_BASE, RV_OP_OP_IMM,   0b011,            IR_OP2_slt,      12, 1, 0, 1)
RV_INSN_ALU_RI( xori,     RV_BASE, RV_OP_OP_IMM,   0b100,            IR_OP2_bxor,     12,    1, 1)
RV_INSN_ALU_RI( ori,      RV_BASE, RV_OP_OP_IMM,   0b110,            IR_OP2_bor,      12,    1, 1)
RV_INSN_ALU_RI( andi,     RV_BASE, RV_OP_OP_IMM,   0b111,            IR_OP2_band,     12,    1, 1)
RV_INSN_ALU_RI( slli,     RV_BASE, RV_OP_OP_IMM,   0b111,            IR_OP2_shl,       7,    1, 1)
RV_INSN_ALU(    srli,     RV_BASE, RV_OP_OP_IMM,   0b111, 0b0000000, IR_OP2_shl,       7, 1, 0, 1)
RV_INSN_ALU(    srai,     RV_BASE, RV_OP_OP_IMM,   0b111, 0b0100000, IR_OP2_shl,       7, 1, 1, 0)
RV_INSN_ALU_RR( add,      RV_BASE, RV_OP_OP,       0b000, 0b0000000, IR_OP2_add,             1, 1)
RV_INSN_ALU_RR( sub,      RV_BASE, RV_OP_OP,       0b000, 0b0100000, IR_OP2_sub,             1, 1)
RV_INSN_ALU_RR( sll,      RV_BASE, RV_OP_OP,       0b001, 0b0000000, IR_OP2_shl,             1, 1)
RV_INSN_ALU_RR( slt,      RV_BASE, RV_OP_OP,       0b010, 0b0000000, IR_OP2_slt,             1, 0)
RV_INSN_ALU_RR( sltu,     RV_BASE, RV_OP_OP,       0b011, 0b0000000, IR_OP2_slt,             0, 1)
RV_INSN_ALU_RR( xor,      RV_BASE, RV_OP_OP,       0b100, 0b0000000, IR_OP2_bxor,            1, 1)
RV_INSN_ALU_CMP(srl,      RV_BASE, RV_OP_OP,       0b101, 0b0000000, IR_OP2_shr,          0, 0, 1)
RV_INSN_ALU_CMP(sra,      RV_BASE, RV_OP_OP,       0b101, 0b0100000, IR_OP2_shr,          0, 1, 0)
RV_INSN_ALU_RR( or,       RV_BASE, RV_OP_OP,       0b110, 0b0000000, IR_OP2_bor,             1, 1)
RV_INSN_ALU_RR( and,      RV_BASE, RV_OP_OP,       0b111, 0b0000000, IR_OP2_band,            1, 1)
RV_INSN_NOIR(   fence,    RV_BASE, RV_OP_MISC_MEM, 0b000, 0b0000000, 0b000000000000,               RV_ENC_I)
RV_INSN_NOIR(   ecall,    RV_BASE, RV_OP_SYSTEM,   0b000, 0b0000000, 0b000000000000,               RV_ENC_BITS)
RV_INSN_NOIR(   ebreak,   RV_BASE, RV_OP_SYSTEM,   0b000, 0b0000000, 0b000000000001,               RV_ENC_BITS)

#undef RV_INSN_MISC
#undef RV_INSN_PSEUDO
#undef RV_INSN_NOIR
#undef RV_INSN_ALU_RI
#undef RV_INSN_ALU_RR
#undef RV_INSN_ALU
#undef RV_INSN_ALU_CMP
#undef RV_INSN_BRANCH
#undef RV_INSN_STORE
#undef RV_INSN_LOAD
