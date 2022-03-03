
#include "gen_util.h"
#include "strmap.h"
#include "string.h"
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

// Define a temp var label.
bool gen_define_temp(asm_ctx_t *ctx, char *label) {
	ctx->temp_num ++;
	ctx->temp_labels = realloc(ctx->temp_labels, sizeof(char *) * ctx->temp_num);
	ctx->temp_usage  = realloc(ctx->temp_usage,  sizeof(bool)   * ctx->temp_num);
	ctx->temp_labels[ctx->temp_num - 1] = label;
	ctx->temp_usage [ctx->temp_num - 1] = 0;
}

// Mark the label as not in use.
void gen_unuse(asm_ctx_t *ctx, gen_var_t *var) {
	if (var->type != VAR_TYPE_LABEL) return;
	char *label = var->label;
	// Free up all the marked temp labels.
	// Because one temp label is one memory word, many variables use two or more.
	for (size_t i = 0; i < ctx->temp_num; i++) {
		// Find the first temp label.
		if (!strcmp(label, ctx->temp_labels[i])) {
			// Then mark all the used temp labels as not in use.
			for (size_t x = i; x < i + var->ctype->size; x++) {
				ctx->temp_usage[i] = 0;
			}
			return;
		}
	}
}



// Compare gen_var_t against each other.
bool gen_cmp(asm_ctx_t *ctx, gen_var_t *a, gen_var_t *b) {
	if (a == b) return 1;
	if (a->type != b->type) return 0;
	switch (a->type) {
		case VAR_TYPE_COND:
			return a->cond == b->cond;
		case VAR_TYPE_CONST:
			return a->iconst == b->iconst;
		case VAR_TYPE_LABEL:
			return a->label == b->label;
		case VAR_TYPE_REG:
			return a->reg == b->reg;
		case VAR_TYPE_RETVAL:
			return 1;
		case VAR_TYPE_STACKFRAME:
			return a->offset == b->offset;
		case VAR_TYPE_STACKOFFS:
			return a->offset == b->offset;
	}
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

