
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "ir.h"



// Count number of trailing zeroes.
int        ir_const_ctz(ir_const_t value);
// Count number of set bits.
int        ir_const_popcnt(ir_const_t value);
// Whether a constant is negative.
bool       ir_const_is_negative(ir_const_t value);
// Truncate unused bits of a constant.
ir_const_t ir_trim_const(ir_const_t value);
// Cast from one type to another with IR rules.
ir_const_t ir_cast(ir_prim_t type, ir_const_t value);
// Calculate the result of an expr1.
ir_const_t ir_calc1(ir_op1_type_t oper, ir_const_t value);
// Calculate the result of an expr2.
ir_const_t ir_calc2(ir_op2_type_t oper, ir_const_t lhs, ir_const_t rhs);
