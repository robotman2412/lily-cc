
#include "asm.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <gen.h>

/* ================ Utilities ================= */
// Utility functions and definitions.

#define ASM_DEF_CLABELS 16
#define ASM_INC_CLABELS 16
#define ASM_DEF_CASMDAT 32
#define ASM_INC_CASMDAT 32

#define ASM_TYPE_RAW   0x13
#define ASM_TYPE_LABEL 0xc4
#define ASM_TYPE_LBREF 0x2f

static bool asm_shit = true;

// Append a raw ASM byte.
static void asm_appendraw(asm_ctx_t *ctx, uint8_t value) {
	if (ctx->nAsmData >= ctx->cAsmData) {
		ctx->cAsmData += ASM_INC_CASMDAT;
		ctx->asmData = (uint8_t *) realloc(ctx->asmData, sizeof(uint8_t) * ctx->cAsmData);
	}
	ctx->asmData[ctx->nAsmData] = value;
	ctx->nAsmData ++;
}

// Append an N byte number.
static void asm_appendnum(asm_ctx_t *ctx, size_t value, uint8_t nBytes) {
	if (nBytes == 1) {
		if (asm_shit) printf("ASM %02lx\n", value);
		asm_appendraw(ctx, (uint8_t) value);
		return;
	}
	if (asm_shit) printf("ASM %04lx\n", value);
#if USE_LITTLE_ENDIAN
	for (uint8_t i = 0; i < nBytes; i++) {
		asm_appendraw(ctx, (uint8_t) value);
		value >>= 8;
	}
#else
	for (uint8_t i = 0; i < nBytes; i++) {
		asm_appendraw(ctx, (uint8_t) (value >> (sizeof(size_t) - 1) * 8));
		value <<= 8;
	}
#endif
}

static size_t asm_add_or_get_label(asm_ctx_t *ctx, label_t label) {
	// Check for existing labels.
	for (size_t i = 0; i < ctx->nLabels; i++) {
		if (!strcmp(label, ctx->labels[i])) {
			return i;
		}
	}
	// Make a new label.
	if (ctx->nLabels >= ctx->cLabels) {
		ctx->cLabels += ASM_INC_CASMDAT;
		ctx->labels = (char **) realloc(ctx->labels, sizeof(char *) * ctx->cLabels);
	}
	ctx->labels[ctx->nLabels] = strdup(label);
	ctx->nLabels ++;
	return ctx->nLabels - 1;
}

static bool asm_label_exists(asm_ctx_t *ctx, label_t label) {
	// Check for existing labels.
	for (size_t i = 0; i < ctx->nLabels; i++) {
		if (!strcmp(label, ctx->labels[i])) {
			return true;
		}
	}
	return false;
}

// Read an N byte number.
static size_t asm_readnum(asm_ctx_t *ctx, size_t offset, uint8_t nBytes) {
	
}


/* ================== Setup =================== */
// Setup and post processing.

// Initialise the ASM context for code generation.
void asm_init(asm_ctx_t *ctx) {
	*ctx = (asm_ctx_t) {
		.labelno = 0,
		.nLabels = 0,
		.cLabels = ASM_DEF_CLABELS,
		.labels = (char **) malloc(sizeof(char *) * ASM_DEF_CLABELS),
		.nWords = 0,
		.words = NULL,
		.nAsmData = 0,
		.cAsmData = ASM_DEF_CASMDAT,
		.asmData = (uint8_t *) malloc(sizeof(uint8_t) * ASM_DEF_CASMDAT),
		.isPostProc = 0
	};
	ctx->scope = (asm_scope_t *) malloc(sizeof(asm_scope_t));
	ctx->scope->parent = NULL;
	ctx->scope->isLoop = false;
	map_create(&ctx->scope->variables);
	map_create(&ctx->scope->upper);
	gen_init(ctx);
}

// Post-process ASM to make binary.
void asm_postproc(asm_ctx_t *ctx) {
	ctx->words = (memword_t *) malloc(sizeof(memword_t) * ctx->nWords);
	size_t ptr = 0;
	// Offset at which the code is placed after assembling.
	size_t outputOffset = 0;
	// Pass 1: Find label addresses.
	address_t currentAddress = 0;
	map_t labelMap;
	map_create(&labelMap);
	while (ptr < ctx->nAsmData) {
		uint8_t type = ctx->asmData[ptr];
		ptr ++;
		if (ptr >= ctx->nAsmData) {
			fflush(stdout);
			fputs("Warning: Missing ASM data. This is a bug.\n", stderr);
			fflush(stderr);
			return;
		}
		switch (type) {
			case (ASM_TYPE_LABEL):
				break;
			case (ASM_TYPE_LBREF):
				break;
			case (ASM_TYPE_RAW):
				currentAddress += asm_readnum(ctx, ptr, 1);
				ptr ++;
				break;
		}
	}
}

/* ================ Appending ================= */
// Appending ASM data.

// Create a label that will reside in bss, one word in size.
void asm_bss_label(asm_ctx_t *ctx, label_t label) {
	asm_bss_label_n(ctx, label, 1);
}

// Create a label that will reside in bss, size words in size.
void asm_bss_label_n(asm_ctx_t *ctx, label_t label, address_t size) {
	if (asm_label_exists(ctx, label)) {
		fprintf(stderr, "Error: Label '%s' already exists!\n", label);
		fflush(stderr);
	}
	asm_add_or_get_label(ctx, label);
	asm_data_t *data = malloc(sizeof(asm_data_t));
	*data = (asm_data_t) {
		.label = label,
		.size = size,
		.data = NULL
	};
	map_set(&ctx->bssLabels, label, data);
	printf("BSS %s (%d)\n", label, size);
}

// Create a label that will reside in data, one word in size.
void asm_data_label(asm_ctx_t *ctx, label_t label, memword_t value) {
	memword_t *mem = malloc(sizeof(memword_t));
	*mem = value;
	asm_data_label_n(ctx, label, 1, mem);
}

// Create a label that will reside in data, size words in size.
// Will not copy provided data.
void asm_data_label_n(asm_ctx_t *ctx, label_t label, address_t size, memword_t *value) {
	if (asm_label_exists(ctx, label)) {
		fprintf(stderr, "Error: Label '%s' already exists!\n", label);
		fflush(stderr);
		asm_data_t *delete = map_get(&ctx->dataLabels, label);
		if (delete) {
			free(delete->data);
			free(delete);
		}
	}
	asm_add_or_get_label(ctx, label);
	asm_data_t *data = malloc(sizeof(asm_data_t));
	*data = (asm_data_t) {
		.label = label,
		.size = size,
		.data = value
	};
	map_set(&ctx->dataLabels, label, data);
	printf("DAT %s (%d)\n", label, size);
}

// Append a label definition.
// Will duplicate the given string.
void asm_label(asm_ctx_t *ctx, label_t label) {
	ctx->rawCountIndex = 0;
	size_t index = asm_add_or_get_label(ctx, label);
	asm_shit = false;
	asm_appendraw(ctx, ASM_TYPE_LABEL);
	asm_appendnum(ctx, index, MEMW_BYTES);
	asm_shit = true;
	printf("LAB '%s'\n", label);
}

// Create a numbered label.
label_t asm_numbered_label(asm_ctx_t *ctx) {
	char *buf = (char *) malloc(10);
	sprintf(buf, ".L%d", ctx->labelno);
	ctx->labelno ++;
	return buf;
}

// Append one word.
void asm_append(asm_ctx_t *ctx, memword_t val) {
	// Check whether we can extend.
	if (!ctx->rawCountIndex || ctx->asmData[ctx->rawCountIndex] > 255 - MEMW_BYTES) {
		// And add a new raw bit otherwise.
		asm_appendraw(ctx, ASM_TYPE_RAW);
		ctx->rawCountIndex = ctx->nAsmData;
		asm_appendraw(ctx, MEMW_BYTES);
	}
	ctx->nWords ++;
#if MEMW_BYTES > 1
	asm_appendnum(ctx, val, MEMW_BYTES);
#else
	printf("ASM %02x\n", val);
	asm_appendraw(ctx, val);
#endif
}

// Append two words.
void asm_append2(asm_ctx_t *ctx, memword2_t val) {
	// Check whether we can extend.
	if (!ctx->rawCountIndex || ctx->asmData[ctx->rawCountIndex] > 255 - MEMW_BYTES * 2) {
		// And add a new raw bit otherwise.
		asm_appendraw(ctx, ASM_TYPE_RAW);
		ctx->rawCountIndex = ctx->nAsmData;
		asm_appendraw(ctx, MEMW_BYTES * 2);
	}
	ctx->nWords += 2;
	asm_appendnum(ctx, val, MEMW_BYTES * 2);
}

// Append three words.
void asm_append3(asm_ctx_t *ctx, memword4_t val) {
	// Check whether we can extend.
	if (!ctx->rawCountIndex || ctx->asmData[ctx->rawCountIndex] > 255 - MEMW_BYTES * 3) {
		// And add a new raw bit otherwise.
		asm_appendraw(ctx, ASM_TYPE_RAW);
		ctx->rawCountIndex = ctx->nAsmData;
		asm_appendraw(ctx, MEMW_BYTES * 3);
	}
	ctx->nWords += 3;
	asm_appendnum(ctx, val, MEMW_BYTES * 3);
}

// Append four words.
void asm_append4(asm_ctx_t *ctx, memword4_t val) {
	// Check whether we can extend.
	if (!ctx->rawCountIndex || ctx->asmData[ctx->rawCountIndex] > 255 - MEMW_BYTES * 4) {
		// And add a new raw bit otherwise.
		asm_appendraw(ctx, ASM_TYPE_RAW);
		ctx->rawCountIndex = ctx->nAsmData;
		asm_appendraw(ctx, MEMW_BYTES * 4);
	}
	ctx->nWords += 4;
	asm_appendnum(ctx, val, MEMW_BYTES * 4);
}


// Reference a label where the output is address_t.
void asm_label_ref(asm_ctx_t *ctx, label_t label, label_res_t res, address_t offset) {
	ctx->rawCountIndex = 0;
	size_t index = asm_add_or_get_label(ctx, label);
	asm_shit = false;
	asm_appendraw(ctx, ASM_TYPE_LBREF);
	asm_appendnum(ctx, index, MEMW_BYTES);
	asm_appendnum(ctx, (size_t) res, sizeof(size_t));
	asm_shit = true;
	ctx->nWords += ADDR_NUM_MEMWS;
	char *mode = res == LABEL_ABSOLUTE ? "ABS" : "REL";
	printf("REF '%s' (%s)\n", label, mode);
}

// Reference a label where the output is N words.
void asm_label_refl(asm_ctx_t *ctx, label_t label, label_res_t res, address_t offset, uint8_t numWords) {
	ctx->rawCountIndex = 0;
	size_t index = asm_add_or_get_label(ctx, label);
	asm_shit = false;
	asm_appendraw(ctx, ASM_TYPE_LBREF);
	asm_appendnum(ctx, index, MEMW_BYTES);
	asm_appendraw(ctx, res);
	asm_appendnum(ctx, offset, sizeof(address_t));
	asm_shit = true;
	ctx->nWords += numWords;
	char *mode = res == LABEL_ABSOLUTE ? "ABS" : "REL";
	printf("REF '%s' (%s)\n", label, mode);
}

/* ================ Hierarchy ================= */
// Utilities for dealing with the hierarchy.

// Open a new scope.
void asm_push_scope(asm_ctx_t *ctx) {
	printf("PUSH SCOPE\n");
	// Create some stuff.
	asm_scope_t *current = malloc(sizeof(asm_scope_t));
	current->parent = ctx->scope;
	map_create(&current->variables);
	map_create(&current->upper);
	// Copy parent's variables.
	asm_scope_t *parent = current->parent;
	for (size_t i = 0; i < parent->upper.numEntries; i++) {
		asm_var_t *var = (asm_var_t *) parent->upper.values[i];
		map_set(&current->upper, var->ident, var);
	}
	for (size_t i = 0; i < parent->variables.numEntries; i++) {
		asm_var_t *var = (asm_var_t *) parent->variables.values[i];
		map_set(&current->upper, var->ident, var);
	}
	for (size_t i = 0; i < current->upper.numEntries; i++) {
		asm_var_t *from = (asm_var_t *) current->upper.values[i];
		asm_var_t *to = (asm_var_t *) malloc(sizeof(asm_var_t));
		memcpy(to, from, sizeof(asm_var_t));
		current->upper.values[i] = to;
		gen_update_ptr(ctx, &from->param, &to->param);
	}
	ctx->scope = current;
	ctx->scope->stackSize = ctx->stackSize;
}

// Restore the previous scope.
// Keeps new variable definitions, but moves any variables back to their old positions.
// If explicit is false, no code is generated to do so, only locations updated.
void asm_restore_scope(asm_ctx_t *ctx, bool explicit) {
	printf("RESTORE SCOPE %d\n", explicit);
	// Set the current scope to it's parent.
	asm_scope_t *current = ctx->scope;
	ctx->scope = current->parent;
	// Restore variable locations to that of the parent scope.
	if (explicit) {
		for (size_t i = 0; i < current->upper.numEntries; i++) {
			asm_var_t *to = (asm_var_t *) map_get(&ctx->scope->variables, current->upper.strings[i]);
			if (!to) to = (asm_var_t *) map_get(&ctx->scope->upper, current->upper.strings[i]);
			if (!to) continue;
			asm_var_t *from = (asm_var_t *) current->upper.values[i];
			gen_restore(ctx, &from->param, &to->param);
			gen_update_ptr(ctx, &from->param, &to->param);
		}
	}
	ctx->stackSize = current->stackSize;
	map_delete_with_values(&current->variables);
	map_delete(&current->upper);
	free(current);
}

// Close the current scope.
void asm_pop_scope(asm_ctx_t *ctx) {
	printf("POP SCOPE\n");
	// Set the current scope to it's parent.
	asm_scope_t *current = ctx->scope;
	ctx->scope = current->parent;
	// Merge variable locations into parent scope.
	for (size_t i = 0; i < current->upper.numEntries; i++) {
		asm_var_t *to = (asm_var_t *) map_get(&ctx->scope->variables, current->upper.strings[i]);
		if (!to) to = (asm_var_t *) map_get(&ctx->scope->upper, current->upper.strings[i]);
		if (!to) continue;
		asm_var_t *from = (asm_var_t *) current->upper.values[i];
		to->param = from->param;
		gen_update_ptr(ctx, &from->param, &to->param);
	}
	map_delete_with_values(&current->variables);
	map_delete_with_values(&current->upper);
	free(current);
}

// Open a new scope for the preprocessor.
void asm_preproc_push_scope(asm_ctx_t *ctx) {
	printf("PUSH PREPROC SCOPE\n");
	// Create some stuff.
	asm_scope_t *current = malloc(sizeof(asm_scope_t));
	current->parent = ctx->scope;
	map_create(&current->variables);
	map_create(&current->upper);
	// Copy parent's variables.
	asm_scope_t *parent = current->parent;
	for (size_t i = 0; i < parent->upper.numEntries; i++) {
		asm_preproc_t *var = (asm_preproc_t *) parent->upper.values[i];
		map_set(&current->upper, var->ident, var);
	}
	for (size_t i = 0; i < parent->variables.numEntries; i++) {
		asm_preproc_t *var = (asm_preproc_t *) parent->variables.values[i];
		map_set(&current->upper, var->ident, var);
	}
	current->isLoop = current->parent->isLoop;
	ctx->scope = current;
}

// Open a new scope for the preprocessor, given the context of a loop.
void asm_preproc_loop_scope(asm_ctx_t *ctx) {
	printf("LOOP PREPROC SCOPE\n");
	// Create some stuff.
	asm_scope_t *current = malloc(sizeof(asm_scope_t));
	current->parent = ctx->scope;
	map_create(&current->variables);
	map_create(&current->upper);
	// Copy parent's variables.
	asm_scope_t *parent = current->parent;
	for (size_t i = 0; i < parent->upper.numEntries; i++) {
		asm_preproc_t *var = (asm_preproc_t *) parent->upper.values[i];
		map_set(&current->upper, var->ident, var);
	}
	for (size_t i = 0; i < parent->variables.numEntries; i++) {
		asm_preproc_t *var = (asm_preproc_t *) parent->variables.values[i];
		map_set(&current->upper, var->ident, var);
	}
	current->isLoop = true;
	ctx->scope = current;
}

// Close the current scope for the preprocessor.
void asm_preproc_pop_scope(asm_ctx_t *ctx) {
	printf("POP PREPROC SCOPE\n");
	// Set the current scope to it's parent.
	asm_scope_t *current = ctx->scope;
	ctx->scope = current->parent;
	if (current->isLoop) {
		for (size_t i = 0; i < current->variables.numEntries; i++) {
			asm_preproc_t *var = (asm_preproc_t *) current->variables.values[i];
			if (var->expr) var->expr->value = 0;
		}
		for (size_t i = 0; i < current->upper.numEntries; i++) {
			asm_preproc_t *var = (asm_preproc_t *) current->upper.values[i];
			if (var->expr) var->expr->value = 0;
		}
	}
	map_delete_with_values(&current->variables);
	map_delete(&current->upper);
	free(current);
}

/* ============== Pre-processing ============== */
// Pre-processing pass before writing ASM.

// Finds or makes the preprocessor data for ident.
asm_preproc_t *asm_preproc_find(asm_ctx_t *ctx, char *ident) {
	asm_preproc_t *opt = (asm_preproc_t *) map_get(&ctx->scope->variables, ident);
	if (opt) return opt;
	opt = (asm_preproc_t *) map_get(&ctx->scope->upper, ident);
	if (opt) return opt;
	opt = (asm_preproc_t *) malloc(sizeof(asm_preproc_t));
	opt->ident = ident;
	opt->expr = NULL;
	map_set(&ctx->scope->variables, ident, opt);
	return opt;
}

// Preprocess the data for an expression.
void asm_preproc_expr(parser_ctx_t *parser_ctx, expression_t *expr) {
	asm_ctx_t *ctx = parser_ctx->asm_ctx;
	switch (expr->type) {
		case (EXPR_TYPE_IDENT): {
			asm_preproc_t *usage = asm_preproc_find(ctx, expr->ident);
			// Mark previous unused.
			if (usage->expr) usage->expr->value = 0;
			// Mark new as used.
			expr->value = 1;
			usage->expr = expr;
			} break;
		case (EXPR_TYPE_ICONST):
			break;
		case (EXPR_TYPE_INVOKE):
			printf("TODO: PREPROC EXPR_TYPE_INVOKE\n");
			break;
		case (EXPR_TYPE_ASSIGN):
			asm_preproc_expr(parser_ctx, expr->expr);
			asm_preproc_expr(parser_ctx, expr->expr1);
			break;
		case (EXPR_TYPE_MATH2):
			asm_preproc_expr(parser_ctx, expr->expr);
			asm_preproc_expr(parser_ctx, expr->expr1);
			break;
		case (EXPR_TYPE_MATH1):
			asm_preproc_expr(parser_ctx, expr->expr);
			break;
	}
}

// Preprocess the data for a statement.
// Returns true if implicit return is required.
bool asm_preproc_statmt(parser_ctx_t *parser_ctx, statement_t *statmt) {
	asm_ctx_t *ctx = parser_ctx->asm_ctx;
	bool implicit = false;
	switch (statmt->type) {
		case (STATMT_TYPE_NOP):
			// Nothing happens.
			return true;
		case (STATMT_TYPE_EXPR):
			// Just an expression.
			asm_preproc_expr(parser_ctx, statmt->expr);
			return true;
		case (STATMT_TYPE_RET):
			// Return expression.
			asm_preproc_expr(parser_ctx, statmt->expr);
			return false;
		case (STATMT_TYPE_VAR):
			// Variable declaration.
			for (int i = 0; i < statmt->decls->num; i++) {
				vardecl_t *decl = &statmt->decls->vars[i];
				asm_preproc_t *var = (asm_preproc_t *) malloc(sizeof(asm_preproc_t));
				var->expr = NULL;
				var->ident = decl->ident.ident;
				if (decl->expr) asm_preproc_expr(parser_ctx, decl->expr);
			}
			return true;
		case (STATMT_TYPE_IF): {
			// If statement.
			asm_preproc_expr(parser_ctx, statmt->expr);
			
			asm_preproc_push_scope(ctx);
			asm_preproc_statmt(parser_ctx, statmt->statement);
			asm_preproc_pop_scope(ctx);
			
			asm_preproc_push_scope(ctx);
			asm_preproc_statmt(parser_ctx, statmt->statement1);
			asm_preproc_pop_scope(ctx);
			} break;
		case (STATMT_TYPE_WHILE): {
			// While statement.
			asm_preproc_expr(parser_ctx, statmt->expr);
			asm_preproc_loop_scope(ctx);
			asm_preproc_statmt(parser_ctx, statmt->statement);
			asm_preproc_pop_scope(ctx);
			implicit = true;
			} break;
		case (STATMT_TYPE_FOR): {
			// For statement.
			asm_preproc_push_scope(ctx);
			asm_preproc_statmt(parser_ctx, statmt->statement);
			asm_preproc_loop_scope(ctx);
			asm_preproc_expr(parser_ctx, statmt->expr);
			asm_preproc_statmt(parser_ctx, statmt->statement1);
			asm_preproc_expr(parser_ctx, statmt->expr1);
			asm_preproc_pop_scope(ctx);
			asm_preproc_pop_scope(ctx);
			} break;
		case (STATMT_TYPE_MULTI):
			// Many statements.
			implicit = true;
			asm_preproc_push_scope(ctx);
			for (int i = 0; i < statmt->statements->num && implicit; i++) {
				implicit = asm_preproc_statmt(parser_ctx, &statmt->statements->statements[i]);
			}
			asm_preproc_pop_scope(ctx);
			break;
		default:
			return true;
	}
	return implicit;
}

// Preprocess the data for a function.
void asm_preproc_function(parser_ctx_t *parser_ctx, funcdef_t *func) {
	asm_ctx_t *ctx = parser_ctx->asm_ctx;
	// Iterate over all statements.
	asm_preproc_push_scope(ctx);
	for (int i = 0; i < func->statements->num; i++) {
		asm_preproc_statmt(parser_ctx, &func->statements->statements[i]);
	}
	asm_preproc_pop_scope(ctx);
}

/* ================= Writing ================== */
// Writing ASM for the given data.

// Finds the parameter with the given name, if any.
param_spec_t *asm_find_var(asm_ctx_t *ctx, char *ident) {
	asm_var_t *var = map_get(&ctx->scope->variables, ident);
	if (var) return &var->param;
	var = map_get(&ctx->scope->upper, ident);
	if (var) return &var->param;
	return NULL;
}

// Replaces the parameter with the one for the appropriate variable.
param_spec_t *asm_filter_param(asm_ctx_t *ctx, param_spec_t *param) {
	if (param->type == LABEL) {
		param_spec_t *opt = asm_find_var(ctx, param->ptr.label);
		if (opt) {
			opt->needs_save = param->needs_save;
			return opt;
		}
	}
	return param;
}

// Convert the given data into ASM for an expression.
void asm_write_expr(parser_ctx_t *parser_ctx, expression_t *expr) {
	asm_ctx_t *ctx = parser_ctx->asm_ctx;
	param_spec_t param = {
		.type = LABEL,
		.type_spec = { .type = NUM_HHU, .size = 1 },
		.ptr = { .label = expr->ident },
		.size = 1,
		.needs_save = false,
		.save_to = NULL
	};
	switch (expr->type) {
		case (EXPR_TYPE_IDENT):
			param = (param_spec_t) {
				.type = LABEL,
				.type_spec = { .type = NUM_HHU, .size = 1 },
				.ptr = { .label = expr->ident },
				.size = 1,
				.needs_save = !expr->value,
				.save_to = NULL
			};
			softstack_push(&ctx->paramStack, param);
			break;
		case (EXPR_TYPE_ICONST):
			param = (param_spec_t) {
				.type = CONSTANT,
				.type_spec = { .type = NUM_HHU, .size = 1 },
				.ptr = { .constant = expr->value },
				.size = 1,
				.needs_save = false,
				.save_to = NULL
			};
			softstack_push(&ctx->paramStack, param);
			break;
		case (EXPR_TYPE_INVOKE):
			printf("TODO: EXPR_TYPE_INVOKE\n");
			break;
		case (EXPR_TYPE_ASSIGN):
			asm_write_expr(parser_ctx, expr->expr);
			asm_write_expr(parser_ctx, expr->expr1);
			// Source.
			param_spec_t *a = asm_filter_param(ctx, softstack_get(&ctx->paramStack, 0));
			softstack_pop(&ctx->paramStack);
			// Destination
			param_spec_t *b = asm_filter_param(ctx, softstack_get(&ctx->paramStack, 0));
			softstack_pop(&ctx->paramStack);
			// Move it.
			gen_mov(ctx, b, a);
			break;
		case (EXPR_TYPE_MATH2):
			asm_write_expr(parser_ctx, expr->expr);
			asm_write_expr(parser_ctx, expr->expr1);
			b = asm_filter_param(ctx, softstack_get(&ctx->paramStack, 0));
			softstack_pop(&ctx->paramStack);
			a = asm_filter_param(ctx, softstack_get(&ctx->paramStack, 0));
			softstack_pop(&ctx->paramStack);
			gen_math2(ctx, a, b, expr->oper, &param);
			softstack_push(&ctx->paramStack, param);
			break;
		case (EXPR_TYPE_MATH1):
			asm_write_expr(parser_ctx, expr->expr);
			a = asm_filter_param(ctx, softstack_get(&ctx->paramStack, 0));
			softstack_pop(&ctx->paramStack);
			gen_math1(ctx, a, expr->oper);
			softstack_push(&ctx->paramStack, *a);
			break;
	}
}

// Convert the given data into ASM for a statement.
// Returns true if implicit return is required.
bool asm_write_statmt(parser_ctx_t *parser_ctx, statement_t *statmt) {
	asm_ctx_t *ctx = parser_ctx->asm_ctx;
	bool implicit = false;
	switch (statmt->type) {
		case (STATMT_TYPE_NOP):
			// Nothing happens.
			break;
		case (STATMT_TYPE_EXPR):
			// Just an expression.
			asm_write_expr(parser_ctx, statmt->expr);
			return true;
		case (STATMT_TYPE_RET):
			// Return expression.
			asm_write_expr(parser_ctx, statmt->expr);
			param_spec_t *ret = asm_filter_param(ctx, softstack_get(&ctx->paramStack, 0));
			softstack_pop(&ctx->paramStack);
			gen_method_ret(ctx, ret);
			return false;
		case (STATMT_TYPE_VAR):
			// Variable declaration.
			for (int i = 0; i < statmt->decls->num; i++) {
				vardecl_t *decl = &statmt->decls->vars[i];
				// Lettcude specify type.
				param_spec_t val = {
					.type_spec = decl->type.type_spec,
					.size = decl->type.type_spec.size,
					.needs_save = false,
					.save_to = NULL
				};
				if (decl->expr) {
					// With value.
					asm_write_expr(parser_ctx, decl->expr);
					param_spec_t *src = asm_filter_param(ctx, softstack_get(&ctx->paramStack, 0));
					softstack_pop(&ctx->paramStack);
					gen_var_assign(ctx, src, &val);
				} else {
					// Without value.
					gen_var(ctx, &val.type_spec, &val);
				}
				// Add the variable.
				asm_var_t *var = (asm_var_t *) malloc(sizeof(asm_var_t));
				var->ident = decl->ident.ident;
				var->param = val;
				map_set(&ctx->scope->variables, decl->ident.ident, var);
			}
			break;
		case (STATMT_TYPE_IF): {
			// If statement.
			asm_write_expr(parser_ctx, statmt->expr);
			
			param_spec_t *cond = asm_filter_param(ctx, softstack_get(&ctx->paramStack, 0));
			softstack_pop(&ctx->paramStack);
			param_spec_t zero = {
				.type = CONSTANT,
				.type_spec = { .type = NUM_HHU },
				.size = 1,
				.ptr = { .constant = 0 },
				.needs_save = false,
				.save_to = NULL
			};
			
			if (statmt->statement->type == STATMT_TYPE_NOP && statmt->statement1->type == STATMT_TYPE_NOP) {
				// Noth are NOP.
				implicit = true;
			} else if (statmt->statement->type == STATMT_TYPE_NOP) {
				// If is NOP.
				label_t skip = asm_numbered_label(ctx);
				
				gen_branch(ctx, cond, &zero, BRANCH_NOT_EQUAL, skip);
				asm_push_scope(ctx);
				implicit = asm_write_statmt(parser_ctx, statmt->statement1);
				asm_restore_scope(ctx, implicit);
				asm_label(ctx, skip);
				
				free(skip);
				implicit = true;
			} else if (statmt->statement1->type == STATMT_TYPE_NOP) {
				// Else if NOP.
				label_t skip = asm_numbered_label(ctx);
				
				gen_branch(ctx, cond, &zero, BRANCH_EQUAL, skip);
				asm_push_scope(ctx);
				implicit = asm_write_statmt(parser_ctx, statmt->statement);
				asm_restore_scope(ctx, implicit);
				asm_label(ctx, skip);
				
				free(skip);
				implicit = true;
			} else {
				// Neither is NOP.
				label_t skip;
				label_t to_else = asm_numbered_label(ctx);
				
				gen_branch(ctx, cond, &zero, BRANCH_EQUAL, to_else);
				asm_push_scope(ctx);
				bool implicit_a = asm_write_statmt(parser_ctx, statmt->statement);
				asm_restore_scope(ctx, implicit_a);
				if (implicit_a) {
					gen_jump(ctx, skip);
					skip = asm_numbered_label(ctx);
				}
				asm_label(ctx, to_else);
				free(to_else);
				asm_push_scope(ctx);
				bool implicit_b = asm_write_statmt(parser_ctx, statmt->statement1);
				asm_restore_scope(ctx, implicit_b);
				if (implicit_a) {
					asm_label(ctx, skip);
					free(skip);
				}
				implicit = implicit_a | implicit_b;
			}
			} break;
		case (STATMT_TYPE_WHILE): {
			// While statement.
			label_t pre = asm_numbered_label(ctx);
			label_t post = asm_numbered_label(ctx);
			
			asm_label(ctx, pre);
			asm_write_expr(parser_ctx, statmt->expr);
			param_spec_t *cond = asm_filter_param(ctx, softstack_get(&ctx->paramStack, 0));
			softstack_pop(&ctx->paramStack);
			param_spec_t zero = {
				.type = CONSTANT,
				.type_spec = { .type = NUM_HHU },
				.size = 1,
				.ptr = { .constant = 0 },
				.needs_save = false,
				.save_to = NULL
			};
			
			gen_branch(ctx, cond, &zero, BRANCH_EQUAL, post);
			asm_write_statmt(parser_ctx, statmt->statement);
			gen_jump(ctx, pre);
			asm_label(ctx, post);
			
			free(pre);
			free(post);
			implicit = true;
			} break;
		case (STATMT_TYPE_FOR): {
			// For statement.
			label_t pre = asm_numbered_label(ctx);
			label_t post = asm_numbered_label(ctx);
			asm_push_scope(ctx);
			// Setup.
			asm_write_statmt(parser_ctx, statmt->statement);
			// Condition.
			asm_label(ctx, pre);
			asm_push_scope(ctx);
			asm_write_expr(parser_ctx, statmt->expr);
			param_spec_t *cond = asm_filter_param(ctx, softstack_get(&ctx->paramStack, 0));
			softstack_pop(&ctx->paramStack);
			param_spec_t zero = {
				.type = CONSTANT,
				.type_spec = { .type = NUM_HHU },
				.size = 1,
				.ptr = { .constant = 0 },
				.needs_save = false,
				.save_to = NULL
			};
			gen_branch(ctx, cond, &zero, BRANCH_EQUAL, post);
			// Loop code.
			asm_write_statmt(parser_ctx, statmt->statement1);
			// Increment.
			asm_write_expr(parser_ctx, statmt->expr1);
			asm_restore_scope(ctx, true);
			// Re-loopening.
			gen_jump(ctx, pre);
			asm_label(ctx, post);
			asm_pop_scope(ctx);
			} break;
		case (STATMT_TYPE_MULTI):
			// Many statements.
			asm_push_scope(ctx);
			for (int i = 0; i < statmt->statements->num; i++) {
				implicit = asm_write_statmt(parser_ctx, &statmt->statements->statements[i]);
			}
			asm_pop_scope(ctx);
			break;
		default:
			return true;
	}
	return implicit;
}

// Convert the given data into ASM for a function.
void asm_write_function(parser_ctx_t *parser_ctx, funcdef_t *func) {
	// Setup.
	asm_ctx_t *ctx = parser_ctx->asm_ctx;
	bool do_implicit_ret = true;
	softstack_create(&ctx->paramStack);
	// Method entry.
	asm_label(ctx, func->ident.ident);
	param_spec_t *param_ptr[func->numParams];
	for (int i = 0; i < func->numParams; i++) {
		asm_var_t *var = (asm_var_t *) malloc(sizeof(asm_var_t));
		*var = (asm_var_t) {
			.ident = func->params[i].ident.ident,
			.param = {
				.type_spec = func->params[i].type.type_spec,
				.size = func->params[i].type.type_spec.size,
				.needs_save = true,
				.save_to = NULL
			}
		};
		param_ptr[i] = &var->param;
		map_set(&ctx->scope->variables, func->params[i].ident.ident, var);
	}
	gen_method_entry(ctx, func, param_ptr, func->numParams);
	// Processing.
	for (int i = 0; i < func->statements->num; i++) {
		do_implicit_ret = asm_write_statmt(parser_ctx, &func->statements->statements[i]);
	}
	// Method exit.
	if (do_implicit_ret) {
		param_spec_t ret_spec = {
			.type = CONSTANT,
			.type_spec = { .type = NUM_HHU },
			.size = 1,
			.ptr = { .constant = 0 },
			.needs_save = false,
			.save_to = NULL
		};
		gen_method_ret(ctx, &ret_spec);
	}
	// Cleanup.
	softstack_delete(&ctx->paramStack);
}

