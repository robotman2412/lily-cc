
#include "template_gen.h"

// Function entry for non-inlined functions. 
void gen_function_entry(asm_ctx_t *ctx, funcdef_t *funcdef) {
}

// Return statement for non-inlined functions.
// retval is null for void returns.
void gen_return(asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *retval) {
}


// Expression: Function call.
// args may be null for zero arguments.
gen_var_t *gen_expr_call(asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *callee, size_t n_args, gen_var_t **args) {
}

// Expression: Binary math operation.
gen_var_t *gen_expr_math2(asm_ctx_t *ctx, oper_t oper, gen_var_t *out_hint, gen_var_t *a, gen_var_t *b) {
}

// Expression: Unary math operation.
gen_var_t *gen_expr_math1(asm_ctx_t *ctx, oper_t oper, gen_var_t *out_hint, gen_var_t *a) {
}
