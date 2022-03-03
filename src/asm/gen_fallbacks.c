
#include "config.h"
#include "gen.h"
#include "gen_util.h"
#include "malloc.h"
#include "gen_preproc.h"
#include <string.h>

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
	ctx->is_inline     = false;
	ctx->current_func  = funcdef;
	ctx->temp_labels   = NULL;
	ctx->temp_usage    = NULL;
	ctx->last_label_no = 0;
	ctx->temp_num      = 0;
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
	if (ctx->temp_labels) free(ctx->temp_labels);
	if (ctx->temp_usage)  free(ctx->temp_usage);
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
	stmt_t *stmt      = ptr;
	bool has_scope    = false;
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
				gen_while(ctx, stmt->cond, stmt->code_true, false);
				return false;
			} break;
			case STMT_TYPE_RET: {
				// A return statement.
				gen_var_t return_hint = {
					.type = VAR_TYPE_RETVAL
				};
				gen_var_t *retval = stmt->expr ? gen_expression(ctx, stmt->expr, &return_hint) : 0;
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
				gen_unuse(ctx, result);
				free(result);
			} break;
			case STMT_TYPE_IASM: {
				// Inline assembly for the win!
				gen_inline_asm(ctx, stmt->iasm);
			} break;
		}
	}
	if (has_scope) {
		gen_pop_scope(ctx);
	}
	return explicit_ret;
}
#endif

#ifdef INLINE_ASM_SUPPORTED
// Decodes a constraints string.
// Returns 1 on success.
static bool iasm_preproc_reg(asm_ctx_t *ctx, iasm_regs_t *regs, int index) {
	iasm_reg_t *reg = &regs->arr[index];
	reg->mode_read = true;
	char *str = reg->mode;
	
	// Initialise all constraints.
	reg->mode_read = false;
	reg->mode_write = false;
	reg->mode_early_clobber = false;
	reg->mode_known_const = false;
	reg->mode_unknown_const = false;
	reg->mode_register = false;
	reg->mode_memory = false;
	reg->mode_commutative_next = false;
	reg->mode_commutative_prev = false;
	
	// COMPUTE the string.
	for (int i = 0; *str; i++) {
		char c = *str;
		switch (c) {
			case ' ':
			case '\t':
			case '\r':
			case '\n':
				// Ignore whitespace.
				break;
				
			case '&':
				// Early clobber: this may not be in the same place as an input.
				reg->mode_early_clobber = true;
				break;
				
			case '%':
				// Commutative with the next one.
				if (index < regs->num - 1) {
					reg->mode_commutative_next = true;
					regs->arr[index + 1].mode_commutative_prev = true;
				} else {
					printf("Error: '%%' constraint used with last operand\n");
					return false;
				}
				break;
				
			case '=':
				// Write-only.
				reg->mode_read = false;
			case '+':
				// Read-write.
				reg->mode_write = true;
				break;
				
			case 'r':
				// Allow registers.
				reg->mode_register = true;
				break;
				
			case 'm':
				// Allow memory.
				reg->mode_memory = true;
				break;
				
			case 'i':
				// Allow integers whose value is not necessarily known.
				reg->mode_unknown_const = true;
			case 's':
			case 'n':
				// Allow integers whose value is already known.
				reg->mode_known_const = true;
				break;
				
			case 'g':
			case 'X':
				// Allow anything.
				reg->mode_register = true;
				reg->mode_memory = true;
				reg->mode_known_const = true;
				reg->mode_unknown_const = true;
				break;
				
			default:
				// Unknown stuff, we'll ignore the rest of the constraint.
				printf("Warning: Unrecognised constraint '%c'\n", c);
				return false;
		}
		str ++;
	}
	
	// Test the constraint to ensure it is possible.
	if (!reg->mode_register && !reg->mode_memory && !reg->mode_known_const) {
		// If no type of thing is allowed, then it is impossible.
		printf("Error: Impossible constraint '%s'\n", reg->mode);
		return false;
	}
	
	return true;
}

// Helper for the string concatenation magic.
static void va_strcat(char **dst, size_t *cap, size_t *len, char *src, size_t add_len) {
	// Ensure enough capacity.
	if (add_len + *len > *cap - 1) {
		// Expand the capacity.
		*cap = add_len + *len + 32;
		*dst = realloc(*dst, *cap);
	}
	// And concatenate.
	strncat(*dst, src, add_len);
	*len += add_len;
}

// Preprocessing of inline assembly text.
// Converts all the inputs and outputs found the the raw text.
static char *iasm_preproc(asm_ctx_t *ctx, iasm_t *iasm, size_t magic_number) {
	// Allocate a small bit of buffer.
	size_t cap    = strlen(iasm->text.strval) + 32 * (iasm->inputs->num + iasm->outputs->num);
	size_t len    = 0;
	char  *output = malloc(cap);
	*output       = 0;
	char  *input  = iasm->text.strval;
	
	// OVERRIDE_%
	while (*input) {
		char *index = strchr(input, '%');
		char next = index ? index[1] : 0;
		if (!index || !next) {
			// There are no more things to substitute.
			va_strcat(&output, &cap, &len, input, strlen(input));
			goto done;
		} else {
			// CONCATENATE excluding the %.
			va_strcat(&output, &cap, &len, input, index - input);
			input = index + 2;
		}
		
		// COMPUTE the %.
		switch (next) {
				char *add;
				
			case '%':
			case '{':
			case '|':
			case '}':
				// Just dump it out.
				va_strcat(&output, &cap, &len, &next, 1);
				break;
				
			case '0' ... '9':
				// Decode the DECIMalATE.
				// And the pick an OPERAND.
				break;
				
			case '[':
				// Grab the string inbetween the [].
				index = strchr(index, ']');
				
				// And find the matching operand, of.
				iasm_reg_t *match = NULL;
				for (size_t i = 0; i < iasm->outputs->num; i++) {
					char *symbol = iasm->outputs->arr[i].symbol;
					if (symbol && !strncmp(input, symbol, index - input)) {
						// We found a match.
						match = &iasm->outputs->arr[i];
						// Now exclude the ].
						input = index + 1;
						goto wegotmatch;
					}
				}
				for (size_t i = 0; i < iasm->inputs->num; i++) {
					char *symbol = iasm->inputs->arr[i].symbol;
					if (symbol && !strncmp(input, symbol, index - input)) {
						// We found a match.
						match = &iasm->inputs->arr[i];
						// Now exclude the ].
						input = index + 1;
						goto wegotmatch;
					}
				}
				break;
				
				wegotmatch:
				// Compute the funny input operand.
				add = gen_iasm_var(ctx, match->expr_result, match);
				va_strcat(&output, &cap, &len, add, strlen(add));
				free(add);
				break;
		}
	}
	
	done:
	return output;
}

// Inline assembly implementation.
void gen_inline_asm(asm_ctx_t *ctx, iasm_t *iasm) {
	// Used to assert that, including clobbers, there are enough free registers.
	size_t num_input_reg  = 0;
	size_t num_output_reg = 0;
	
	// Process outputs.
	for (size_t i = 0; i < iasm->outputs->num; i++) {
		// Process the constraints string.
		if (!iasm_preproc_reg(ctx, iasm->outputs, i)) return;
		iasm_reg_t *reg = &iasm->outputs->arr[i];
		
		// Output operands must be marked as written to (even if they aren't).
		if (!reg->mode_write) {
			printf("Error: output operand constraint '%s' lacks '='\n", reg->mode);
			return;
		}
		
		if (iasm->outputs->arr[i].symbol) {
			DEBUG_GEN("Output '%s' mode '%s'\n", reg->symbol, reg->mode);
		} else {
			DEBUG_GEN("Output anonymous mode '%s'\n", reg->mode);
		}
	}
	
	// Process inputs.
	for (size_t i = 0; i < iasm->inputs->num; i++) {
		// Process the constraints string.
		if (!iasm_preproc_reg(ctx, iasm->inputs, i)) return;
		iasm_reg_t *reg = &iasm->inputs->arr[i];
		
		// Input operands may not be written to.
		if (reg->mode_write) {
			printf("Error: input operand constraint '%s' contains '%c'\n", reg->mode, reg->mode_read ? '+' : '=');
			return;
		}
		
		if (reg->symbol) {
			DEBUG_GEN("Input '%s' mode '%s'\n", reg->symbol, reg->mode);
		} else {
			DEBUG_GEN("Input anonymous mode '%s'\n", reg->mode);
		}
	}
	
	// Now, process output expressions.
	for (size_t i = 0; i < iasm->outputs->num; i++) {
		iasm_reg_t *reg = &iasm->outputs->arr[i];
		reg->expr_result = gen_expression(ctx, reg->expr, NULL);
	}
	// And input expression.
	for (size_t i = 0; i < iasm->inputs->num; i++) {
		iasm_reg_t *reg = &iasm->inputs->arr[i];
		reg->expr_result = gen_expression(ctx, reg->expr, NULL);
	}
	
	// TODO: Clobbers, inputs and outputs.
	char *text = iasm_preproc(ctx, iasm, 0);
	DEBUG_GEN("Final assembly:\n\t%s\n", text);
	gen_asm(ctx, text);
	free(text);
}
#else
// Inline assembly implementation.
void gen_inline_asm(asm_ctx_t *ctx, iasm_t *iasm) {
	// Inline assembly support does not exist.
	printf("Error: inline assembly is not supported!\n");
}
#endif

/* ================= Expressions ================= */

// Generic fallback for expressions that doesn't take into account any architecture-specific optimisations.
#ifdef FALLBACK_gen_expression
// Every aspect of the expression to be written. (generic fallback)
gen_var_t *gen_expression(asm_ctx_t *ctx, expr_t *expr, gen_var_t *out_hint) {
	switch (expr->type) {
		case EXPR_TYPE_CONST: {
			// Constants are very simple.
			gen_var_t *val = (gen_var_t *) malloc(sizeof(gen_var_t));
			*val = (gen_var_t) {
				.type = VAR_TYPE_CONST,
				.iconst = expr->iconst
			};
			return val;
		} break;
		
		case EXPR_TYPE_IDENT: {
			// Variable references.
			gen_var_t *val = (gen_var_t *) malloc(sizeof(gen_var_t));
			*val = (gen_var_t) {
				.type = VAR_TYPE_LABEL,
				.label = gen_translate_label(ctx, expr->ident->strval)
			};
			if (!val->label) {
				// TODO: Some fix or error report goes here.
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
			// Unary math operations (things like ++a, &b, *c and !d)
			oper_t oper = expr->oper;
			if (oper == OP_DEREF && out_hint && out_hint->type == VAR_TYPE_PTR) {
				// This happens when writing to a pointer dereference.
				out_hint->ptr = gen_expression(ctx, expr->par_a, NULL);
				return out_hint;
				
			} else if (oper == OP_LOGIC_NOT) {
				// Apply the "condition" output hint to logic not.
				gen_var_t *cond_hint = COPY(&(gen_var_t) {
					.type = VAR_TYPE_COND
				}, gen_var_t);
				// And do the rest as usual.
				gen_var_t *a = gen_expression(ctx, expr->par_a, cond_hint);
				return gen_expr_math1(ctx, expr->oper, out_hint, a);
				
			} else {
				// Simple expression.
				gen_var_t *a = gen_expression(ctx, expr->par_a, out_hint);
				return gen_expr_math1(ctx, expr->oper, out_hint, a);
			}
		} break;
		
		case EXPR_TYPE_MATH2: {
			if (expr->oper == OP_ASSIGN) {
				// Handle assignment seperately.
				if (expr->par_a->type == EXPR_TYPE_IDENT) {
					// Assignment to an identity (explicit variable).
					gen_var_t *a = gen_expression(ctx, expr->par_a, NULL);
					gen_var_t *b = gen_expression(ctx, expr->par_b, a);
					// Have the move performed.
					gen_mov(ctx, a, b);
					// Free up variables if necessary.
					if (!gen_cmp(ctx, a, b)) gen_unuse(ctx, a);
					return a;
					
				} else {
					// Assignment to a different type (usually pointer dereference).
					gen_var_t *ptr_hint = COPY(&(gen_var_t) {
						.type = VAR_TYPE_PTR
					}, gen_var_t);
					// Generate with the pointer hint.
					gen_var_t *a = gen_expression(ctx, expr->par_a, ptr_hint);
					gen_var_t *b = gen_expression(ctx, expr->par_b, NULL);
					// Have the move performed.
					gen_mov(ctx, a, b);
					// Free up variables if necessary.
					if (!gen_cmp(ctx, a, b)) gen_unuse(ctx, a);
					return a;
				}
			} else {
				// Simple binary math (things like a * b, c + d, e & f, etc.)
				gen_var_t *a   = gen_expression(ctx, expr->par_a, out_hint);
				gen_var_t *b   = gen_expression(ctx, expr->par_b, NULL);
				gen_var_t *out = gen_expr_math2(ctx, expr->oper, out_hint, a, b);
				// Free up variables if necessary.
				if (!gen_cmp(ctx, a, out)) gen_unuse(ctx, a);
				if (!gen_cmp(ctx, b, out)) gen_unuse(ctx, b);
				return out;
			}
		} break;
	}
	printf("\n\nOH SHIT\n");
}
#endif

