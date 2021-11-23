
#include "config.h"
#include "gen.h"

// Generic fallback for expressions that doesn't take into account any architecture-specific optimisations.
#ifdef FALLBACK_gen_expression
// Every aspect of the expression to be written. (generic fallback)
gen_var_t *gen_expression(asm_ctx_t *ctx, expr_t *expr) {
    switch (expr->type) {
        case EXPR_TYPE_CONST: {
            gen_var_t *val = (gen_var_t *) malloc(sizeof(gen_var_t));
            *val = (gen_var_t) {
                .type = VAR_TYPE_CONST,
                .iconst = expr->iconst
            };
        } break;
        case EXPR_TYPE_IDENT: {
            gen_var_t *val = (gen_var_t *) malloc(sizeof(gen_var_t));
            *val = (gen_var_t) {
                .type = VAR_TYPE_LABEL,
                .label = expr->ident->strval
            };
        } break;
        case EXPR_TYPE_CALL: {
            // TODO: check for inlining possibilities.
            gen_expr_call(ctx, NULL, expr->func, expr->args->num, expr->args->arr);
        } break;
        case EXPR_TYPE_MATH1: {
            gen_var_t *a = gen_expression(ctx, expr->par_a);
            gen_expr_math1(ctx, expr->oper, a);
        } break;
        case EXPR_TYPE_MATH2: {
            gen_var_t *a = gen_expression(ctx, expr->par_a);
            gen_var_t *b = gen_expression(ctx, expr->par_a);
            gen_expr_math2(ctx, expr->oper, expr->par_a, expr->par_b);
        } break;
    }
}
#endif

#ifdef FALLBACK_gen_expr_inline
// Expression: Inline function call. (generic fallback)
// args may be null for zero arguments.
gen_var_t *gen_expr_inline(asm_ctx_t *ctx, funcdef_t *callee,  size_t n_args, gen_var_t **args) {
    // TODO
}

// Expression: Inline function entry. (stub)
void gen_inline_entry(asm_ctx_t *ctx, funcdef_t *funcdef) {}

// Return statement for non-inlined functions. (stub)
// retval is null for void returns.
void gen_inline_return(asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *retval) {}
#endif

#if defined(FALLBACK_gen_inline_entry) && !defined(FALLBACK_gen_expr_inline)
// Expression: Inline function entry. (generic fallback)
void gen_inline_entry(asm_ctx_t *ctx, funcdef_t *funcdef) {
    
}
#endif

#ifdef defined(FALLBACK_gen_inline_return) && !defined(FALLBACK_gen_expr_inline)
// Return statement for non-inlined functions. (generic fallback)
// retval is null for void returns.
void gen_inline_return(asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *retval) {
    
}
#endif
