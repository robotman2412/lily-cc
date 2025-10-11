
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT
// clang-format off

#ifndef IR_OP2_DEF
#define IR_OP2_DEF(keyw)
#endif

/* ==== Comparison operators ==== */
// Greater than.
IR_OP2_DEF(sgt)
// Less than or equal.
IR_OP2_DEF(sle)
// Less than.
IR_OP2_DEF(slt)
// Greater than or equal.
IR_OP2_DEF(sge)
// Equal.
IR_OP2_DEF(seq)
// Not equal.
IR_OP2_DEF(sne)

/* ==== Arithmetic operators ==== */
// Addition.
IR_OP2_DEF(add)
// Subtraction.
IR_OP2_DEF(sub)
// Multiplication.
IR_OP2_DEF(mul)
// Division.
IR_OP2_DEF(div)
// Remainder.
IR_OP2_DEF(rem)

/* ==== Bitwise operators ==== */
// Bitwise shift left.
IR_OP2_DEF(shl)
// Bitwise shift right.
IR_OP2_DEF(shr)
// Bitwise AND.
IR_OP2_DEF(band)
// Bitwise OR.
IR_OP2_DEF(bor)
// Bitwise XOR.
IR_OP2_DEF(bxor)

// TODO: min, max

#undef IR_OP2_DEF
