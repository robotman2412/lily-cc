
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "c_prepass.h"
#include "c_std.h"
#include "c_tokenizer.h"
#include "compiler.h"
#include "ir.h"
#include "map.h"
#include "refcount.h"



// C type primitives.
typedef enum {
    // `_Bool`, `bool`
    C_PRIM_BOOL,

    // `unsigned char`
    C_PRIM_UCHAR,
    // `signed char`
    C_PRIM_SCHAR,
    // `unsigned short (int)`
    C_PRIM_USHORT,
    // `(signed) short (int)`
    C_PRIM_SSHORT,
    // `unsigned (int)`
    C_PRIM_UINT,
    // `(signed) int`, `signed`
    C_PRIM_SINT,
    // `unsigned long (int)`
    C_PRIM_ULONG,
    // `signed long (int)`
    C_PRIM_SLONG,
    // `unsigned long long (int)`
    C_PRIM_ULLONG,
    // `signed long long (int)`
    C_PRIM_SLLONG,

    // `float`
    C_PRIM_FLOAT,
    // `double`
    C_PRIM_DOUBLE,
    // `long double`
    C_PRIM_LDOUBLE,

    // `void`
    C_PRIM_VOID,

    // Number of type primitives.
    C_N_PRIM,

    // Type is a struct.
    C_COMP_STRUCT = C_N_PRIM,
    // Type is an union.
    C_COMP_UNION,
    // Type is an enum.
    C_COMP_ENUM,
    // Type is a pointer.
    C_COMP_POINTER,
    // Type is an array.
    C_COMP_ARRAY,
    // Type is a function.
    C_COMP_FUNCTION,
} c_prim_t;

// Types of C value.
typedef enum {
    // Represents a missing value caused by compilation error.
    C_VALUE_ERROR,
    // Lvalue by pointer + offset.
    C_LVALUE_PTR,
    // Lvalue by c_var_t.
    C_LVALUE_VAR,
    // Rvalue.
    C_RVALUE,
} c_value_type_t;



// C variable.
typedef struct c_var          c_var_t;
// C scope.
typedef struct c_scope        c_scope_t;
// C type.
typedef struct c_type         c_type_t;
// C value.
typedef struct c_value        c_value_t;
// C compiler options.
typedef struct c_options      c_options_t;
// C compiler context.
typedef struct c_compiler     c_compiler_t;
// Used for compiling expressions.
typedef struct c_compile_expr c_compile_expr_t;



// C variable.
struct c_var {
    // Is a global variable?
    bool      is_global;
    // Has a pointer been taken of this variable?
    bool      pointer_taken;
    // Variable type (refcount ptr of `c_var_t`).
    rc_t      type;
    // Variable's IR register.
    ir_var_t *ir_var;
    union {
        // Local variable's stack frame.
        ir_frame_t *ir_frame;
        // Global/static local variable's symbol.
        char       *symbol;
    };
};

// C scope.
struct c_scope {
    // Scope depth; 0 is global.
    int        depth;
    // Disallow variable decls that exist in a parent scope.
    // Used specifically for for loops.
    bool       local_exclusive;
    // Parent scope, if any.
    c_scope_t *parent;
    // Local variable map (entry type is `c_var_t`).
    map_t      locals;
};

// C type.
struct c_type {
    // Primitive type.
    c_prim_t primitive;
    // Size of this type; 0 for incomplete types.
    uint64_t size;
    // Alignment of this type; must be a power of 2.
    uint64_t align;
    // Is volatile?
    bool     is_volatile;
    // Is const?
    bool     is_const;
    // Is _Atomic?
    bool     is_atomic;
    // Is a restrict pointer?
    bool     is_restrict;
    union {
        // Inner type of pointers and arrays.
        rc_t inner;
        struct {
            // Function return type.
            rc_t   return_type;
            // Number of arguments.
            size_t args_len;
            // Argument types.
            rc_t  *args;
            // Argument names.
            char **arg_names;
        } func;
    };
};

// A C value returned from an expression.
struct c_value {
    // Type of value that this is.
    c_value_type_t value_type;
    // Refcount pointer of `c_type_t`; C type of this value.
    rc_t           c_type;
    // Representation of the value.
    union {
        union {
            // The pointer at which the variable is to be stored.
            ir_operand_t ptr;
            // Associated C variable, if any.
            c_var_t     *c_var;
        } lvalue;
        // IR operand that holds the current rvalue.
        ir_operand_t rvalue;
    };
};

// C compiler options.
struct c_options {
    // Current C standard.
    int      c_std;
    // GNU extensions are enabled.
    bool     gnu_ext_enable;
    // Char is signed by default.
    bool     char_is_signed;
    // `short` is 16-bit.
    bool     short16;
    // `int` is 32-bit.
    bool     int32;
    // `long` is 64-bit.
    bool     long64;
    // C primitive corresponding to unsigned size_t.
    c_prim_t size_type;
};

// C compiler context.
struct c_compiler {
    // C compiler options.
    c_options_t options;
    // Actual primitive type definitions derived from options.
    c_type_t    prim_types[C_N_PRIM];
    // Fake refcount ptrs to `c_type_t` for all C primitive types.
    struct rc_t prim_rcs[C_N_PRIM];
    // Map of global typedefs (entry type is `c_type_t`).
    map_t       typedefs;
    // Global scope.
    c_scope_t   global_scope;
    // Generic compiler context.
    cctx_t     *cctx;
};

// Used for compiling expressions.
struct c_compile_expr {
    // Result of expression.
    c_value_t  res;
    // Code path linearly after the expression.
    ir_code_t *code;
};



// Create a new C compiler context.
c_compiler_t *c_compiler_create(cctx_t *cctx, c_options_t options);
// Destroy a C compiler context.
void          c_compiler_destroy(c_compiler_t *cc);

// Create a C type from a specifier-qualifer list.
// Returns a refcount pointer of `c_type_t`.
rc_t c_compile_spec_qual_list(c_compiler_t *ctx, token_t *list);
// Create a C type and get the name from an (abstract) declarator.
// Takes ownership of the `spec_qual_type` share passed.
rc_t c_compile_decl(c_compiler_t *ctx, token_t *decl, rc_t spec_qual_type, token_t const **name_out);

// Create a new scope.
c_scope_t *c_scope_create(c_scope_t *parent);
// Clean up a scope.
void       c_scope_destroy(c_scope_t *scope);
// Look up a variable in scope.
c_var_t   *c_scope_lookup(c_scope_t *scope, char const *ident);

// Create a type that is a pointer to an existing type.
rc_t          c_type_pointer(c_compiler_t *ctx, rc_t inner);
// Determine type promotion to apply in an infix context.
// Note: This does not take ownership of the refcount ptr;
// It will call `rc_share` on the callers behalf only if necessary.
rc_t          c_type_promote(c_compiler_t *ctx, c_tokentype_t oper, rc_t a, rc_t b);
// Convert C binary operator to IR binary operator.
ir_op2_type_t c_op2_to_ir_op2(c_tokentype_t subtype);
// Convert C unary operator to IR unary operator.
ir_op1_type_t c_op1_to_ir_op1(c_tokentype_t subtype);
// Convert C primitive or pointer type to IR primitive type.
ir_prim_t     c_prim_to_ir_type(c_compiler_t *ctx, c_prim_t prim);
// Convert C primitive or pointer type to IR primitive type.
ir_prim_t     c_type_to_ir_type(c_compiler_t *ctx, c_type_t *type);
// Cast one IR type to another according to the C rules for doing so.
ir_operand_t  c_cast_ir_operand(ir_code_t *code, ir_operand_t operand, ir_prim_t type);

// Clean up an lvalue or rvalue.
void         c_value_destroy(c_value_t value);
// Write to an lvalue.
void         c_value_write(c_compiler_t *ctx, ir_code_t *code, c_value_t const *lvalue, c_value_t const *rvalue);
// Get the address of an lvalue.
ir_operand_t c_value_addrof(c_compiler_t *ctx, ir_code_t *code, c_value_t const *value);
// Read a value for scalar arithmetic.
ir_operand_t c_value_read(c_compiler_t *ctx, ir_code_t *code, c_value_t const *value);

// Compile an expression into IR.
c_compile_expr_t
           c_compile_expr(c_compiler_t *ctx, c_prepass_t *prepass, ir_code_t *code, c_scope_t *scope, token_t *expr);
// Compile a statement node into IR.
// Returns the code path linearly after this.
ir_code_t *c_compile_stmt(c_compiler_t *ctx, c_prepass_t *prepass, ir_code_t *code, c_scope_t *scope, token_t *stmt);
// Compile a C function definition into IR.
ir_func_t *c_compile_func_def(c_compiler_t *ctx, token_t *def, c_prepass_t *prepass);
// Compile a declaration statement.
// If in global scope, `func` will be NULL.
void       c_compile_decls(c_compiler_t *ctx, ir_func_t *func, c_scope_t *scope, token_t *decls);

// Explain a C type.
void c_type_explain(c_type_t *type, FILE *to);
