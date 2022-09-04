
#ifndef GEN_PREPROC_H
#define GEN_PREPROC_H

struct preproc_data;

typedef struct preproc_data preproc_data_t;

#include "gen.h"

struct preproc_data {
	// All variables declared in this scope and their default memory location.
	// Maps char* to gen_var_t*.
	// Duplicate the value before modification.
	map_t           *vars;
	// The number of children.
	size_t           n_children;
	// The children.
	preproc_data_t **children;
	// Extra data on an architecture basis.
	PREPROC_EXTRAS
};

// Preprocess a function.
// Determines recursive nature, number of variables per scope and number of intermidiaries.
void gen_preproc_function   (asm_ctx_t *ctx, funcdef_t      *funcdef);
// Preprocess a statement.
// Returns true if an explicit return occurred.
bool gen_preproc_stmt       (asm_ctx_t *ctx, preproc_data_t *parent, void      *stmt, bool       is_stmts);
// Preprocess an expression.
void gen_preproc_expression (asm_ctx_t *ctx, preproc_data_t *parent, expr_t    *expr);
// Preprocess a list of expressions.
void gen_preproc_expressions(asm_ctx_t *ctx, preproc_data_t *parent, exprs_t    *expr);

#endif //GEN_PREPROC_H
