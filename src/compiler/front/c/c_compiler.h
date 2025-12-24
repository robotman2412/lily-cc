
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once



#include "c_prepass.h"
#include "c_tokenizer.h"
#include "c_types.h"
#include "c_values.h"
#include "compiler.h"
#include "ir_types.h"
#include "map.h"
#include "refcount.h"



// Where a C variable is to be stored.
typedef enum {
    // Variable is stored in a register / IR variable (local; no pointer taken).
    C_VAR_STORAGE_REG,
    // Variable is stored in a stack frame (local; pointer possibly taken).
    C_VAR_STORAGE_FRAME,
    // Variable is stored at a symbol (global; pointer taken by definition).
    C_VAR_STORAGE_GLOBAL,
    // Variable is an enum variant.
    C_VAR_STORAGE_ENUM_VARIANT
} c_var_storage_t;



// C variable.
typedef struct c_var          c_var_t;
// C scope.
typedef struct c_scope        c_scope_t;
// C compiler options.
typedef struct c_options      c_options_t;
// C compiler context.
typedef struct c_compiler     c_compiler_t;
// Used for compiling expressions.
typedef struct c_compile_expr c_compile_expr_t;



// C variable.
struct c_var {
    // Where the variable is to be stored.
    c_var_storage_t storage;
    // Variable type (refcount ptr of `c_var_t`).
    rc_t            type;
    union {
        // Local (no pointer taken) variable's IR variable.
        ir_var_t   *ir_var;
        // Local (pointer taken) variable's stack frame.
        ir_frame_t *frame;
        // Global/static local variable's symbol.
        char       *sym;
        // Enum variant index.
        int         enum_variant;
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
    // C variables by declaration's identifier token; `token_t const *` -> `c_var_t *`.
    map_t      locals_by_decl;
    // Local variable map (entry type is `c_var_t`).
    map_t      locals;
    // Map of typedefs (entry type is `rc_t` of `c_type_t`).
    map_t      typedefs;
    // Map of enums, structs and unions (entry type is `rc_t` of `c_type_t`).
    map_t      comp_types;
};

// C compiler options.
struct c_options {
    // Current C standard.
    int      c_std;
    // GNU extensions are enabled.
    uint32_t gnu_ext_enable : 1;
    // Char is signed by default.
    uint32_t char_is_signed : 1;
    // `short` is 16-bit.
    uint32_t short16        : 1;
    // `int` is 32-bit.
    uint32_t int32          : 1;
    // `long` is 64-bit.
    uint32_t long64         : 1;
    // Target is big-endian.
    uint32_t big_endian     : 1;
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
    // Global scope.
    c_scope_t   global_scope;
    // Generic compiler context.
    cctx_t     *cctx;
};

// Used for compiling expressions.
struct c_compile_expr {
    // Result of expression.
    c_value_t  res;
    // Code path linearly after the expression if it exists.
    ir_code_t *code;
};



// Create a new C compiler context.
c_compiler_t *c_compiler_create(cctx_t *cctx, c_options_t options);
// Destroy a C compiler context.
void          c_compiler_destroy(c_compiler_t *cc);

// Create a new scope.
c_scope_t c_scope_create(c_scope_t *parent);
// Clean up a scope.
void      c_scope_destroy(c_scope_t scope);
// Look up a variable in scope.
c_var_t  *c_scope_lookup(c_scope_t *scope, char const *ident);
// Look up a variable in scope by declaration token.
c_var_t  *c_scope_lookup_by_decl(c_scope_t *scope, token_t const *decl);
// Look up a compound type in scope.
rc_t      c_scope_lookup_comp(c_scope_t *scope, char const *ident);
// Look up a typedef in scope.
rc_t      c_scope_lookup_typedef(c_scope_t *scope, char const *ident);

// Convert C binary operator to IR binary operator.
ir_op2_type_t c_op2_to_ir_op2(c_tokentype_t subtype);
// Convert C unary operator to IR unary operator.
ir_op1_type_t c_op1_to_ir_op1(c_tokentype_t subtype);

// Create a C variable in the current translation unit.
// If in global scope, `code` and `prepass` must be `NULL`.
c_var_t *c_var_create(
    c_compiler_t *ctx, c_prepass_t *prepass, ir_func_t *func, rc_t type_rc, token_t const *name_tkn, c_scope_t *scope
);

// Compile a compound initializer into IR.
c_compile_expr_t c_compile_comp_init(
    c_compiler_t  *ctx,
    c_prepass_t   *prepass,
    ir_code_t     *code,
    c_scope_t     *scope,
    token_t const *init,
    rc_t           type_rc,
    pos_t          type_pos
);
// Compile an expression into IR.
c_compile_expr_t
    c_compile_expr(c_compiler_t *ctx, c_prepass_t *prepass, ir_code_t *code, c_scope_t *scope, token_t const *expr);
// Compile a statement node into IR.
// Returns the code path linearly after this.
ir_code_t *
    c_compile_stmt(c_compiler_t *ctx, c_prepass_t *prepass, ir_code_t *code, c_scope_t *scope, token_t const *stmt);
// Compile a C function definition into IR.
ir_func_t *c_compile_func_def(c_compiler_t *ctx, token_t const *def, c_prepass_t *prepass);
// Compile a declaration statement.
// If in global scope, `code` and `prepass` must be `NULL`, and will return `NULL`.
// If not global, all must not be `NULL` and will return the code path linearly after this.
ir_code_t *
    c_compile_decls(c_compiler_t *ctx, c_prepass_t *prepass, ir_code_t *code, c_scope_t *scope, token_t const *decls);

// Explain a C type.
void c_type_explain(c_type_t *type, FILE *to);
