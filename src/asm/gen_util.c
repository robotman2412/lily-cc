
#include "gen_util.h"
#include "strmap.h"
#include "string.h"
#include "malloc.h"

// Get or create a simple type in the current scope.
var_type_t *ctype_simple(asm_ctx_t *ctx, simple_type_t of) {
	static bool did_i_init = false;
	static var_type_t list[STYPE_VOID+1];
	if (!did_i_init) {
		for (size_t i = 0; i < STYPE_VOID + 1; i++) {
			simple_type_t stype = (simple_type_t) i;
			list[i] = (var_type_t) {
				.size        = simple_type_size[stype],
				.simple_type = stype,
				.is_complete = true,
				.category    = TYPE_CAT_SIMPLE,
			};
		}
		did_i_init = true;
	}
	return &list[of];
}

// Get or create an array type of a simple type in the current scope.
// Define len as 0 if unknown.
var_type_t *ctype_arr_simple(asm_ctx_t *ctx, simple_type_t of, size_t len) {
	var_type_t *ctype = xalloc(ctx->current_scope->allocator, sizeof(var_type_t));
	*ctype = (var_type_t) {
		.size        = simple_type_size[of] * len,
		.simple_type = STYPE_VOID,
		.is_complete = !!len,
		.category    = TYPE_CAT_ARRAY,
		.underlying  = ctype_simple(ctx, of),
	};
}

// Get or create a pointer type of a simple type in the current scope.
var_type_t *ctype_ptr_simple(asm_ctx_t *ctx, simple_type_t of) {
	var_type_t *ctype = xalloc(ctx->current_scope->allocator, sizeof(var_type_t));
	*ctype = (var_type_t) {
		.size        = SSIZE_POINTER,
		.simple_type = STYPE_POINTER,
		.is_complete = true,
		.category    = TYPE_CAT_POINTER,
		.underlying  = ctype_simple(ctx, of),
	};
}

// Get or create a pointer type of a given type in the current scope.
var_type_t *ctype_ptr(asm_ctx_t *ctx, var_type_t *of) {
	var_type_t *ctype = xalloc(ctx->current_scope->allocator, sizeof(var_type_t));
	*ctype = (var_type_t) {
		.size        = SSIZE_POINTER,
		.simple_type = STYPE_POINTER,
		.is_complete = true,
		.category    = TYPE_CAT_POINTER,
		.underlying  = of,
	};
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
	ctx->temp_labels = xrealloc(ctx->allocator, ctx->temp_labels, sizeof(char *) * ctx->temp_num);
	ctx->temp_usage  = xrealloc(ctx->allocator, ctx->temp_usage,  sizeof(bool)   * ctx->temp_num);
	ctx->temp_labels[ctx->temp_num - 1] = label;
	ctx->temp_usage [ctx->temp_num - 1] = 0;
}

// Mark the label as not in use.
void gen_unuse(asm_ctx_t *ctx, gen_var_t *var) {
	// Ignore when has owner name.
	if (var->owner) return;
	
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
// Returns true when equal.
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
		case VAR_TYPE_PTR:
			return gen_cmp(ctx, a->ptr, b->ptr);
	}
}



// New scope.
void gen_push_scope(asm_ctx_t *ctx) {
	asm_scope_t *scope = (asm_scope_t *) xalloc(ctx->allocator, sizeof(asm_scope_t));
	*scope = *ctx->current_scope;
	scope->allocator   = alloc_create(ctx->allocator);
	scope->parent      = ctx->current_scope;
	map_create(&scope->vars);
	ctx->current_scope = scope;
}

// Close scope.
void gen_pop_scope(asm_ctx_t *ctx) {
	asm_scope_t *old = ctx->current_scope;
	
	// Delete the map.
	map_delete(&old->vars);
	alloc_destroy(old->allocator);
	
	// Unlink it.
	ctx->current_scope = old->parent;
	xfree(ctx->allocator, old);
}



// Creates an escaped representation of a given C-string.
char *esc_cstr(alloc_ctx_t allocator, const char *cstr, size_t len) {
	if (!cstr) return NULL;
	
	// Pass 1: count output size.
	size_t cap = 0;
	for (size_t i = 0; i < len; i++) {
		if (!cstr[i] || cstr[i] == '\a' || cstr[i] == '\b'
			 || cstr[i] == '\f' || cstr[i] == '\n' || cstr[i] == '\r'
			 || cstr[i] == '\t' || cstr[i] == '\v' || cstr[i] == '\\'
			 || cstr[i] == '\'' || cstr[i] == '\"') {
			cap += 2;
		} else if (cstr[i] < 0x20 || cstr[i] > 0x7e) {
			cap += 4;
		} else {
			cap ++;
		}
	}
	// Pass 2: generate string repr.
	char *buffer = xalloc(allocator, cap + 1);
	char *write  = buffer;
	for (size_t i = 0; i < len; i++) {
		switch (cstr[i]) {
			case 0:
				*(write++) = '\\';
				*(write++) = '0';
				break;
			case '\a':
				*(write++) = '\\';
				*(write++) = 'a';
				break;
			case '\b':
				*(write++) = '\\';
				*(write++) = 'b';
				break;
			case '\f':
				*(write++) = '\\';
				*(write++) = 'f';
				break;
			case '\n':
				*(write++) = '\\';
				*(write++) = 'n';
				break;
			case '\r':
				*(write++) = '\\';
				*(write++) = 'r';
				break;
			case '\t':
				*(write++) = '\\';
				*(write++) = 't';
				break;
			case '\v':
				*(write++) = '\\';
				*(write++) = 'v';
				break;
			case '\\':
				*(write++) = '\\';
				*(write++) = '\\';
				break;
			case '\'':
				*(write++) = '\\';
				*(write++) = '\'';
				break;
			case '\"':
				*(write++) = '\\';
				*(write++) = '\"';
				break;
			default:
				if (cstr[i] < 0x20 || cstr[i] > 0x7e) {
					// HEXHEXHEXHEX.
					char temp[3];
					sprintf(temp, "%02hhx", (uint8_t) cstr[i]);
					*(write++) = '\\';
					*(write++) = 'x';
					*(write++) = temp[0];
					*(write++) = temp[1];
				} else {
					// Default.
					*(write ++) = cstr[i];
				}
		}
	}
	*(write++) = 0;
	return buffer;
}
