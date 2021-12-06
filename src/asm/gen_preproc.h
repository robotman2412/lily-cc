
#ifndef GEN_PREPROC_H
#define GEN_PREPROC_H

struct preproc_data;

typedef struct preproc_data preproc_data_t;

#include "gen.h"

struct preproc_data {
    // All variables declared in this scope.
    map_t           *vars;
    // The number of temporary vaiables required in this scope.
    size_t           n_temp;
    // The number of schildren.
    size_t           n_children;
    // The children.
    preproc_data_t **children;
};

// Preprocess a function.
// Determines recursive nature, number of variables per scope and number of intermidiaries.
void   gen_preproc_function  (asm_ctx_t *ctx, funcdef_t      *funcdef);
// Preprocess a statement.
// Returns true if an explicit return occurred.
bool   gen_preproc_stmt      (asm_ctx_t *ctx, preproc_data_t *parent, void      *stmt, bool       is_stmts);
// Preprocess an expression.
// Returns the number of required temporary variables.
size_t gen_preproc_expression(asm_ctx_t *ctx, preproc_data_t *parent, expr_t    *expr);

#endif //GEN_PREPROC_H
