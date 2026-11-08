
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT
// clang-format off

#ifndef IR_OP2_DEF
#define IR_OP2_DEF(keyw)
#endif

/* ==== Comparison operators ==== */
// Greater than (unsigned).
IR_OP2_DEF(sgtu)
// Less than or equal (unsigned).
IR_OP2_DEF(sleu)
// Less than (unsigned).
IR_OP2_DEF(sltu)
// Greater than or equal (unsigned).
IR_OP2_DEF(sgeu)
// Greater than (signed).
IR_OP2_DEF(sgts)
// Less than or equal (signed).
IR_OP2_DEF(sles)
// Less than (signed).
IR_OP2_DEF(slts)
// Greater than or equal (signed).
IR_OP2_DEF(sges)
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
// Division (unsigned).
IR_OP2_DEF(divu)
// Remainder (unsigned).
IR_OP2_DEF(remu)
// Division (signed).
IR_OP2_DEF(divs)
// Remainder (signed).
IR_OP2_DEF(rems)

/* ==== Bitwise operators ==== */
// Bitwise shift left.
IR_OP2_DEF(shl)
// Bitwise (unsigned) shift right.
IR_OP2_DEF(shru)
// Arithmetic (signed) shift right.
IR_OP2_DEF(shrs)
// Bitwise AND.
IR_OP2_DEF(band)
// Bitwise OR.
IR_OP2_DEF(bor)
// Bitwise XOR.
IR_OP2_DEF(bxor)

// TODO: min, max

#undef IR_OP2_DEF
