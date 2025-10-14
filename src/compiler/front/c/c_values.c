
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_values.h"

#include "c_types.h"
#include "ir.h"

#include <stdio.h>



// Clean up an lvalue or rvalue.
void c_value_destroy(c_value_t value) {
    rc_delete(value.c_type);
    if (value.value_type == C_LVALUE_MEM && value.lvalue.memref.base_type == IR_MEMBASE_SYM) {
        free(value.lvalue.memref.base_sym);
    }
}

// Write to an lvalue.
void c_value_write(c_compiler_t *ctx, ir_code_t *code, c_value_t const *lvalue, c_value_t const *rvalue) {
    ir_operand_t tmp = c_value_read(ctx, code, rvalue);
    switch (lvalue->value_type) {
        case C_VALUE_ERROR:
            printf("[BUG] c_value_write called on C_VALUE_ERROR\n");
            abort();
            break;
        case C_RVALUE:
            printf("[BUG] c_value_write called with an `rvalue` destination");
            abort();
            break;
        case C_LVALUE_MEM:
            // Store to the pointer.
            ir_add_store(IR_APPEND(code), tmp, lvalue->lvalue.memref);
            break;
        case C_LVALUE_VAR:
            // Store to the IR variable.
            ir_add_expr1(IR_APPEND(code), lvalue->lvalue.ir_var, IR_OP1_mov, tmp);
            break;
    }
}

// Get the memory location of an lvalue.
ir_memref_t c_value_memref(c_compiler_t *ctx, ir_code_t *code, c_value_t const *lvalue) {
    (void)ctx;
    (void)code;

    if (lvalue->value_type == C_VALUE_ERROR) {
        printf("[BUG] c_value_addrof called on C_VALUE_ERROR\n");
        abort();
    } else if (lvalue->value_type == C_RVALUE) {
        printf("[BUG] c_value_addrof called on C_RVALUE\n");
        abort();
    }

    ir_memref_t memref;
    if (lvalue->value_type == C_LVALUE_MEM) {
        // Directly use the pointer from the lvalue.
        memref = lvalue->lvalue.memref;

    } else if (lvalue->value_type == C_LVALUE_VAR) {
        // Take pointer of C variable.
        printf("[BUG] Address taken of C variable but the variable was not marked as such\n");
        abort();

    } else {
        __builtin_unreachable();
    }

    return memref;
}

// Read a value for scalar arithmetic.
ir_operand_t c_value_read(c_compiler_t *ctx, ir_code_t *code, c_value_t const *value) {
    switch (value->value_type) {
        default: abort(); break;
        case C_VALUE_ERROR:
            printf("[BUG] c_value_read called on C_VALUE_ERROR\n");
            abort();
            break;
        case C_RVALUE:
            // Rvalues don't need any reading because they're already in an `ir_operand_t`.
            return value->rvalue;
        case C_LVALUE_MEM: {
            // Pointer lvalue is read from memory.
            ir_var_t *tmp = ir_var_create(code->func, c_type_to_ir_type(ctx, value->c_type->data), NULL);
            ir_add_load(IR_APPEND(code), tmp, c_value_memref(ctx, code, value));
            return IR_OPERAND_VAR(tmp);
        }
        case C_LVALUE_VAR:
            // C variables with no pointer taken are stored in IR variables.
            return IR_OPERAND_VAR(value->lvalue.ir_var);
    }
}
