
#ifndef GEN_UTIL_H
#define GEN_UTIL_H

#include <asm.h>

// Get or create a simple type in the current scope.
var_type_t *ctype_simple(asm_ctx_t *ctx, simple_type_t of);
// Get or create an array type of a simple type in the current scope.
// Define len as 0 if unknown.
var_type_t *ctype_arr_simple(asm_ctx_t *ctx, simple_type_t of, size_t len);
// Get or create a pointer type of a simple type in the current scope.
var_type_t *ctype_ptr_simple(asm_ctx_t *ctx, simple_type_t of);
// Get or create a pointer type of a given type in the current scope.
var_type_t *ctype_ptr(asm_ctx_t *ctx, var_type_t *of);

// Find and return the location of the variable with the given name.
gen_var_t *gen_get_variable(asm_ctx_t *ctx, char      *label);
// Define the variable with the given ident.
bool       gen_define_var  (asm_ctx_t *ctx, gen_var_t *var, char *ident);
// Define a temp var label.
bool       gen_define_temp (asm_ctx_t *ctx, char      *label);
// Mark the label as not in use.
void       gen_unuse       (asm_ctx_t *ctx, gen_var_t *var);

// Compare gen_var_t against each other.
bool       gen_cmp         (asm_ctx_t *ctx, gen_var_t *a, gen_var_t *b);

// New scope.
void       gen_push_scope  (asm_ctx_t *ctx);
// Close scope.
void       gen_pop_scope   (asm_ctx_t *ctx);

// Creates an escaped representation of a given C-string.
char      *esc_cstr        (alloc_ctx_t allocator, const char *cstr, size_t len);

#endif //GEN_UTIL_H
