
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "ir.h"



// Count how many bits are needed to represent the value.
int        ir_count_bits(ir_const_t value, bool allow_s, bool allow_u) __attribute__((const));
// Count number of leading zeroes. Interprets all values as 128-bit.
int        ir_const_clz(ir_const_t value) __attribute__((const));
// Count number of trailing zeroes.
int        ir_const_ctz(ir_const_t value) __attribute__((const));
// Count number of set bits.
int        ir_const_popcnt(ir_const_t value) __attribute__((const));
// Whether a constant is negative.
bool       ir_const_is_negative(ir_const_t value) __attribute__((const));
// Determines whether two constants are identical.
// Floats will be compared bitwise.
bool       ir_const_identical(ir_const_t lhs, ir_const_t rhs) __attribute__((const));
// Determines whether two constants are effectively identical after casting.
// Floats are promoted to f64, then compared bitwise.
bool       ir_const_lenient_identical(ir_const_t lhs, ir_const_t rhs) __attribute__((const));
// Truncate unused bits of a constant.
ir_const_t ir_trim_const(ir_const_t value) __attribute__((const));
// Cast from one type to another with IR rules.
ir_const_t ir_cast(ir_prim_t type, ir_const_t value) __attribute__((const));
// Calculate the result of an expr1.
ir_const_t ir_calc1(ir_op1_type_t oper, ir_const_t value) __attribute__((const));
// Calculate the result of an expr2.
ir_const_t ir_calc2(ir_op2_type_t oper, ir_const_t lhs, ir_const_t rhs) __attribute__((const));

// Determine whether two operands are either the same variable or identical.
// Floats will be compared bitwise.
bool ir_operand_identical(ir_operand_t lhs, ir_operand_t rhs) __attribute__((const));
// Determines whether two operands are either the same variable or effectively identical after casting.
// Floats are promoted to f64, then compared bitwise.
bool ir_operand_lenient_identical(ir_operand_t lhs, ir_operand_t rhs) __attribute__((const));
