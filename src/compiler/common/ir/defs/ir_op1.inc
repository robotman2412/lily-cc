
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT
// clang-format off

#ifndef IR_OP1_DEF
#define IR_OP1_DEF(keyw)
#endif

// Move value without conversion.
IR_OP1_DEF(mov)
// Arithmetic cast (signed).
IR_OP1_DEF(cast)
// Arithmetic cast (unsigned).
IR_OP1_DEF(castu)
// Bit-cast value.
IR_OP1_DEF(bitcast)

// Set if equal to zero.
IR_OP1_DEF(seqz)
// Set if not equal to zero.
IR_OP1_DEF(snez)

// Arithmetic negate.
IR_OP1_DEF(neg)

// Bitwise negate.
IR_OP1_DEF(bneg)

// TODO: clz, ctz, popcnt, sqrt, abs, float classification

#undef IR_OP1_DEF
