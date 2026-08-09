
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "c_compiler.h"
#include "c_ir.h"
#include "ir_types.h"
#include "vec.h"



// C IR lowering scope.
typedef struct c_lower_scope c_lower_scope_t;
// Return type of lowering expressions.
typedef struct c_lower_expr  c_lower_expr_t;
// Case label definition.
typedef struct c_lower_case  c_lower_case_t;

VEC_TYPE_DEF(vec_c_lower_case_t, c_lower_case_t);



// C IR lowering scope.
struct c_lower_scope {
    c_lower_scope_t    *parent;
    // Labelled statements; `char *` -> `ir_code_t *`.
    // Stored at function scope.
    map_t              *labels;
    // Temporary values; `cir_tmpval_t const *` -> `ir_operand_t *`.
    // Stored at function scope.
    map_t              *tmpvals;
    // Case label collection of nearest switch, if any.
    vec_c_lower_case_t *case_labels;
    // Variables' stack frames; `char *` -> `ir_frame_t *`.
    map_t               vars;
    // Target of `break`.
    ir_code_t          *break_target;
    // Target of `continue`.
    ir_code_t          *continue_target;
};

// Return type of lowering expressions.
struct c_lower_expr {
    ir_code_t   *code;
    ir_operand_t val;
};

// Case label definition.
struct c_lower_case {
    pos_t      pos;
    // Target code block.
    ir_code_t *code;
    // Lower bound/case value.
    ir_const_t lo;
    // Higher bound (inclusive).
    ir_const_t hi;
    // Is `default`.
    bool       is_default;
    // Is ranged (e.g. `case 1...3:`).
    bool       is_ranged;
};



// Lower a C IR function to Lily IR.
ir_func_t *c_lower_func(c_compiler_t *cc, cir_func_t const *func);

// Lower a C IR value expression to Lily IR.
c_lower_expr_t c_lower_expr_value(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_value_t const *value);
// Lower a C IR function call expression to Lily IR.
c_lower_expr_t c_lower_expr_call(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_call_t const *call);
// Lower a C IR cast expression to Lily IR.
c_lower_expr_t c_lower_expr_cast(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_cast_t const *cast);
// Lower a C IR ternary expression to Lily IR.
c_lower_expr_t
    c_lower_expr_ternary(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_ternary_t const *ternary);
// Lower a C IR calculation expression to Lily IR.
c_lower_expr_t c_lower_expr_calc(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_calc_t const *calc);
// Lower a C IR address-of expression to Lily IR.
c_lower_expr_t
    c_lower_expr_addrof(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_addrof_t const *addrof);
// Lower a C IR pointer dereference expression to Lily IR.
c_lower_expr_t c_lower_expr_deref(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_deref_t const *deref);
// Lower a C IR expression list to Lily IR.
c_lower_expr_t c_lower_expr_exprs(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_exprs_t const *exprs);
// Lower a C IR assignment expression to Lily IR.
c_lower_expr_t
    c_lower_expr_assign(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_assign_t const *assign);
// Lower a C IR statement expression to Lily IR.
c_lower_expr_t c_lower_expr_stmt(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_stmt_t const *stmt);
// Lower a C IR expression to Lily IR.
c_lower_expr_t c_lower_expr(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_expr_t const *expr);

// Lower a C IR compound statement to Lily IR.
ir_code_t *c_lower_stmt_stmts(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_stmts_t const *stmts);
// Lower a C IR for loop statement to Lily IR.
ir_code_t *c_lower_stmt_for(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_for_t const *for_loop);
// Lower a C IR while/do-while loop statement to Lily IR.
ir_code_t *c_lower_stmt_while(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_while_t const *while_loop);
// Lower a C IR switch statement to Lily IR.
ir_code_t *
    c_lower_stmt_switch(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_switch_t const *switch_stmt);
// Lower a C IR if/else statement to Lily IR.
ir_code_t *c_lower_stmt_if(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_if_t const *if_stmt);
// Lower a C IR case-labeled statement to Lily IR.
ir_code_t *c_lower_stmt_case(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_case_t const *cir_case);
// Lower a C IR labeled statement to Lily IR.
ir_code_t *c_lower_stmt_label(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_label_t const *label);
// Lower a C IR goto statement to Lily IR.
ir_code_t *c_lower_stmt_goto(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_goto_t const *goto_stmt);
// Lower a C IR break statement to Lily IR.
ir_code_t *c_lower_stmt_break(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_break_t const *break_stmt);
// Lower a C IR return statement to Lily IR.
ir_code_t *
    c_lower_stmt_return(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_return_t const *return_stmt);
// Lower a C IR unit list statement to Lily IR.
ir_code_t *c_lower_stmt_units(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_unit_list_t const *units);
// Lower a C IR statement to Lily IR.
ir_code_t *c_lower_stmt(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_stmt_t const *stmt);
