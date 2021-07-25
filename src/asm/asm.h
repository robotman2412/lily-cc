
#ifndef ASM_H
#define ASM_H

struct asm_ctx;
struct asm_var;
struct asm_data;
struct asm_scope;
struct asm_preproc;

typedef struct asm_ctx asm_ctx_t;
typedef struct asm_var asm_var_t;
typedef struct asm_data asm_data_t;
typedef struct asm_scope asm_scope_t;
typedef struct asm_preproc asm_preproc_t;

#include <config.h>
#include "types.h"
#include <parser.h>
#include <gen.h>
#include <softstack.h>
#include <strmap.h>

struct asm_ctx {
	/* ====== Data insertion =======  */
	// Data to be put in .bss section.
	map_t bssLabels;
	// Data to be put in .data section.
	map_t dataLabels;
	/* ==== Assembly generation ====  */
	// Number of next numbered label.
	int labelno;
	// Number of labels defined in this ASM.
	int nLabels;
	// Capacity for labels defined in this ASM.
	int cLabels;
	// Labels defined at this point.
	// Labels will reference the first address in this bit of code.
	char **labels;
	// Number of words worth of ASM appended.
	address_t nWords;
	// Words appended.
	// Only used after postprocessing.
	memword_t *words;
	// Number of ASM bytes appended.
	size_t nAsmData;
	// Capacity for ASM bytes.
	size_t cAsmData;
	// ASM data appended.
	uint8_t *asmData;
	// Used to increase the count of raw bytes in a bit of raw data.
	// This count is always one byte long.
	size_t rawCountIndex;
	// Whether we are currently post-processing.
	char isPostProc;
	// Parameters and variables.
	softstack_t paramStack;
	// The current scope.
	asm_scope_t *scope;
	/* ====== Machine states =======  */
	// The number of times in this method that the stack has been pushed to.
	address_t stackSize;
	// Custon context for the generator.
	gen_ctx_t gen_ctx;
};

struct asm_var {
	// Location of the variable.
	param_spec_t param;
	// Name of the variable.
	char *ident;
};

struct asm_data {
	// Label name.
	label_t label;
	// Size in words.
	address_t size;
	// Value for .data section.
	memword_t *data;
};

struct asm_scope {
	// New variables defined in this scope.
	map_t variables;
	// Copies of variables from upper scopes.
	map_t upper;
	// Parent scope.
	asm_scope_t *parent;
	// The number of times in this method that the stack has been pushed to.
	address_t stackSize;
	// Whether or not the scope is in a loop.
	bool isLoop;
};

struct asm_preproc {
	// Expression of last usage.
	expression_t *expr;
	// Name of the variable.
	char *ident;
};

/* ================ Utilities ================= */
// Utility functions and definitions.

// Function pointer for label reference resolution.
// This function will invoke asm_append later on.
typedef enum label_res label_res_t;

enum label_res {
	LABEL_ABSOLUTE,
	LABEL_RELATIVE
};

/* ================== Setup =================== */
// Setup and post processing.

// Initialise the ASM context for code generation.
void asm_init(asm_ctx_t *ctx);
// Post-process ASM to make binary.
void asm_postproc(asm_ctx_t *ctx);

/* ================ Appending ================= */
// Appending ASM data.

// Create a label that will reside in bss, one word in size.
void asm_bss_label(asm_ctx_t *ctx, label_t label);
// Create a label that will reside in bss, size words in size.
void asm_bss_label_n(asm_ctx_t *ctx, label_t label, address_t size);
// Create a label that will reside in data, one word in size.
void asm_data_label(asm_ctx_t *ctx, label_t label, memword_t value);
// Create a label that will reside in data, size words in size.
// Will not copy provided data.
void asm_data_label_n(asm_ctx_t *ctx, label_t label, address_t size, memword_t *value);
// Append a label definition.
// Will duplicate the given string.
void asm_label(asm_ctx_t *ctx, label_t label);
// Create a numbered label.
label_t asm_numbered_label(asm_ctx_t *ctx);

// Append one word.
void asm_append(asm_ctx_t *ctx, memword_t val);
// Append two words.
void asm_append2(asm_ctx_t *ctx, memword2_t val);
// Append three words.
void asm_append3(asm_ctx_t *ctx, memword4_t val);
// Append four words.
void asm_append4(asm_ctx_t *ctx, memword4_t val);

// Reference a label where the output is address_t.
// Will duplicate the given string.
void asm_label_ref(asm_ctx_t *ctx, label_t label, label_res_t res, address_t offset);
// Reference a label where the output is N words.
// Will duplicate the given string.
void asm_label_refl(asm_ctx_t *ctx, label_t label, label_res_t res, address_t offset, uint8_t numWords);

/* ================ Hierarchy ================= */
// Utilities for dealing with the hierarchy.

// Open a new scope.
void asm_push_scope(asm_ctx_t *ctx);
// Restore the previous scope.
// Keeps new variable definitions, but moves any variables back to their old positions.
// If explicit is false, no code is generated to do so, only locations updated.
void asm_restore_scope(asm_ctx_t *ctx, bool explicit);
// Close the current scope.
void asm_pop_scope(asm_ctx_t *ctx);
// Open a new scope for the preprocessor.
void asm_preproc_push_scope(asm_ctx_t *ctx);
// Open a new scope for the preprocessor, given the context of a loop.
void asm_preproc_loop_scope(asm_ctx_t *ctx);
// Close the current scope for the preprocessor.
void asm_preproc_pop_scope(asm_ctx_t *ctx);

/* ============== Pre-processing ============== */
// Pre-processing pass before writing ASM.
// Things like finding the last usages of variables.

// Finds or makes the preprocessor data for ident.
asm_preproc_t *asm_preproc_find(asm_ctx_t *ctx, char *ident);
// Preprocess the data for an expression.
void asm_preproc_expr(parser_ctx_t *parser_ctx, expression_t *expr);
// Preprocess the data for a statement.
// Returns true if implicit return is required.
bool asm_preproc_statmt(parser_ctx_t *parser_ctx, statement_t *statmt);
// Preprocess the data for a function.
void asm_preproc_function(parser_ctx_t *parser_ctx, funcdef_t *func);

/* ================= Writing ================== */
// Writing ASM for the given data.

// Finds the variable with the given name, if any.
param_spec_t *asm_find_var(asm_ctx_t *ctx, char *ident);
// Replaces the parameter with the one for the appropriate variable.
param_spec_t *asm_filter_param(asm_ctx_t *ctx, param_spec_t *param);
// Convert the given data into ASM for an expression.
void asm_write_expr(parser_ctx_t *parser_ctx, expression_t *expr);
// Convert the given data into ASM for a statement.
// Returns true if implicit return is required.
bool asm_write_statmt(parser_ctx_t *parser_ctx, statement_t *statmt);
// Convert the given data into ASM for a function.
void asm_write_function(parser_ctx_t *parser_ctx, funcdef_t *func);

#endif // ASM_H
