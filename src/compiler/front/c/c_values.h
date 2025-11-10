
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once



#include "compiler.h"
#include "ir_types.h"
#include "refcount.h"
#include "unreachable.h"



// Types of C value.
typedef enum {
    // Represents a missing value caused by compilation error.
    C_VALUE_ERROR,
    // Lvalue by pointer + offset.
    C_LVALUE_MEM,
    // Lvalue by c_var_t.
    C_LVALUE_VAR,
    // Rvalue by ir_operand_t.
    C_RVALUE_OPERAND,
    // Rvalue of an array.
    C_RVALUE_ARR,
    // Rvalue for structs by map_t.
    C_RVALUE_MAP,
    // Rvalue for structs/unions/arrays by binary representation.
    C_RVALUE_BINARY,
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
        union {
            // IR operand that holds the current rvalue.
            ir_operand_t operand;
            // Array of `rc_t` to rvalues; the length is implied by the type.
            c_value_t   *arr;
            // Map of field name to rvalues.
            map_t        map;
            // Binary blob; the size is implied by the type.
            uint8_t     *blob;
        } rvalue;
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
// Access the field of a struct/union value.
c_value_t    c_value_field(c_compiler_t *ctx, ir_code_t *code, c_value_t const *value, char const *field);
// Clone a C value.
c_value_t    c_value_clone(c_compiler_t *ctx, c_value_t const *value);
// Determine whether a value is assignable.
// Produces a diagnostic if it is not.
bool         c_value_assignable(c_compiler_t *ctx, c_value_t const *value, pos_t diag_pos);
// Determine whether the value is a constant rvalue.
bool         c_value_is_const(c_value_t const *value);

// Whether a value is an rvalue.
static inline bool c_is_rvalue(c_value_t const *value) {
    switch (value->value_type) {
        case C_VALUE_ERROR:
        case C_LVALUE_MEM:
        case C_LVALUE_VAR: return false;
        case C_RVALUE_OPERAND:
        case C_RVALUE_ARR:
        case C_RVALUE_MAP:
        case C_RVALUE_BINARY: return true;
    }
    UNREACHABLE();
}
