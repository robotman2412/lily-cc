
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_compile_stmt.h"

#include "c_ir.h"
#include "lilycc_malloc.h"
#include "vec.h"



// Compile a statement.
// All functions return NULL on failure; this one may succeed with any number of definition statements.
// Therefor, its return type is instead a vector of `cir_stmt_t *`.
vec_cir_stmt_t *c_compile2_stmt(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_t const *stmt) {
    cir_stmt_t *res;
    switch (stmt->tag) {
        case C_AST_TAG_STMT_FOR: res = c_compile2_stmt_for(cc, scope, stmt->stmt_for);
        case C_AST_TAG_STMTS: res = c_compile2_stmt_stmts(cc, scope, stmt->stmt_stmts);
        case C_AST_TAG_STMT_WHILE: res = c_compile2_stmt_while(cc, scope, stmt->stmt_while);
        case C_AST_TAG_STMT_IF: res = c_compile2_stmt_if(cc, scope, stmt->stmt_if);
        case C_AST_TAG_STMT_SWITCH: res = c_compile2_stmt_switch(cc, scope, stmt->stmt_switch);
        case C_AST_TAG_STMT_CASE: res = c_compile2_stmt_case(cc, scope, stmt->stmt_case);
        case C_AST_TAG_STMT_LABEL: res = c_compile2_stmt_label(cc, scope, stmt->stmt_label);
        case C_AST_TAG_STMT_RETURN: res = c_compile2_stmt_return(cc, scope, stmt->stmt_return);
        case C_AST_TAG_STMT_GOTO: res = c_compile2_stmt_goto(cc, scope, stmt->stmt_goto);
        case C_AST_TAG_STMT_EXPR: res = c_compile2_stmt_expr(cc, scope, stmt->stmt_expr);
        case C_AST_TAG_STMT_DEF: return c_compile2_stmt_def(cc, scope, stmt->stmt_def);
        case C_AST_TAG_STMT_GARBAGE: return NULL;
        default: abort();
    }
    if (!res) {
        return NULL;
    }
    vec_cir_stmt_t *vec = lilycc_calloc(1, sizeof(vec_cir_stmt_t));
    vec_push(vec, res);
    return vec;
}


// Compile a statements block.
cir_stmt_t *c_compile2_stmt_stmts(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_list_t const *stmt) {
    fprintf(stderr, "TODO: c_compile2_stmt_stmts\n");
    abort();
}

// Compile a for loop statement.
cir_stmt_t *c_compile2_stmt_for(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_for_t const *stmt) {
    fprintf(stderr, "TODO: c_compile2_stmt_for\n");
    abort();
}

// Compile a while or do...while loop statement.
cir_stmt_t *c_compile2_stmt_while(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_while_t const *stmt) {
    fprintf(stderr, "TODO: c_compile2_stmt_while\n");
    abort();
}

// Compile an if...else statement.
cir_stmt_t *c_compile2_stmt_if(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_if_t const *stmt) {
    fprintf(stderr, "TODO: c_compile2_stmt_if\n");
    abort();
}

// Compile a switch statement.
cir_stmt_t *c_compile2_stmt_switch(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_switch_t const *stmt) {
    fprintf(stderr, "TODO: c_compile2_stmt_switch\n");
    abort();
}

// Compile a case statement.
cir_stmt_t *c_compile2_stmt_case(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_case_t const *stmt) {
    fprintf(stderr, "TODO: c_compile2_stmt_case\n");
    abort();
}

// Compile a label statement.
cir_stmt_t *c_compile2_stmt_label(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_label_t const *stmt) {
    fprintf(stderr, "TODO: c_compile2_stmt_label\n");
    abort();
}

// Compile a return statement.
cir_stmt_t *c_compile2_stmt_return(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_return_t const *stmt) {
    fprintf(stderr, "TODO: c_compile2_stmt_return\n");
    abort();
}

// Compile a goto statement.
cir_stmt_t *c_compile2_stmt_goto(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_goto_t const *stmt) {
    fprintf(stderr, "TODO: c_compile2_stmt_goto\n");
    abort();
}

// Compile an expression in a statement.
cir_stmt_t *c_compile2_stmt_expr(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_list_t const *stmt) {
    fprintf(stderr, "TODO: c_compile2_stmt_expr\n");
    abort();
}

// Compile a declaration statement.
// All functions return NULL on failure; this one may succeed with any number of definition statements.
// Therefor, its return type is instead a vector of `cir_stmt_t *`.
vec_cir_stmt_t *c_compile2_stmt_def(c_compiler_t *cc, cir_scope_t *scope, c_ast_def_t const *stmt) {
    fprintf(stderr, "TODO: c_compile2_stmt_def\n");
    abort();
}
