
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

// clang-format off



#ifndef RV_INSN_BASE
// Base instruction that does not fit any other category.
#define RV_INSN_BASE(name, op_maj, funct3, funct7, funct12, encoding)
#endif

#ifndef RV_INSN_PSEUDO
// Pseudo-instructions.
#define RV_INSN_PSEUDO(name, encoding) \
    RV_INSN_BASE(name, 0, 0, 0, 0, encoding)
#endif

#ifndef RV_INSN_ALU_RI
// Define a register-immediate ALU instruction.
#define RV_INSN_ALU_RI(name, op_maj, funct3, funct7) \
    RV_INSN_BASE(name, op_maj, funct3, funct7, 0, RV_ENC_I)
#endif

#ifndef RV_INSN_ALU_RR
// Define a register-register ALU instruction.
#define RV_INSN_ALU_RR(name, op_maj, funct3, funct7) \
    RV_INSN_BASE(name, op_maj, funct3, funct7, 0, RV_ENV_R)
#endif

#ifndef RV_INSN_BRANCH
// Define a branch instruction.
#define RV_INSN_BRANCH(name, op_maj, funct3) \
    RV_INSN_BASE(name, op_maj, funct3, 0, 0, RV_ENC_I)
#endif

#ifndef RV_INSN_STORE
// Define a store instruction.
#define RV_INSN_STORE(name, op_maj, funct3) \
    RV_INSN_BASE(name, op_maj, funct3, 0, 0, RV_ENC_S)
#endif

#ifndef RV_INSN_LOAD
// Define a load instruction.
#define RV_INSN_LOAD(name, op_maj, funct3) \
    RV_INSN_BASE(name, op_maj, funct3, 0, 0, RV_ENC_I)
#endif



/* ==== RV pseudo-instructions ==== */

//              [ name ] [   encoding    ]
RV_INSN_PSEUDO( mv,      RV_ENC_PSEUDO_MV)
RV_INSN_PSEUDO( li,      RV_ENC_PSEUDO_LI)
RV_INSN_PSEUDO( ret,     RV_ENC_PSEUDO_RET)
RV_INSN_PSEUDO( j,       RV_ENC_PSEUDO_J)
RV_INSN_PSEUDO( jr,      RV_ENC_PSEUDO_JR)

/* ==== RV32I instructions ==== */

//             [ name ]  [   opcode   ]   [f3 ]  [funct7 ]  [  funct12   ]  [ enc  ]
RV_INSN_BASE(  lui,      RV_OP_LUI,       0b000, 0b0000000, 0b000000000000, RV_ENC_U)
RV_INSN_BASE(  auipc,    RV_OP_LUI,       0b000, 0b0000000, 0b000000000000, RV_ENC_U)
RV_INSN_BASE(  jal,      RV_OP_JAL,       0b000, 0b0000000, 0b000000000000, RV_ENC_J)
RV_INSN_BASE(  jalr,     RV_OP_JALR,      0b000, 0b0000000, 0b000000000000, RV_ENC_I)
RV_INSN_LOAD(  lb,       RV_OP_LOAD,      0b000)
RV_INSN_LOAD(  lh,       RV_OP_LOAD,      0b001)
RV_INSN_LOAD(  lw,       RV_OP_LOAD,      0b010)
RV_INSN_LOAD(  lbu,      RV_OP_LOAD,      0b100)
RV_INSN_LOAD(  lhu,      RV_OP_LOAD,      0b101)
RV_INSN_STORE( sb,       RV_OP_STORE,     0b000)
RV_INSN_STORE( sh,       RV_OP_STORE,     0b001)
RV_INSN_STORE( sw,       RV_OP_STORE,     0b010)
RV_INSN_BRANCH(beq,      RV_OP_BRANCH,    0b000)
RV_INSN_BRANCH(bne,      RV_OP_BRANCH,    0b001)
RV_INSN_BRANCH(blt,      RV_OP_BRANCH,    0b100)
RV_INSN_BRANCH(bge,      RV_OP_BRANCH,    0b101)
RV_INSN_BRANCH(bltu,     RV_OP_BRANCH,    0b110)
RV_INSN_BRANCH(bgeu,     RV_OP_BRANCH,    0b111)
RV_INSN_ALU_RI(addi,     RV_OP_OP_IMM,    0b000, 0b0000000)
RV_INSN_ALU_RI(slti,     RV_OP_OP_IMM,    0b010, 0b0000000)
RV_INSN_ALU_RI(sltiu,    RV_OP_OP_IMM,    0b011, 0b0000000)
RV_INSN_ALU_RI(xori,     RV_OP_OP_IMM,    0b100, 0b0000000)
RV_INSN_ALU_RI(ori,      RV_OP_OP_IMM,    0b110, 0b0000000)
RV_INSN_ALU_RI(andi,     RV_OP_OP_IMM,    0b111, 0b0000000)
RV_INSN_ALU_RI(slli,     RV_OP_OP_IMM,    0b111, 0b0000000)
RV_INSN_ALU_RI(srli,     RV_OP_OP_IMM,    0b111, 0b0000000)
RV_INSN_ALU_RI(srai,     RV_OP_OP_IMM,    0b111, 0b0100000)
RV_INSN_ALU_RR(add,      RV_OP_OP,        0b000, 0b0000000)
RV_INSN_ALU_RR(sub,      RV_OP_OP,        0b000, 0b0100000)
RV_INSN_ALU_RR(sll,      RV_OP_OP,        0b001, 0b0000000)
RV_INSN_ALU_RR(slt,      RV_OP_OP,        0b010, 0b0000000)
RV_INSN_ALU_RR(sltu,     RV_OP_OP,        0b011, 0b0000000)
RV_INSN_ALU_RR(xor,      RV_OP_OP,        0b100, 0b0000000)
RV_INSN_ALU_RR(srl,      RV_OP_OP,        0b101, 0b0000000)
RV_INSN_ALU_RR(sra,      RV_OP_OP,        0b101, 0b0100000)
RV_INSN_ALU_RR(or,       RV_OP_OP,        0b110, 0b0000000)
RV_INSN_ALU_RR(and,      RV_OP_OP,        0b111, 0b0000000)
RV_INSN_BASE(  fence,    RV_OP_MISC_MEM,  0b000, 0b0000000, 0b000000000000, RV_ENC_BITS) // Actually I encoding, but Lily-CC never emits that
RV_INSN_BASE(  ecall,    RV_OP_SYSTEM,    0b000, 0b0000000, 0b000000000000, RV_ENC_BITS)
RV_INSN_BASE(  ebreak,   RV_OP_SYSTEM,    0b000, 0b0000000, 0b000000000001, RV_ENC_BITS)

/* ==== RV64I instructions ==== */

RV_INSN_LOAD(  lwu,      RV_OP_LOAD,      0b110)
RV_INSN_LOAD(  ld,       RV_OP_LOAD,      0b011)
RV_INSN_STORE( sd,       RV_OP_STORE,     0b011)
RV_INSN_ALU_RI(addiw,    RV_OP_OP_IMM_32, 0b000, 0b0000000)
RV_INSN_ALU_RI(slliw,    RV_OP_OP_IMM_32, 0b111, 0b0000000)
RV_INSN_ALU_RI(srliw,    RV_OP_OP_IMM_32, 0b111, 0b0000000)
RV_INSN_ALU_RI(sraiw,    RV_OP_OP_IMM_32, 0b111, 0b0100000)
RV_INSN_ALU_RR(addw,     RV_OP_OP_32,     0b000, 0b0000000)
RV_INSN_ALU_RR(subw,     RV_OP_OP_32,     0b000, 0b0100000)
RV_INSN_ALU_RR(sllw,     RV_OP_OP_32,     0b001, 0b0000000)
RV_INSN_ALU_RR(srlw,     RV_OP_OP_32,     0b101, 0b0000000)
RV_INSN_ALU_RR(sraw,     RV_OP_OP_32,     0b101, 0b0100000)

/* ==== RVM instructions ==== */

RV_INSN_ALU_RR(mul,      RV_OP_OP,        0b000, 0b0000001)
RV_INSN_ALU_RR(mulh,     RV_OP_OP,        0b000, 0b0000001)
RV_INSN_ALU_RR(mulhu,    RV_OP_OP,        0b000, 0b0000001)
RV_INSN_ALU_RR(mulhsu,   RV_OP_OP,        0b000, 0b0000001) // Can C even cause the emission of this?
RV_INSN_ALU_RR(div,      RV_OP_OP,        0b000, 0b0000001)
RV_INSN_ALU_RR(divu,     RV_OP_OP,        0b000, 0b0000001)
RV_INSN_ALU_RR(rem,      RV_OP_OP,        0b000, 0b0000001)
RV_INSN_ALU_RR(remu,     RV_OP_OP,        0b000, 0b0000001)

/* ==== RV64M instructions ==== */

RV_INSN_ALU_RR(mulw,     RV_OP_OP_32,     0b000, 0b0000001)
RV_INSN_ALU_RR(divw,     RV_OP_OP_32,     0b000, 0b0000001)
RV_INSN_ALU_RR(divuw,    RV_OP_OP_32,     0b000, 0b0000001)
RV_INSN_ALU_RR(remw,     RV_OP_OP_32,     0b000, 0b0000001)
RV_INSN_ALU_RR(remuw,    RV_OP_OP_32,     0b000, 0b0000001)



#undef RV_INSN_BASE
#undef RV_INSN_PSEUDO
#undef RV_INSN_NOIR
#undef RV_INSN_ALU_RI
#undef RV_INSN_ALU_RR
#undef RV_INSN_BRANCH
#undef RV_INSN_STORE
#undef RV_INSN_LOAD
