
#include "gen_util.h"
#include "strmap.h"
#include "malloc.h"

// Find the correct label to correspond to the variable in the current scope.
char *gen_translate_label(asm_ctx_t *ctx, char *label) {
    asm_scope_t *scope = ctx->current_scope;
    while (scope) {
        char *val = map_get(&scope->vars, label);
        if (val) return val;
        scope = scope->parent;
    }
    return NULL;
}

// Define the variable with the given ident.
bool gen_define_var(asm_ctx_t *ctx, char *label, char *ident) {
    void *repl = map_set(&ctx->current_scope->vars, ident, label);
    if (repl) return false;
    if (ctx->current_scope != &ctx->global_scope) {
        // Don't want to deal with global varialbe numbers inside functions.
        ctx->current_scope->local_num ++;
    }
    ctx->current_scope->num ++;
    return true;
}

// New scope.
void gen_push_scope(asm_ctx_t *ctx) {
    asm_scope_t *scope = (asm_scope_t *) malloc(sizeof(asm_scope_t));
    scope->parent      = ctx->current_scope;
    scope->num         = ctx->current_scope->num;
    scope->local_num   = ctx->current_scope->local_num;
    map_create(&scope->vars);
    ctx->current_scope = scope;
}

// Close scope.
void gen_pop_scope(asm_ctx_t *ctx) {
    asm_scope_t *old = ctx->current_scope;
    map_delete_with_values(&old->vars);
    ctx->current_scope = old->parent;
    free(old);
}

