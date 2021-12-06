
#ifndef GEN_H
#define GEN_H

struct gen_var;

typedef struct gen_var gen_var_t;

#include <parser-util.h>
#include <asm.h>
#include <gen_preproc.h>

struct gen_var {
    gen_var_type_t type;
    union {
        address_t   iconst;
        address_t   offset;
        asm_label_t label;
        reg_t       reg;
        cond_t      cond;
    };
};

/* ================== Functions ================== */

// Function implementation for non-inlined functions. (generic fallback provided)
void       gen_function      (asm_ctx_t *ctx, funcdef_t *funcdef);
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

/* ================== Statements ================= */

// Statement implementation. (generic fallback provided)
// Returns true if the statement has an explicit return.
bool       gen_stmt          (asm_ctx_t *ctx, void      *stmt,    bool is_stmts);
// If statement implementation.
bool       gen_if            (asm_ctx_t *ctx, gen_var_t *cond,    stmt_t    *s_if,     stmt_t    *s_else);
// While statement implementation.
bool       gen_while         (asm_ctx_t *ctx, expr_t    *cond,    stmt_t    *code,     bool       do_while);

/* ================= Expressions ================= */

// Every aspect of the expression to be written. (generic fallback provided)
gen_var_t *gen_expression    (asm_ctx_t *ctx, expr_t    *expr,    gen_var_t *out_hint);
// Expression: Function call.
// args may be null for zero arguments.
gen_var_t *gen_expr_call     (asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *callee,   size_t     n_args, gen_var_t **args);
// Expression: Binary math operation.
gen_var_t *gen_expr_math2    (asm_ctx_t *ctx, oper_t     oper,    gen_var_t *out_hint, gen_var_t *a,      gen_var_t  *b);
// Expression: Unary math operation.
gen_var_t *gen_expr_math1    (asm_ctx_t *ctx, oper_t     oper,    gen_var_t *out_hint, gen_var_t *a);

/* ================== Variables ================== */

// Variables: Move variable to another location.
void       gen_mov           (asm_ctx_t *ctx, gen_var_t *dest,    gen_var_t *src);
// Variables: Create a variable based on other value.
// Other value is null if not initialised.
void       gen_var_dup       (asm_ctx_t *ctx, funcdef_t *funcdef, ident_t *ident,      gen_var_t *other);
// Variables: Create a label for the varialbe at preprocessing time.
char      *gen_preproc_var   (asm_ctx_t *ctx, preproc_data_t *parent, ident_t *ident);


#endif //GEN_H
