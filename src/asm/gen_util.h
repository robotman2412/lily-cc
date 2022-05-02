
#ifndef GEN_UTIL_H
#define GEN_UTIL_H

#include <asm.h>

// Free an instance of gen_var_t and it's underlying data.
void       free_gen_var    (gen_var_t *ptr);

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

#endif //GEN_UTIL_H
