
#include "ansi_codes.h"
#include "config.h"
#include "gen.h"
#include "gen_util.h"
#include "malloc.h"
#include "gen_preproc.h"
#include <string.h>
#include <stdlib.h>

/* ================== Functions ================== */

#if defined(FALLFALLBACK_gen_function) || defined(FALLBACK_gen_stmt)
static inline void gen_var_scope(asm_ctx_t *ctx, map_t *map) {
	address_t pre = ctx->current_scope->stack_size;
	
	// Add the variables to the current scope.
	for (size_t i = 0; i < map_size(map); i++) {
		DEBUG_PRE("got var '%s'\n", map->strings[i]);
		gen_var_t *var = (gen_var_t *) map->values[i];
		
		if ((var->type == VAR_TYPE_STACKFRAME || var->type == VAR_TYPE_STACKOFFS) && var->offset == (address_t) -1) {
			// Have it in the stack.
			var->offset = ctx->current_scope->stack_size;
			ctx->current_scope->stack_size ++;
		}
		
		gen_define_var(ctx, var, map->strings[i]);
	}
	
	// Update the stack size.
	DEBUG_GEN("// updating stack offset to %d\n", ctx->current_scope->stack_size);
	gen_stack_space(ctx, ctx->current_scope->stack_size - pre);
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
	ctx->current_scope->stack_size    = 0;
	gen_preproc_function(ctx, funcdef);
	
	// New function, new scope.
	gen_push_scope(ctx);
	gen_var_scope(ctx, funcdef->preproc->vars);
	
	// Mark line position.
	asm_write_pos(ctx, funcdef->pos);
	
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
	if (ctx->temp_labels) xfree(ctx->allocator, ctx->temp_labels);
	if (ctx->temp_usage)  xfree(ctx->allocator, ctx->temp_usage);
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

// Return statement for inlined functions. (stub)
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
		// Mark line position.
		asm_write_pos(ctx, stmt->pos);
		
		// One of the other statement types.
		switch (stmt->type) {
			case STMT_TYPE_EMPTY: {
				return false;
			} break;
			case STMT_TYPE_IF: {
				gen_var_t cond_hint = {
					.type = VAR_TYPE_COND
				};
				gen_var_t *cond = gen_expression(ctx, stmt->expr, &cond_hint);
				return cond && gen_if(ctx, stmt, cond, stmt->code_true, stmt->code_false);
			} break;
			case STMT_TYPE_WHILE: {
				gen_while(ctx, stmt, stmt->cond, stmt->code_true, false);
				return false;
			} break;
			case STMT_TYPE_FOR: {
				gen_push_scope(ctx);
				gen_var_scope(ctx, stmt->preproc->vars);
				gen_stmt(ctx, stmt->for_init, false);
				gen_for(ctx, stmt, stmt->for_cond, stmt->for_code, stmt->for_next);
				gen_pop_scope(ctx);
				return false;
			} break;
			case STMT_TYPE_RET: {
				// A return statement.
				gen_var_t return_hint = {
					.type  = VAR_TYPE_RETVAL,
					.ctype = ctx->current_func->returns,
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
				// Handle variable assignment status.
				for (size_t i = 0; i < stmt->vars->num; i++) {
					// Find the variable.
					gen_var_t *var = gen_get_variable(ctx, stmt->vars->arr[i].strval);
					
					if (stmt->vars->arr[i].initialiser) {
						// Perform assignment.
						expr_t *expr = stmt->vars->arr[i].initialiser;
						gen_init_var(ctx, var, expr);
					}
				}
			} break;
			case STMT_TYPE_EXPR: {
				// The expression.
				gen_var_t *result = gen_expression(ctx, stmt->expr, NULL);
				if (result) gen_unuse(ctx, result);
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
static void va_strcat(alloc_ctx_t alloc, char **dst, size_t *cap, size_t *len, char *src, size_t add_len) {
	// Ensure enough capacity.
	if (add_len + *len > *cap - 1) {
		// Expand the capacity.
		*cap = add_len + *len + 32;
		*dst = xrealloc(alloc, *dst, *cap);
	}
	// And concatenate.
	strncat(*dst, src, add_len);
	*len += add_len;
}

// Preprocessing of inline assembly text.
// Converts all the inputs and outputs found the the raw text.
static char *iasm_preproc(asm_ctx_t *ctx, iasm_t *iasm, size_t magic_number) {
	// Allocate a small bit of buffer.
	size_t cap    = strlen(iasm->text.strval) + 2 + 32 * (iasm->inputs->num + iasm->outputs->num);
	size_t len    = 0;
	char  *output = xalloc(ctx->allocator, cap);
	output[0]     = '\t';
	output[1]     = 0;
	char  *input  = iasm->text.strval;
	
	// OVERRIDE_%
	while (*input) {
		char *index = strchr(input, '%');
		char next = index ? index[1] : 0;
		if (!index || !next) {
			// There are no more things to substitute.
			va_strcat(ctx->allocator, &output, &cap, &len, input, strlen(input));
			goto done;
		} else {
			// CONCATENATE excluding the %.
			va_strcat(ctx->allocator, &output, &cap, &len, input, index - input);
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
				va_strcat(ctx->allocator, &output, &cap, &len, &next, 1);
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
				va_strcat(ctx->allocator, &output, &cap, &len, add, strlen(add));
				xfree(ctx->current_scope->allocator, add);
				break;
		}
	}
	
	done:
	return output;
}

// Whether the character fits ident restrictions, allowing for the period character.
static bool asm_is_ident(char c, bool allow_numeric) {
	if (c >= '0' && c <= '9' && allow_numeric) {
		return true;
	}
	switch (c) {
		case 'a' ... 'z':
		case 'A' ... 'Z':
		case '.':
		case '_':
			return true;
		default:
			return false;
	}
}

static inline void nomore_whitespace(tokeniser_ctx_t *lex_ctx) {
	while (1) {
		char c = tokeniser_nextchar(lex_ctx);
		if (c == '/' && tokeniser_nextchar_no(lex_ctx, 1) == '/') {
			// We go to EOL.
			do {
				c = tokeniser_readchar(lex_ctx);
			} while (c != '\r' && c != '\n');
			continue;
		}
		if (!is_space(c)) return;
		tokeniser_readchar(lex_ctx);
	}
}

static inline void nomore_spaces(tokeniser_ctx_t *lex_ctx) {
	while (1) {
		char c = tokeniser_nextchar(lex_ctx);
		if (c != ' ' && c != '\t') return;
		tokeniser_readchar(lex_ctx);
	}
}

// Test whether the first next data matches the pattern for an ident.
// Also allows for the period '.' character.
// Returns the length found if true, 0 otherwise.
static size_t asm_expect_ident(tokeniser_ctx_t *lex_ctx) {
	bool allow_numeric = false;
	size_t offset = 0;
	while (true) {
		char next = tokeniser_nextchar_no(lex_ctx, offset);
		if (!asm_is_ident(next, allow_numeric)) {
			return offset;
		}
		allow_numeric = true;
		offset ++;
	}
}

// Expect a string and if so, return it.
static char *asm_expect_str(tokeniser_ctx_t *lex_ctx) {
	nomore_spaces(lex_ctx);
	char c = tokeniser_nextchar(lex_ctx);
	if (c == '\'' || c == '\"') {
		tokeniser_readchar(lex_ctx);
		return tokeniser_getstr(lex_ctx, c);
	} else {
		return NULL;
	}
}

// Expect an ival and if so, return it.
static bool asm_expect_ival(tokeniser_ctx_t *lex_ctx, long long *out) {
	bool negative = false;
	retry:
	nomore_spaces(lex_ctx);
	char c = tokeniser_nextchar(lex_ctx);
	char next = tokeniser_nextchar_no(lex_ctx, 1);
	
	// Negative?
	if (c == '-') {
		tokeniser_readchar(lex_ctx);
		negative = !negative;
		goto retry;
	}
	
	// This could be hexadecimal.
	if (c == '0' && (next == 'x' || next == 'X')) {
		int offs = 2;
		while (is_hexadecimal(tokeniser_nextchar_no(lex_ctx, offs))) offs++;
		// Skip the 0x.
		tokeniser_readchar(lex_ctx);
		tokeniser_readchar(lex_ctx);
		// Now, grab it.
		offs --;
		char *strval = (char *) xalloc(lex_ctx->allocator, sizeof(char) * offs);
		strval[offs] = 0;
		for (int i = 0; i < offs-1; i++) {
			strval[i] = tokeniser_readchar(lex_ctx);
		}
		// Turn it into a number, hexadecimal.
		*out = strtoull(strval, NULL, 16);
		if (negative) *out = -*out;
		xfree(lex_ctx->allocator, strval);
		return true;
	}
	
	// This could be a number.
	if (is_numeric(c)) {
		// Check how many of these we get.
		int offs = 1;
		while (is_alphanumeric(tokeniser_nextchar_no(lex_ctx, offs))) offs++;
		// Skip the first digit.
		tokeniser_readchar(lex_ctx);
		// Now, grab it.
		char *strval = (char *) xalloc(lex_ctx->allocator, sizeof(char) * (offs + 1));
		*strval = c;
		strval[offs] = 0;
		for (int i = 1; i < offs; i++) {
			strval[i] = tokeniser_readchar(lex_ctx);
		}
		// Turn it into a number, respecting octal.
		*out = strtoull(strval, NULL, c == '0' ? 8 : 10);
		if (negative) *out = -*out;
		xfree(lex_ctx->allocator, strval);
		return true;
	}
    
	return false;
}

// Extracts len characters from lex_ctx in a c-string.
static char *asm_extract(tokeniser_ctx_t *lex_ctx, size_t len) {
	char *cstr = xalloc(lex_ctx->allocator, len + 1);
	for (size_t i = 0; i < len; i++) {
		cstr[i] = tokeniser_readchar(lex_ctx);
	}
	cstr[len] = 0;
	return cstr;
}

// Handles the .db macro in assembly.
static void gen_asm_db(asm_ctx_t *ctx, tokeniser_ctx_t *lex_ctx) {
	gimme:
	nomore_spaces(lex_ctx);
	pos_t pre_pos = pos_empty(lex_ctx);
	
	// Try strconst.
	char *str = asm_expect_str(lex_ctx);
	if (str) {
		// Note file position.
		asm_write_pos(ctx, pos_merge(pre_pos, pos_empty(lex_ctx)));
		
		// It worked!
		for (size_t i = 0; i < strlen(str); i++) {
			asm_write_memword(ctx, str[i]);
		}
		goto next;
	}
	// Try ident.
	size_t len = asm_expect_ident(lex_ctx);
	if (len) {
		// Note file position.
		asm_write_pos(ctx, pos_merge(pre_pos, pos_empty(lex_ctx)));
		
		// TODO: More fancy impl.
		char *ident = asm_extract(lex_ctx, len);
		asm_write_label_ref(ctx, ident, 0, ASM_LABEL_REF_ABS_PTR);
		xfree(lex_ctx->allocator, ident);
		goto next;
	}
	// Try ival.
	long long ival;
	bool success = asm_expect_ival(lex_ctx, &ival);
	if (success) {
		// Note file position.
		asm_write_pos(ctx, pos_merge(pre_pos, pos_empty(lex_ctx)));
		
		// APPEND integer.
		asm_write_memword(ctx, ival);
		goto next;
	}
	// It didn't work.
	printf("Error: Expected STRING, IDENT or IVAL after .db\n");
	return;
	
	next:
	if (tokeniser_nextchar(lex_ctx) == ',') {
		tokeniser_readchar(lex_ctx);
		goto gimme;
	}
}

// Complete file of assembly. (only if inline assembly is supported)
// Splits the assembly file into lines and nicely asks the backend to fix it.
void gen_asm_file(asm_ctx_t *ctx, tokeniser_ctx_t *lex_ctx) {
	
	// While there is things to process.
	while (tokeniser_nextchar(lex_ctx)) {
		// Remove spaces.
		nomore_whitespace(lex_ctx);
		
		// Check for a label.
		size_t len = asm_expect_ident(lex_ctx);
		if (len && tokeniser_nextchar_no(lex_ctx, len) == ':') {
			// CONSUME label.
			char *label = asm_extract(lex_ctx, len);
			asm_write_label(ctx, label);
			xfree(lex_ctx->allocator, label);
			tokeniser_readchar(lex_ctx);
		}
		
		// Check for directives.
		if (tokeniser_nextchar(lex_ctx) == '.') {
			tokeniser_readchar(lex_ctx);
			// Handle directive.
			size_t len = asm_expect_ident(lex_ctx);
			char *directive = asm_extract(lex_ctx, len);
			if (!strcmp(directive, "text") || !strcmp(directive, "bss")
			|| !strcmp(directive, "rodata") || !strcmp(directive, "data")) {
				asm_use_sect(ctx, directive, ASM_NOT_ALIGNED);
			} else if (!strcmp(directive, "section")) {
				char *sect_id = asm_expect_str(lex_ctx);
				if (sect_id) {
					asm_use_sect(ctx, sect_id, ASM_NOT_ALIGNED);
					xfree(lex_ctx->allocator, sect_id);
				} else {
					printf("Error: Expected STRING after '.section'\n");
				}
			} else if (!strcmp(directive, "zero")) {
				// Zero-ed bytes instertion.
				long long count;
				bool success = asm_expect_ival(lex_ctx, &count);
				if (success) {
					// Found the ival.
					asm_write_zero(ctx, count);
				} else {
					// Expected a ival, but did not get it.
					printf("Error: Expected NUMBER after '.zero'\n");
				}
			} else if (!strcmp(directive, "db")) {
				// Handle the D.B.
				gen_asm_db(ctx, lex_ctx);
			} else {
				printf("Warning: Unknown directive '%s'\n", directive);
			}
			xfree(lex_ctx->allocator, directive);
			// TODO: Enforce end of line.
		} else {
			// Hand it off.
			gen_asm(ctx, lex_ctx);
		}
	}
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
			printf("// Error: output operand constraint '%s' lacks '='\n", reg->mode);
			return;
		}
		
		if (iasm->outputs->arr[i].symbol) {
			DEBUG_GEN("// Output '%s' mode '%s'\n", reg->symbol, reg->mode);
		} else {
			DEBUG_GEN("// Output anonymous mode '%s'\n", reg->mode);
		}
	}
	// Process inputs.
	for (size_t i = 0; i < iasm->inputs->num; i++) {
		// Process the constraints string.
		if (!iasm_preproc_reg(ctx, iasm->inputs, i)) return;
		iasm_reg_t *reg = &iasm->inputs->arr[i];
		
		// Input operands may not be written to.
		if (reg->mode_write) {
			printf("// Error: input operand constraint '%s' contains '%c'\n", reg->mode, reg->mode_read ? '+' : '=');
			return;
		}
		
		if (reg->symbol) {
			DEBUG_GEN("// Input '%s' mode '%s'\n", reg->symbol, reg->mode);
		} else {
			DEBUG_GEN("// Input anonymous mode '%s'\n", reg->mode);
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
	
	// Process clobbers, inputs and outputs.
	char *text = iasm_preproc(ctx, iasm, 0);
	// Now treat this like a normal assembly file.
	tokeniser_ctx_t lex_ctx;
	tokeniser_init_cstr(&lex_ctx, text);
	gen_asm_file(ctx, &lex_ctx);
	// And finish up.
	xfree(ctx->allocator, text);
}
#else
// Complete file of assembly.
void gen_asm_file(asm_ctx_t *ctx, tokeniser_ctx_t *lex) {
	// Inline assembly support does not exist.
	printf("Error: inline assembly is not supported!\n");
}

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
			gen_var_t *val = (gen_var_t *) xalloc(ctx->allocator, sizeof(gen_var_t));
			*val = (gen_var_t) {
				.type   = VAR_TYPE_CONST,
				.iconst = expr->iconst,
				.ctype  = ctype_simple(ctx, STYPE_S_INT),
			};
			return val;
		} break;
		
		case EXPR_TYPE_CSTR: {
			// C-strings are label references.
			gen_var_t *val = (gen_var_t *) xalloc(ctx->allocator, sizeof(gen_var_t));
			*val = (gen_var_t) {
				.type   = VAR_TYPE_LABEL,
				.label  = expr->label,
				.ctype  = ctype_ptr_simple(ctx, STYPE_CHAR),
			};
			return gen_expr_math1(ctx, expr, OP_ADROF, NULL, val);
		} break;
		
		case EXPR_TYPE_IDENT: {
			// Variable references.
			gen_var_t *val = gen_get_variable(ctx, expr->ident->strval);
			if (!val) {
				// Is it maybe a function?
				funcdef_t *func = map_get(&ctx->functions, expr->ident->strval);
				if (func) {
					// It's a function, so make a label variable out of it.
					val = (gen_var_t *) xalloc(ctx->allocator, sizeof(gen_var_t));
					*val = (gen_var_t) {
						.type   = VAR_TYPE_LABEL,
						.label  = expr->ident->strval,
						.ctype  = ctype_ptr_simple(ctx, STYPE_VOID),
					};
				} else {
					// Neither function nor variable, so it's an error.
					report_errorf(ctx->tokeniser_ctx, E_ERROR, expr->pos, "Identifier '%s' is undefined", expr->ident->strval);
					return NULL;
				}
			}
			val->owner = expr->ident->strval;
			return val;
		} break;
		
		case EXPR_TYPE_CALL: {
			// Mark line position.
			asm_write_pos(ctx, expr->pos);
			
			// TODO: check for inlining possibilities.
			
			// For now, enforce that func is always an ident.
			// Then, look up it's funcdef.
			funcdef_t *funcdef;
			if (expr->func->type == EXPR_TYPE_IDENT) {
				// Funcdef lookup.
				const char *name = expr->func->ident->strval;
				funcdef = map_get(&ctx->functions, name);
				
				if (!funcdef) {
					// Undefined function called.
					report_error(ctx->tokeniser_ctx, E_ERROR, expr->func->pos, "WIP: Implicit function definitions are unsupported.");
					return NULL;
				}
			} else {
				// Simply reject all other scenarios.
				report_error(ctx->tokeniser_ctx, E_ERROR, expr->func->pos, "WIP: Function calls other than by name are unsupported.");
				return NULL;
			}
			
			return gen_expr_call(ctx, funcdef, expr->func, expr->args->num, expr->args->arr);
		} break;
		
		case EXPR_TYPE_MATH1: {
			// Mark line position.
			asm_write_pos(ctx, expr->pos);
			
			// Unary math operations (things like ++a, &b, *c and !d)
			oper_t oper = expr->oper;
			if (oper == OP_DEREF && out_hint && out_hint->type == VAR_TYPE_PTR) {
				// This happens when writing to a pointer dereference.
				out_hint->ptr = gen_expression(ctx, expr->par_a, NULL);
				return out_hint->ptr ? out_hint : NULL;
				
			} else if (oper == OP_LOGIC_NOT) {
				// Apply the "condition" output hint to logic not.
				gen_var_t *cond_hint = xalloc(ctx->current_scope->allocator, sizeof(gen_var_t));
				*cond_hint = (gen_var_t) {
					.type = VAR_TYPE_COND,
					.ctype = ctype_simple(ctx, STYPE_BOOL),
				};
				// And do the rest as usual.
				gen_var_t *a = gen_expression(ctx, expr->par_a, cond_hint);
				return a ? gen_expr_math1(ctx, expr, expr->oper, out_hint, a) : NULL;
				
			} else {
				// Simple expression.
				gen_var_t *a = gen_expression(ctx, expr->par_a, out_hint);
				return a ? gen_expr_math1(ctx, expr, expr->oper, out_hint, a) : NULL;
			}
		} break;
		
		case EXPR_TYPE_MATH2: {
			// Mark line position.
			asm_write_pos(ctx, expr->pos);
			
			if (expr->oper == OP_ASSIGN) {
				// Handle assignment seperately.
				if (expr->par_a->type == EXPR_TYPE_IDENT) {
					// Assignment to an identity (explicit variable).
					gen_var_t *a = gen_expression(ctx, expr->par_a, NULL);
					if (!a) return NULL;
					gen_var_t *b = gen_expression(ctx, expr->par_b, a);
					if (!b && a) gen_unuse(ctx, a);
					if (!b) return NULL;
					// Have the move performed.
					gen_mov(ctx, a, b);
					// Free up variables if necessary.
					if (!gen_cmp(ctx, a, b)) gen_unuse(ctx, a);
					return a;
					
				} else {
					// Assignment to a different type (usually pointer dereference).
					gen_var_t *ptr_hint = xalloc(ctx->current_scope->allocator, sizeof(gen_var_t));
					*ptr_hint = (gen_var_t) {
						.type = VAR_TYPE_PTR,
						.ctype = ctype_simple(ctx, STYPE_BOOL),
					};
					// Generate with the pointer hint.
					gen_var_t *a = gen_expression(ctx, expr->par_a, ptr_hint);
					if (!a) return NULL;
					// Enforce that A is writable.
					if (a->type == VAR_TYPE_COND || a->type == VAR_TYPE_CONST) {
						// These aren't writable.
						report_error(ctx->tokeniser_ctx, E_ERROR, expr->par_a->pos, "Left hand of an assignment must be assignable.");
						gen_unuse(ctx, a);
						return NULL;
					}
					gen_var_t *b = gen_expression(ctx, expr->par_b, a);
					if (!b && a) gen_unuse(ctx, a);
					if (!b) return NULL;
					// Have the move performed.
					gen_mov(ctx, a, b);
					// Free up variables if necessary.
					if (!gen_cmp(ctx, a, b)) gen_unuse(ctx, a);
					return a;
				}
			} else {
				// Simple binary math (things like a * b, c + d, e & f, etc.)
				gen_var_t *a   = gen_expression(ctx, expr->par_a, out_hint);
				if (!a) return NULL;
				gen_var_t *b   = gen_expression(ctx, expr->par_b, NULL);
				if (!b && a) gen_unuse(ctx, a);
				if (!b) return NULL;
				gen_var_t *out = gen_expr_math2(ctx, expr, expr->oper, out_hint, a, b);
				if (!out && a) gen_unuse(ctx, a);
				if (!out && b) gen_unuse(ctx, b);
				if (!out) return NULL;
				// Free up variables if necessary.
				if (!gen_cmp(ctx, a, out)) gen_unuse(ctx, a);
				if (!gen_cmp(ctx, b, out)) gen_unuse(ctx, b);
				return out;
			}
		} break;
	}
	printf("\n\nOH SHIT\n");
	exit(23);
}
#endif

