
#include "gen_preproc.h"
#include "malloc.h"

static inline void pre_stmt_push(asm_ctx_t *ctx, preproc_data_t **parent, stmt_t *stmt) {
    // Make some new preprocessing data.
    stmt->preproc             = malloc(sizeof(preproc_data_t));
    stmt->preproc->n_children = 0;
    stmt->preproc->children   = NULL;
    stmt->preproc->n_temp     = 0;
    stmt->preproc->vars       = malloc(sizeof(map_t));
    map_create(stmt->preproc->vars);
    // Update the parent.
    (*parent)->n_children ++;
    (*parent)->children = realloc((*parent)->children, (*parent)->n_children * sizeof(preproc_data_t *));
    (*parent)->children[(*parent)->n_children - 1] = stmt->preproc;
    // Swappening.
    *parent = stmt->preproc;
}

// Preprocess a function.
// Determines recursive nature, number of variables per scope and number of intermidiaries.
void gen_preproc_function(asm_ctx_t *ctx, funcdef_t *funcdef) {
    DEBUG_PRE("Preprocessing '%s'\n", funcdef->ident.strval);
    funcdef->preproc             = malloc(sizeof(preproc_data_t));
    funcdef->preproc->n_temp     = 0;
    funcdef->preproc->n_children = 0;
    funcdef->preproc->children   = NULL;
    funcdef->preproc->vars       = malloc(sizeof(map_t));
    map_create(funcdef->preproc->vars);
    gen_preproc_stmt(ctx, funcdef->preproc, funcdef->stmts, true);
    DEBUG_PRE("Preprocessing done\n");
}

// Preprocess a statement.
bool gen_preproc_stmt(asm_ctx_t *ctx, preproc_data_t *parent, void *ptr, bool is_stmts) {
    preproc_data_t *current = parent;
    stmt_t *stmt = ptr;
    if (is_stmts) goto stmts_impl;
    if (stmt->type == STMT_TYPE_MULTI) {
        // Pointer perplexing.
        pre_stmt_push(ctx, &current, stmt);
        ptr = stmt->stmts;
        stmts_t *stmts;
        // Epic arrays.
        stmts_impl:
        stmts = ptr;
        for (size_t i = 0; i < stmts->num; i++) {
            // Gen them one by one.
            bool explicit_ret = gen_preproc_stmt(ctx, current, &stmts->arr[i], false);
            // We'll completely ignore unreachable code.
            if (explicit_ret) return true;
        }
        if (current->n_temp > parent->n_temp) {
            parent->n_temp = current->n_temp;
        }
        return false;
    }
    // One of the other statement types.
    switch (stmt->type) {
        case STMT_TYPE_IF: {
            // An if statement.
            size_t num = gen_preproc_expression(ctx, current, stmt->expr);
            if (num > current->n_temp) {
                current->n_temp = num;
            }
            bool e_if   = gen_preproc_stmt(ctx, parent, stmt->code_true,  false);
            bool e_else = stmt->code_false && gen_preproc_stmt(ctx, parent, stmt->code_false, false);
            return e_if && e_else;
        } break;
        case STMT_TYPE_WHILE: {
            size_t num = gen_preproc_expression(ctx, current, stmt->expr);
            if (num > current->n_temp) {
                current->n_temp = num;
            }
            gen_preproc_stmt(ctx, parent, stmt->code_true, false);
            return false;
        } break;
        case STMT_TYPE_RET: {
            // A return statement.
            size_t num = gen_preproc_expression(ctx, current, stmt->expr);
            if (num > current->n_temp) {
                current->n_temp = num;
            }
            return true;
        } break;
        case STMT_TYPE_VAR: {
            // TODO: Vardecls structure to change later.
            for (size_t i = 0; i < stmt->vars->num; i++) {
                char *label = gen_preproc_var(ctx, current, &stmt->vars->arr[i]);
                DEBUG_PRE("var '%s': %s\n", stmt->vars->arr[i].strval, label);
                char *repl  = map_set(current->vars, stmt->vars->arr[i].strval, label);
            }
        } break;
        case STMT_TYPE_EXPR: {
            // The expression.
            size_t num = gen_preproc_expression(ctx, current, stmt->expr);
            if (num > current->n_temp) {
                current->n_temp = num;
            }
        } break;
        case STMT_TYPE_IASM: {
            // TODO (low priority).
        } break;
    }
    return false;
}

// Preprocess an expression.
size_t gen_preproc_expression(asm_ctx_t *ctx, preproc_data_t *parent, expr_t *expr) {
    
}

