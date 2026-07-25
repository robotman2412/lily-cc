
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "ir_types.h"



// Get the range of possible values from an IR operand.
bool ir_get_operand_range(ir_operand_t operand, i128_t *min_out, i128_t *max_out);
// Update the possible range according to an expr2 instruction.
void ir_expr2_range(ir_var_t *var, ir_insn_t const *insn);
// Update the possible range according to an expr1 instruction.
void ir_expr1_range(ir_var_t *var, ir_insn_t const *insn);
// Update the possible range according to an instruction.
void ir_insn_range(ir_var_t *var, ir_insn_t const *insn);
// Expand the possible range of a variable.
void ir_expand_range(ir_var_t *var, i128_t min, i128_t max);
// Recompute the possible range of a variable.
void ir_calc_var_range(ir_var_t *var);
// Recompute the possible ranges of all variables in the function.
void ir_calc_all_ranges(ir_func_t *func);

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
