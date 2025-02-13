
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "ir_types.h"

#include <stdio.h>



// Create a new IR function.
// Function argument types are IR_PRIM_S32 by default.
ir_func_t *ir_func_create(char const *name, char const *entry_name, size_t args_len, char const *const *args_name);
// Delete an IR function.
void       ir_func_destroy(ir_func_t *func);
// Serialize an IR function.
void       ir_func_serialize(ir_func_t *func, FILE *to);

// Convert non-SSA to SSA form.
void ir_func_to_ssa(ir_func_t *func);
// Recalculate the predecessors and successors for code blocks.
void ir_func_recalc_flow(ir_func_t *func);

// Create a new variable.
// If `name` is `NULL`, its name will be a decimal number.
// For this reason, avoid explicitly passing names that are just a decimal number.
ir_var_t  *ir_var_create(ir_func_t *func, ir_prim_t type, char const *name);
// Delete an IR variable, removing all assignments and references in the process.
void       ir_var_delete(ir_var_t *var);
// Replace all references to a variable with a constant.
// Does not replace assignments, nor does it delete the variable.
void       ir_var_replace(ir_var_t *var, ir_operand_t value);
// Create a new IR code block.
// If `name` is `NULL`, its name will be a decimal number.
// For this reason, avoid explicitly passing names that are just a decimal number.
ir_code_t *ir_code_create(ir_func_t *func, char const *name);
// Delete an IR code block and all contained instructions.
void       ir_code_delete(ir_code_t *code);
// Delete an instruction from the code.
void       ir_insn_delete(ir_insn_t *insn);

// Add a combinator function to a code block.
// Takes ownership of the `from` array.
void ir_add_combinator(ir_code_t *code, ir_var_t *dest, size_t from_len, ir_combinator_t *from);
// Add an expression to a code block.
void ir_add_expr1(ir_code_t *code, ir_var_t *dest, ir_op1_type_t oper, ir_operand_t operand);
// Add an expression to a code block.
void ir_add_expr2(ir_code_t *code, ir_var_t *dest, ir_op2_type_t oper, ir_operand_t lhs, ir_operand_t rhs);
// Add an undefined variable.
void ir_add_undefined(ir_code_t *code, ir_var_t *dest);
// Add a direct (by label) function call.
// Takes ownership of `params`.
void ir_add_call_direct(ir_code_t *from, char const *label, size_t params_len, ir_operand_t *params);
// Add an indirect (by pointer) function call.
// Takes ownership of `params`.
void ir_add_call_ptr(ir_code_t *from, ir_operand_t funcptr, size_t params_len, ir_operand_t *params);
// Add an unconditional jump.
void ir_add_jump(ir_code_t *from, ir_code_t *to);
// Add a conditional branch.
void ir_add_branch(ir_code_t *from, ir_operand_t cond, ir_code_t *to);
// Add a return without value.
void ir_add_return0(ir_code_t *from);
// Add a return with value.
void ir_add_return1(ir_code_t *from, ir_operand_t value);
