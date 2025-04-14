
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#ifndef DEF_MISC
#define DEF_MISC(name, isa, immbits, sign, r, i, _32)
#endif

#ifndef DEF_BRANCH
// Defines a branch instruction.
#define DEF_BRANCH(name, sign, funct3, ir_cmp) DEF_MISC(name, RV_BASE, 0, sign, 1, 0, 0)
#endif

#ifndef DEF_ALU_IWD
// Defines ALU operations with only imm, 64- and 32-bit versions.
#define DEF_ALU_IWD(name, isa, immbits, sign, funct3, ir_op2) DEF_MISC(name, isa, immbits, sign, 0, 1, 1)
#endif

#ifndef DEF_ALU_RIWD
// Defines ALU operations with imm, 64- and 32-bit versions.
#define DEF_ALU_RIWD(name, isa, immbits, sign, funct3, ir_op2) DEF_MISC(name, isa, immbits, sign, 1, 1, 1)
#endif

#ifndef DEF_ALU_RWD
// Defines ALU operations with 64- and 32-bit versions.
#define DEF_ALU_RWD(name, isa, immbits, sign, funct3, ir_op2) DEF_MISC(name, isa, immbits, sign, 1, 0, 1)
#endif

#ifndef DEF_ALU_RIW
// Defines ALU operations with imm in 32-bit.
#define DEF_ALU_RIW(name, isa, immbits, sign, funct3, ir_op2) DEF_MISC(name, isa, immbits, sign, 1, 1, 1)
#endif

#ifndef DEF_ALU_RID
// Defines ALU operations with imm in 64-bit.
#define DEF_ALU_RID(name, isa, immbits, sign, funct3, ir_op2) DEF_MISC(name, isa, immbits, sign, 1, 1, 1)
#endif

#ifndef DEF_MEM
// Defines memory instructions
#define DEF_MEM(name, isa, membits, sign, funct3, is_store, op_maj) DEF_MISC(name, isa, 0, sign, 1, 0, 1)
#endif

// clang-format off



/* ==== RV32I instructions ==== */

DEF_MISC(    lui,    RV_BASE,   12, s,  0, 0, 0)
DEF_MISC(    auipc,  RV_BASE,   12, s,  0, 0, 0)
DEF_MISC(    jal,    RV_BASE,   21, s,  0, 0, 0)
DEF_MISC(    jalr,   RV_BASE,   21, s,  0, 0, 0)
DEF_BRANCH(  beq,                   su, 0b000, IR_OP2_seq)
DEF_BRANCH(  bne,                   su, 0b001, IR_OP2_sne)
DEF_BRANCH(  blt,                   s,  0b100, IR_OP2_slt)
DEF_BRANCH(  bge,                   s,  0b101, IR_OP2_sge)
DEF_BRANCH(  bltu,                  u,  0b110, IR_OP2_slt)
DEF_BRANCH(  bgeu,                  u,  0b111, IR_OP2_sge)
DEF_MEM(     lb,     RV_BASE,    8, s,  0b000, 0, RV_OP_LOAD)
DEF_MEM(     lh,     RV_BASE,   16, s,  0b001, 0, RV_OP_LOAD)
DEF_MEM(     lw,     RV_BASE,   32, s,  0b010, 0, RV_OP_LOAD)
DEF_MEM(     lbu,    RV_BASE,    8, u,  0b100, 0, RV_OP_LOAD)
DEF_MEM(     lhu,    RV_BASE,   16, u,  0b101, 0, RV_OP_LOAD)
DEF_MEM(     sb,     RV_BASE,    8, su, 0b000, 1, RV_OP_STORE)
DEF_MEM(     sh,     RV_BASE,   16, su, 0b001, 1, RV_OP_STORE)
DEF_MEM(     sw,     RV_BASE,   32, su, 0b010, 1, RV_OP_STORE)
DEF_ALU_RIWD(add,    RV_BASE,   12, su, 0b000, IR_OP2_add)
DEF_ALU_RWD( sub,    RV_BASE,   12, su, 0b000, IR_OP2_sub)
DEF_ALU_RIWD(slt,    RV_BASE,   12, s,  0b010, IP_OP2_slt)
DEF_ALU_RWD( sltu,   RV_BASE,   12, u,  0b010, IP_OP2_slt)
DEF_ALU_IWD( sltiu,  RV_BASE,   12, u,  0b010, IP_OP2_slt)
DEF_ALU_RIWD(sll,    RV_32ONLY,  5, su, 0b001, IP_OP2_shl)
DEF_ALU_RIWD(srl,    RV_32ONLY,  5, u,  0b101, IP_OP2_shr)
DEF_ALU_RIWD(sra,    RV_32ONLY,  5, s,  0b101, IP_OP2_shr)
DEF_ALU_RIWD(xor,    RV_BASE,   12, su, 0b100, IR_OP2_bxor)
DEF_ALU_RIWD(or,     RV_BASE,   12, su, 0b110, IR_OP2_bor)
DEF_ALU_RIWD(and,    RV_BASE,   12, su, 0b111, IR_OP2_band)
DEF_MISC(    fence,  RV_BASE,    0, _,  0, 0, 0)
DEF_MISC(    ecall,  RV_BASE,    0, _,  0, 0, 0)
DEF_MISC(    ebreak, RV_BASE,    0, _,  0, 0, 0)

/* ==== RV64I instructions ==== */
DEF_MEM(     lwu,    RV_64,     32, u,  0b110, 0, RV_OP_LOAD)
DEF_MEM(     ld,     RV_64,     32, u,  0b011, 0, RV_OP_LOAD)
DEF_MEM(     sd,     RV_64,     32, u,  0b011, 1, RV_OP_STORE)
DEF_ALU_RID(sll,     RV_64,       6, su, 0b001, IP_OP2_shl)
DEF_ALU_RID(srl,     RV_64,       6, u,  0b101, IP_OP2_shr)
DEF_ALU_RID(sra,     RV_64,       6, s,  0b101, IP_OP2_shr)
DEF_ALU_RIW(sll,     RV_64,       5, su, 0b001, IP_OP2_shl)
DEF_ALU_RIW(srl,     RV_64,       5, u,  0b101, IP_OP2_shr)
DEF_ALU_RIW(sra,     RV_64,       5, s,  0b101, IP_OP2_shr)
