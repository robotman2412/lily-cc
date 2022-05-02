
#include "gen_util.h"
#include "strmap.h"
#include "string.h"
#include "malloc.h"

// Free an instance of gen_var_t and it's underlying data.
void free_gen_var(gen_var_t *ptr) {
	if (ptr->type == VAR_TYPE_PTR) free_gen_var(ptr->ptr);
	if (ptr->default_loc) free_gen_var(ptr->default_loc);
	free(ptr);
}

// Find and return the location of the variable with the given name.
gen_var_t *gen_get_variable(asm_ctx_t *ctx, char *label) {
	asm_scope_t *scope = ctx->current_scope;
	while (scope) {
		gen_var_t *var = (gen_var_t *) map_get(&scope->vars, label);
		if (var) return var;
		scope = scope->parent;
	}
	return NULL;
}

// Define the variable with the given ident.
bool gen_define_var(asm_ctx_t *ctx, gen_var_t *var, char *ident) {
	void *repl = map_set(&ctx->current_scope->vars, ident, var);
	if (repl) return false;
	
	if (ctx->current_scope != &ctx->global_scope) {
		// Don't want to deal with global variable numbers inside functions.
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
	// Mark registers as free.
	if (var->type == VAR_TYPE_REG) {
		ctx->current_scope->reg_usage[var->reg] = NULL;
	}
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
	if (!a && b) return 0;
	if (!b && a) return 0;
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
	memcpy(scope->reg_usage, ctx->current_scope->reg_usage, sizeof(scope->reg_usage));
	scope->stack_size = ctx->stack_size;
	map_create(&scope->vars);
	ctx->current_scope = scope;
}

// Close scope.
void gen_pop_scope(asm_ctx_t *ctx) {
	asm_scope_t *old = ctx->current_scope;
	
	// Delete the map.
	map_delete_with_values(&old->vars);
	
	// Unlink it.
	ctx->current_scope = old->parent;
	ctx->stack_size = old->stack_size;
	free(old);
}

