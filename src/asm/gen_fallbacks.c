
#include "config.h"
#include "gen.h"
#include "gen_util.h"
#include "malloc.h"
#include "gen_preproc.h"

/* ================== Functions ================== */

#if defined(FALLFALLBACK_gen_function) || defined(FALLBACK_gen_stmt)
static inline void gen_var_scope(asm_ctx_t *ctx, map_t *map) {
    // Add the variables to the current scope.
    for (size_t i = 0; i < map_size(map); i++) {
        DEBUG_PRE("got var '%s': %s\n", map->strings[i], (char *) map->values[i]);
        gen_define_var(ctx, map->values[i], map->strings[i]);
    }
}
#endif

// Generic fallback for function that doesn't take into account any architecture-specific optimisations.
#ifdef FALLBACK_gen_function
// Function implementation for non-inlined functions. (generic fallback)
void gen_function(asm_ctx_t *ctx, funcdef_t *funcdef) {
    // Have the function preprocessed.
    ctx->is_inline = false;
    ctx->current_func = funcdef;
    ctx->last_label_no = 0;
    gen_preproc_function(ctx, funcdef);
    
    // New function, new scope.
    gen_push_scope(ctx);
    gen_var_scope(ctx, funcdef->preproc->vars);
    
    // Start the process with the function entry.
    DEBUG_GEN("// function entry\n");
    gen_function_entry(ctx, funcdef);
    
    // The statements.
    DEBUG_GEN("// function code\n");
    bool explicit = gen_stmt(ctx, funcdef->stmts, true);
    
    // Add a return if there is not already an explicit one.
    if (!explicit) {
        DEBUG_GEN("// implicit return\n");
        gen_return(ctx, funcdef, NULL);
    } else {
        DEBUG_GEN("// return was explicit\n");
    }
    
    // Close the scope.
    gen_pop_scope(ctx);
    ctx->current_func = NULL;
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

#if defined(FALLBACK_gen_inline_return) && !defined(FALLBACK_gen_expr_inline)
// Return statement for non-inlined functions. (generic fallback)
// retval is null for void returns.
void gen_inline_return(asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *retval) {
    
}
#endif

/* ================== Statements ================= */

#ifdef FALLBACK_gen_stmt
// Statement implementation. (generic fallback)
// Returns true if the statement has an explicit return.
bool gen_stmt(asm_ctx_t *ctx, void *ptr, bool is_stmts) {
    stmt_t *stmt = ptr;
    bool has_scope = false;
    bool explicit_ret = false;
    if (is_stmts) goto stmts_impl;
    if (stmt->type == STMT_TYPE_MULTI) {
        // Use the preprocessor data to create a scope.
        gen_push_scope(ctx);
        gen_var_scope(ctx, stmt->preproc->vars);
        has_scope = true;
        // Pointer perplexing.
        ptr = stmt->stmts;
        stmts_t *stmts;
        // Epic arrays.
        stmts_impl:
        stmts = ptr;
        for (size_t i = 0; i < stmts->num; i++) {
            // Gen them one by one.
            explicit_ret = gen_stmt(ctx, &stmts->arr[i], false);
            // We'll completely ignore unreachable code.
            if (explicit_ret) break;
        }
    } else {
        // One of the other statement types.
        switch (stmt->type) {
            case STMT_TYPE_IF: {
                gen_var_t cond_hint = {
                    .type = VAR_TYPE_COND
                };
                gen_var_t *cond = gen_expression(ctx, stmt->expr, &cond_hint);
                return gen_if(ctx, cond, stmt->code_true, stmt->code_false);
            } break;
            case STMT_TYPE_WHILE: {
                return gen_while(ctx, stmt->cond, stmt->code_true, false);
            } break;
            case STMT_TYPE_RET: {
                // A return statement.
                gen_var_t return_hint = {
                    .type = VAR_TYPE_RETVAL
                };
                gen_var_t *retval = gen_expression(ctx, stmt->expr, &return_hint);
                // Apply the appropriate return code.
                if (ctx->is_inline) {
                    gen_inline_return(ctx, ctx->current_func, retval);
                } else {
                    gen_return(ctx, ctx->current_func, retval);
                }
                return true;
            } break;
            case STMT_TYPE_VAR: {
                // TODO: Vardecls structure to change later.
                for (size_t i = 0; i < stmt->vars->num; i++) {
                    // TODO: Make assignments.
                }
            } break;
            case STMT_TYPE_EXPR: {
                // The expression.
                gen_var_t *result = gen_expression(ctx, stmt->expr, NULL);
                // TODO: Notify the funny man that the result is no longer required.
                free(result);
            } break;
            case STMT_TYPE_IASM: {
                // TODO (low priority).
            } break;
        }
    }
    if (has_scope) {
        gen_pop_scope(ctx);
    }
    return explicit_ret;
}
#endif

/* ================= Expressions ================= */

// Generic fallback for expressions that doesn't take into account any architecture-specific optimisations.
#ifdef FALLBACK_gen_expression
// Every aspect of the expression to be written. (generic fallback)
gen_var_t *gen_expression(asm_ctx_t *ctx, expr_t *expr, gen_var_t *out_hint) {
    switch (expr->type) {
        case EXPR_TYPE_CONST: {
            gen_var_t *val = (gen_var_t *) malloc(sizeof(gen_var_t));
            *val = (gen_var_t) {
                .type = VAR_TYPE_CONST,
                .iconst = expr->iconst
            };
            return val;
        } break;
        case EXPR_TYPE_IDENT: {
            gen_var_t *val = (gen_var_t *) malloc(sizeof(gen_var_t));
            *val = (gen_var_t) {
                .type = VAR_TYPE_LABEL,
                .label = gen_translate_label(ctx, expr->ident->strval)
            };
            if (!val->label) {
                val->label = expr->ident->strval;
                DEBUG_GEN("unknown \"%s\"\n", expr->ident->strval);
            }
            return val;
        } break;
        case EXPR_TYPE_CALL: {
            // TODO: check for inlining possibilities.
            //gen_expr_call(ctx, NULL, expr->func, expr->args->num, expr->args->arr);
        } break;
        case EXPR_TYPE_MATH1: {
            gen_var_t *a = gen_expression(ctx, expr->par_a, out_hint);
            return gen_expr_math1(ctx, expr->oper, out_hint, a);
        } break;
        case EXPR_TYPE_MATH2: {
            if (expr->oper == OP_ASSIGN) {
                // Handle assignment seperately.
                gen_var_t *a = gen_expression(ctx, expr->par_a, NULL);
                gen_var_t *b = gen_expression(ctx, expr->par_b, a);
                gen_mov(ctx, a, b);
                return a;
            } else {
                gen_var_t *a = gen_expression(ctx, expr->par_a, out_hint);
                gen_var_t *b = gen_expression(ctx, expr->par_b, NULL);
                return gen_expr_math2(ctx, expr->oper, out_hint, a, b);
            }
        } break;
    }
}
#endif

