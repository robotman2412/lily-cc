
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#ifndef DEF_MISC
#define DEF_MISC(name, isa, s, u, r, i, _32)
#endif

#ifndef DEF_ALU_I32
// Defines ALU operations with only imm and 32-bit versions.
#define DEF_ALU_I32(name, isa, s, u, ir_op2) DEF_MISC(name, isa, 0, 1, 1)
#endif

#ifndef DEF_ALU_RI32
// Defines ALU operations with imm and 32-bit versions.
#define DEF_ALU_RI32(name, isa, s, u, ir_op2) DEF_MISC(name, isa, 1, 1, 1)
#endif

#ifndef DEF_ALU_R32
// Defines ALU operations with 32-bit versions.
#define DEF_ALU_R32(name, isa, s, u, ir_op2) DEF_MISC(name, isa, 1, 0, 1)
#endif

// clang-format off



DEF_ALU_RI32(add, RV_BASE_E, 1, 1, IR_OP2_add)
DEF_ALU_R32( sub, RV_BASE_E, 1, 1, IR_OP2_sub)
DEF_ALU_RI32(xor, RV_BASE_E, 1, 1, IR_OP2_bxor)
DEF_ALU_RI32(or,  RV_BASE_E, 1, 1, IR_OP2_bor)
DEF_ALU_RI32(and, RV_BASE_E, 1, 1, IR_OP2_band)
