
#ifndef GEN_UTIL_H
#define GEN_UTIL_H

#include <asm.h>

// Find the correct label to correspond to the variable in the current scope.
char *gen_translate_label(asm_ctx_t *ctx, char *label);
// Define the variable with the given ident.
bool  gen_define_var     (asm_ctx_t *ctx, char *label, char *ident);
// New scope.
void  gen_push_scope     (asm_ctx_t *ctx);
// Close scope.
void  gen_pop_scope      (asm_ctx_t *ctx);

#endif //GEN_UTIL_H
