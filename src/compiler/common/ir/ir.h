
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "ir_serialization.h"
#include "ir_types.h"

#include <stdio.h>

// Type of IR code placement location.
typedef enum {
    // Place at the end of a code block.
    IR_INSNLOC_APPEND_CODE,
    // Place after an existing instruction.
    IR_INSNLOC_AFTER_INSN,
    // Place before an existing instruction.
    IR_INSNLOC_BEFORE_INSN,
} ir_insnloc_type_t;

// Points at which an instruction can be inserted.
typedef struct {
    // Type of IR code placement location.
    ir_insnloc_type_t type;
    union {
        // Code to append to the end of.
        ir_code_t *code;
        // Instruction to place after.
        ir_insn_t *insn;
    };
} ir_insnloc_t;

// Construct an `ir_insnloc_t` that places at the end of `code_`.
#define IR_APPEND(code_)      ((ir_insnloc_t){.type = IR_INSNLOC_APPEND_CODE, .code = (code_)})
// Construct an `ir_insnloc_t` that places after `insn_`.
#define IR_AFTER_INSN(insn_)  ((ir_insnloc_t){.type = IR_INSNLOC_AFTER_INSN, .insn = (insn_)})
// Construct an `ir_insnloc_t` that places before `insn_`.
#define IR_BEFORE_INSN(insn_) ((ir_insnloc_t){.type = IR_INSNLOC_BEFORE_INSN, .insn = (insn_)})



// Create a new IR function.
// Function argument types are IR_PRIM_S32 by default.
ir_func_t *ir_func_create(char const *name, char const *entry_name, size_t args_len);
// Create an IR function without operands nor code.
ir_func_t *ir_func_create_empty(char const *name);
// Delete an IR function.
void       ir_func_delete(ir_func_t *func);

// Convert non-SSA to SSA form.
void ir_func_to_ssa(ir_func_t *func);
// Recalculate the predecessors and successors for code blocks.
void ir_func_recalc_flow(ir_func_t *func);

// Create a new stack frame.
// If `name` is `NULL`, its name will be `frame%zu` where `%zu` is a number.
ir_frame_t *ir_frame_create(ir_func_t *func, uint64_t size, uint64_t align, char const *name);

// Create a new variable.
// If `name` is `NULL`, its name will be `var%zu` where `%zu` is a number.
ir_var_t *ir_var_create(ir_func_t *func, ir_prim_t type, char const *name);
// Delete an IR variable, removing all assignments and references in the process.
void      ir_var_delete(ir_var_t *var);
// Replace all references to a variable with a constant.
// Does not replace assignments, nor does it delete the variable.
void      ir_var_replace(ir_var_t *var, ir_operand_t value);

// Create a new IR code block.
// If `name` is `NULL`, its name will be `code%zu` where `%zu` is a number.
ir_code_t *ir_code_create(ir_func_t *func, char const *name);
// Delete an IR code block and all contained instructions.
void       ir_code_delete(ir_code_t *code);

// Delete an instruction from the code.
void ir_insn_delete(ir_insn_t *insn);
// Set an IR instruction's operand by index.
void ir_insn_set_operand(ir_insn_t *insn, size_t index, ir_operand_t operand);
// Set an IR instruction's return variable by index.
void ir_insn_set_return(ir_insn_t *insn, size_t index, ir_var_t *var);


// Add a combinator function to a code block.
// Takes ownership of the `from` array.
ir_insn_t *ir_add_combinator(ir_insnloc_t loc, ir_var_t *dest, size_t from_len, ir_combinator_t *from);
// Add an expression to a code block.
ir_insn_t *ir_add_expr1(ir_insnloc_t loc, ir_var_t *dest, ir_op1_type_t oper, ir_operand_t operand);
// Add an expression to a code block.
ir_insn_t *ir_add_expr2(ir_insnloc_t loc, ir_var_t *dest, ir_op2_type_t oper, ir_operand_t lhs, ir_operand_t rhs);

// Add a load effective address of a stack frame to a code block.
ir_insn_t *ir_add_lea_stack(ir_insnloc_t loc, ir_var_t *dest, ir_frame_t *frame, uint64_t offset);
// Add a load effective address of a symbol to a code block.
ir_insn_t *ir_add_lea_symbol(ir_insnloc_t loc, ir_var_t *dest, char const *symbol, uint64_t offset);
// Add a load effective address.
ir_insn_t *ir_add_lea(ir_insnloc_t loc, ir_var_t *dest, ir_memref_t memref);
// Add a memory load to a code block.
ir_insn_t *ir_add_load(ir_insnloc_t loc, ir_var_t *dest, ir_memref_t memref);
// Add a memory store to a code block.
ir_insn_t *ir_add_store(ir_insnloc_t loc, ir_operand_t src, ir_memref_t memref);

// Add a function call.
ir_insn_t *ir_add_call(
    ir_insnloc_t        from,
    ir_memref_t         to,
    size_t              returns_len,
    ir_var_t          **returns,
    size_t              params_len,
    ir_operand_t const *params
);
// Add an unconditional jump.
ir_insn_t *ir_add_jump(ir_insnloc_t from, ir_code_t *to);
// Add a conditional branch.
ir_insn_t *ir_add_branch(ir_insnloc_t from, ir_operand_t cond, ir_code_t *to);
// Add a return without value.
ir_insn_t *ir_add_return0(ir_insnloc_t from);
// Add a return with value.
ir_insn_t *ir_add_return1(ir_insnloc_t from, ir_operand_t value);
// Add a return.
ir_insn_t *ir_add_return(ir_insnloc_t from, size_t values_len, ir_operand_t const *values);

// Add a machine instruction.
ir_insn_t *ir_add_mach_insn(
    ir_insnloc_t loc, ir_var_t *dest, insn_proto_t const *proto, size_t operands_len, ir_operand_t const *operands
);



// If the instruction stores to a variable, returns the variable written to.
static inline ir_var_t *ir_insn_get_dest(ir_insn_t const *insn) {
    if (insn->returns_len == 1) {
        return insn->returns[0];
    } else {
        return NULL;
    }
}

// Get the code that an `ir_insnloc_t` lies within.
static inline ir_code_t *ir_insnloc_code(ir_insnloc_t loc) {
    switch (loc.type) {
        case IR_INSNLOC_APPEND_CODE: return loc.code;
        case IR_INSNLOC_AFTER_INSN:
        case IR_INSNLOC_BEFORE_INSN: return loc.insn->code;
        default: __builtin_unreachable();
    }
}
