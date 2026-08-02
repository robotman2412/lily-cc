
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "c_ast.h"
#include "c_compiler.h"
#include "c_ir.h"



// Compile a statement.
cir_stmt_t *c_compile2_stmt(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_t const *stmt);

// Compile a statements block.
cir_stmt_t *c_compile2_stmt_stmts(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_list_t const *stmt);
// Compile a for loop statement.
cir_stmt_t *c_compile2_stmt_for(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_for_t const *stmt);
// Compile a while or do...while loop statement.
cir_stmt_t *c_compile2_stmt_while(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_while_t const *stmt);
// Compile an if...else statement.
cir_stmt_t *c_compile2_stmt_if(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_if_t const *stmt);
// Compile a switch statement.
cir_stmt_t *c_compile2_stmt_switch(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_switch_t const *stmt);
// Compile a case statement.
cir_stmt_t *c_compile2_stmt_case(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_case_t const *stmt);
// Compile a label statement.
cir_stmt_t *c_compile2_stmt_label(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_label_t const *stmt);
// Compile a return statement.
cir_stmt_t *c_compile2_stmt_return(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_return_t const *stmt);
// Compile a goto statement.
cir_stmt_t *c_compile2_stmt_goto(c_compiler_t *cc, cir_scope_t *scope, c_ast_stmt_goto_t const *stmt);
// Compile an expression in a statement.
cir_stmt_t *c_compile2_stmt_expr(c_compiler_t *cc, cir_scope_t *scope, c_ast_expr_list_t const *stmt);
// Compile a declaration statement.
cir_stmt_t *c_compile2_stmt_def(c_compiler_t *cc, cir_scope_t *scope, c_ast_def_t const *stmt);
