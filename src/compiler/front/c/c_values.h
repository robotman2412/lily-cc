
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once



#include "ir_types.h"
#include "refcount.h"



// Types of C value.
typedef enum {
    // Represents a missing value caused by compilation error.
    C_VALUE_ERROR,
    // Lvalue by pointer + offset.
    C_LVALUE_MEM,
    // Lvalue by c_var_t.
    C_LVALUE_VAR,
    // Rvalue by ir_operand_t.
    C_RVALUE,
} c_value_type_t;



// C value.
typedef struct c_value c_value_t;

// C scope.
typedef struct c_scope    c_scope_t;
// C compiler context.
typedef struct c_compiler c_compiler_t;



// A C value returned from an expression.
struct c_value {
    // Type of value that this is.
    c_value_type_t value_type;
    // Refcount pointer of `c_type_t`; C type of this value (may be more restrictive than that of `c_var`).
    rc_t           c_type;
    // Representation of the value.
    union {
        union {
            // The location at which the variable is to be stored.
            ir_memref_t memref;
            // The IR variable associated.
            ir_var_t   *ir_var;
        } lvalue;
        // IR operand that holds the current rvalue.
        ir_operand_t rvalue;
    };
};



// Clean up an lvalue or rvalue.
void         c_value_destroy(c_value_t value);
// Write to an lvalue.
void         c_value_write(c_compiler_t *ctx, ir_code_t *code, c_value_t const *lvalue, c_value_t const *rvalue);
// Get the address of an lvalue.
ir_memref_t  c_value_memref(c_compiler_t *ctx, ir_code_t *code, c_value_t const *value);
// Read a value for scalar arithmetic.
ir_operand_t c_value_read(c_compiler_t *ctx, ir_code_t *code, c_value_t const *value);
