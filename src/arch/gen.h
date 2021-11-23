
#ifndef GEN_H
#define GEN_H

struct gen_var;

typedef struct gen_var gen_var_t;

#include <parser-util.h>
#include <asm.h>

struct gen_var {
    gen_var_type_t type;
    union {
        address_t   iconst;
        asm_label_t label;
        uint16_t    reg;
    };
};

/* ================== Functions ================== */

// Function entry for non-inlined functions. 
void       gen_function_entry(asm_ctx_t *ctx, funcdef_t *funcdef);
// Return statement for non-inlined functions.
// retval is null for void returns.
void       gen_return        (asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *retval);
// Expression: Inline function call. (generic fallback provided)
// args may be null for zero arguments.
gen_var_t *gen_expr_inline   (asm_ctx_t *ctx, funcdef_t *callee,  size_t n_args, gen_var_t **args);
// Expression: Inline function entry. (generic fallback provided; don't define if gen_expr_inline fallback is used)
void       gen_inline_entry  (asm_ctx_t *ctx, funcdef_t *funcdef);
// Return statement for inlined functions. (generic fallback provided; don't define if gen_expr_inline fallback is used)
// retval is null for void returns.
void       gen_inline_return (asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *retval);

/* ================= Expressions ================= */

// Every aspect of the expression to be written. (generic fallback provided)
gen_var_t *gen_expression    (asm_ctx_t *ctx, expr_t    *expr);
// Expression: Function call.
// args may be null for zero arguments.
gen_var_t *gen_expr_call     (asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *callee, size_t n_args, gen_var_t **args);
// Expression: Binary math operation.
gen_var_t *gen_expr_math2    (asm_ctx_t *ctx, oper_t    *expr,    gen_var_t *a,      gen_var_t *b);
// Expression: Unary math operation.
gen_var_t *gen_expr_math1    (asm_ctx_t *ctx, oper_t    *expr,    gen_var_t *a);

/* ================== Variables ================== */

// Variables: Move variable to another location.
void       gen_mov           (asm_ctx_t *ctx, gen_var_t *dest,    gen_var_t *src);
// Variables: Create a variable based on parameter.
void       gen_var_arg       (asm_ctx_t *ctx, funcdef_t *funcdef, size_t     argno);
// Variables: Create a variable based on other value.
void       gen_var_dup       (asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *other);


#endif //GEN_H
