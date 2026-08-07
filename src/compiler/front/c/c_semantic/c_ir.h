
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "c_types.h"
#include "compiler.h"
#include "ir_types.h"
#include "map.h"
#include "unreachable.h"
#include "vec.h"



typedef enum {
    CIR_CALC_ADD,  // +
    CIR_CALC_SUB,  // -
    CIR_CALC_MUL,  // *
    CIR_CALC_DIV,  // /
    CIR_CALC_MOD,  // %
    CIR_CALC_SHL,  // <<
    CIR_CALC_SHR,  // >>
    CIR_CALC_BAND, // &
    CIR_CALC_BOR,  // |
    CIR_CALC_BXOR, // ^

    CIR_CALC_LAND, // &&
    CIR_CALC_LOR,  // ||

    CIR_CALC_EQ, // ==
    CIR_CALC_NE, // !=
    CIR_CALC_LT, // <
    CIR_CALC_LE, // <=
    CIR_CALC_GT, // >
    CIR_CALC_GE, // >=
} cir_calc_op_t;

static inline ir_op2_type_t cir_calc_op_to_ir_op2(cir_calc_op_t op) {
    switch (op) {
        case CIR_CALC_ADD: return IR_OP2_add;
        case CIR_CALC_SUB: return IR_OP2_sub;
        case CIR_CALC_MUL: return IR_OP2_mul;
        case CIR_CALC_DIV: return IR_OP2_div;
        case CIR_CALC_MOD: return IR_OP2_rem;
        case CIR_CALC_SHL: return IR_OP2_shl;
        case CIR_CALC_SHR: return IR_OP2_shr;
        case CIR_CALC_BAND: return IR_OP2_band;
        case CIR_CALC_BOR: return IR_OP2_bor;
        case CIR_CALC_BXOR: return IR_OP2_bxor;
        case CIR_CALC_EQ: return IR_OP2_seq;
        case CIR_CALC_NE: return IR_OP2_sne;
        case CIR_CALC_LT: return IR_OP2_slt;
        case CIR_CALC_LE: return IR_OP2_sle;
        case CIR_CALC_GT: return IR_OP2_sgt;
        case CIR_CALC_GE: return IR_OP2_sge;
        case CIR_CALC_LAND:
        case CIR_CALC_LOR: break;
    }
    UNREACHABLE();
}

// Tag for `cir_value_t`.
typedef enum {
    CIR_VALUE_TMPVAL,
    CIR_VALUE_SCOPE_VAL,
    CIR_VALUE_CONST,
    CIR_VALUE_COMP_CONST,
    CIR_VALUE_COMP_VALUE,
} cir_value_tag_t;

// Tag for `cir_expr_t`.
typedef enum {
    CIR_EXPR_VALUE,
    CIR_EXPR_CALL,
    CIR_EXPR_CAST,
    CIR_EXPR_TERNARY,
    CIR_EXPR_CALC,
    CIR_EXPR_ADDROF,
    CIR_EXPR_DEREF,
    CIR_EXPR_EXPRS,
    CIR_EXPR_ASSIGN,
    CIR_EXPR_STMT,
} cir_expr_tag_t;

// Tag for `cir_stmt_t`.
typedef enum {
    CIR_STMT_STMTS,
    CIR_STMT_FOR,
    CIR_STMT_WHILE,
    CIR_STMT_SWITCH,
    CIR_STMT_IF,
    CIR_STMT_CASE,
    CIR_STMT_LABEL,
    CIR_STMT_GOTO,
    CIR_STMT_BREAK,
    CIR_STMT_RETURN,
    CIR_STMT_EXPR,
    CIR_STMT_UNITS,
    CIR_STMT_NOP,
} cir_stmt_tag_t;

// Tag for `cir_unit_t`.
typedef enum {
    CIR_UNIT_DECL,
    CIR_UNIT_FUNC,
} cir_unit_tag_t;

// Kind of a `cir_scope_t`.
typedef enum {
    // Translation-unit scope. Has no parent.
    CIR_SCOPE_GLOBAL,
    // Function scope.
    // The only scope kind that owns the label namespace.
    CIR_SCOPE_FUNC,
    // Block scope (`{ ... }`).
    CIR_SCOPE_STMTS,
    // `switch` statement scope.
    // Allows declarations, holds case labels and sets the `break`  target.
    CIR_SCOPE_SWITCH,
    // `while` loop scope.
    // Sets the `break` and `continue` targets.
    CIR_SCOPE_WHILE,
    // `for` loop initializer scope.
    // Allows non-conflicting declarations and sets the `break` and `continue` targets.
    CIR_SCOPE_FOR,
} cir_scope_type_t;

// Tag for `cir_scope_val_t`.
typedef enum {
    // Variable declaration.
    CIR_SCOPE_VAL_DECL,
    // Function declaration or definition.
    CIR_SCOPE_VAL_FUNC,
    // Enum constant.
    CIR_SCOPE_VAL_ENUM_CONST,
} cir_scope_val_tag_t;


// A C scope including the regular, tag and optionally label namespaces.
typedef struct cir_scope     cir_scope_t;
// One entry in a scope's value namespace; tagged union of decl, func or enum constant.
typedef struct cir_scope_val cir_scope_val_t;
// A typedef and the position it was declared at.
typedef struct cir_typedef   cir_typedef_t;

// A constant value of primitive type.
typedef struct cir_const      cir_const_t;
// A constant value of compound type.
typedef struct cir_comp_const cir_comp_const_t;
// One virtual memory write for `cir_comp_value_t`.
typedef struct cir_comp_store cir_comp_store_t;
// A non-constant value of compound type.
typedef struct cir_comp_value cir_comp_value_t;
// A C value; tagged union of structs above.
typedef struct cir_value      cir_value_t;

// Common fields of all expression subtypes.
typedef struct cir_expr_common cir_expr_common_t;
// Function call operator.
typedef struct cir_call        cir_call_t;
// A casting expression.
typedef struct cir_cast        cir_cast_t;
// Ternary operator.
typedef struct cir_ternary     cir_ternary_t;
// A two-operand calculation that returns one value.
// The return type depends on operand types and operator.
typedef struct cir_calc        cir_calc_t;
// Address-of operator, may be emitted implicitly.
typedef struct cir_addrof      cir_addrof_t;
// Pointer dereference operator.
typedef struct cir_deref       cir_deref_t;
// A temporary value in a `cir_exprs_t`.
typedef struct cir_tmpval      cir_tmpval_t;
// A comma-separated expression list; evaluates each in order and yields the last value.
typedef struct cir_exprs       cir_exprs_t;
// An assignment of an rvalue to an lvalue.
typedef struct cir_assign      cir_assign_t;
// Any type of expression; tagged union of structs above as well as `cir_value_t`.
typedef struct cir_expr        cir_expr_t;

// A compound statement (a sequence of statements wrapped in `{}`).
typedef struct cir_stmts  cir_stmts_t;
// A for loop.
typedef struct cir_for    cir_for_t;
// A while or do...while loop.
typedef struct cir_while  cir_while_t;
// A switch statement.
typedef struct cir_switch cir_switch_t;
// A case label.
typedef struct cir_case   cir_case_t;
// An if/else statement.
typedef struct cir_if     cir_if_t;
// A labeled statement.
typedef struct cir_label  cir_label_t;
// A goto label statement.
typedef struct cir_goto   cir_goto_t;
// A continue or break statement.
typedef struct cir_break  cir_break_t;
// A return statement.
typedef struct cir_return cir_return_t;
// Any type of statement; tagged union of structs above.
typedef struct cir_stmt   cir_stmt_t;

// A variable declaration with an optional initializer.
typedef struct cir_decl       cir_decl_t;
// A function definition or declaration.
typedef struct cir_func       cir_func_t;
// A declaration/definition unit; tagged union of `cir_decl_t` or `cir_func_t`.
typedef struct cir_unit       cir_unit_t;
// A a sequence of `cir_unit_t`.
typedef struct cir_unit_list  cir_unit_list_t;
// Translation unit; the global scope and a sequence of `cir_unit_t`.
typedef struct cir_trans_unit cir_trans_unit_t;

VEC_TYPE_DEF(vec_cir_comp_store_t, cir_comp_store_t);
VEC_TYPE_DEF(vec_cir_tmpval_t, cir_tmpval_t *);
VEC_TYPE_DEF(vec_cir_expr_t, cir_expr_t *);
VEC_TYPE_DEF(vec_cir_stmt_t, cir_stmt_t *);
VEC_TYPE_DEF(vec_cir_unit_t, cir_unit_t *);



// Common fields of all expression subtypes.
struct cir_expr_common {
    // Source location this was compiled from.
    pos_t    pos;
    // Resolved type.
    c_type_t type;
    // Is an lvalue.
    bool     is_lvalue;
    // Allows address-of operator.
    bool     allow_addrof;
};



// A constant value of primitive type.
struct cir_const {
    // Source location this was compiled from.
    pos_t      pos;
    // C primitive type.
    c_prim_t   prim;
    // Corresponding IR constant.
    ir_const_t iconst;
};

// A constant value of compound type.
struct cir_comp_const {
    // Source location this was compiled from.
    pos_t    pos;
    // Compound type.
    c_type_t type;
    // Compound constant blob; size is implied by the type.
    uint8_t *blob;
};

// One virtual memory write for `cir_comp_value_t`.
struct cir_comp_store {
    // Byte offset in parent value.
    uint64_t    offset;
    // Expression whose value shall be written.
    cir_expr_t *value;
};

// A non-constant value of compound type.
struct cir_comp_value {
    // Source location this was compiled from.
    pos_t                pos;
    // Compound type.
    c_type_t             type;
    // Vector of expressions and store offsets.
    vec_cir_comp_store_t stores;
};

// A C value; tagged union of value variants above.
struct cir_value {
    // Common expression fields.
    cir_expr_common_t common;
    // Active union variant.
    cir_value_tag_t   tag;
    union {
        cir_tmpval_t const    *tmpval;
        cir_scope_val_t const *scope_val;
        cir_const_t           *iconst;
        cir_comp_const_t      *comp_const;
        cir_comp_value_t      *comp_value;
    };
};


// Function call operator.
struct cir_call {
    // Common expression fields.
    cir_expr_common_t common;
    // Function expression.
    cir_expr_t       *func;
    // Function parameters.
    vec_cir_expr_t    args;
};

// A casting expression.
struct cir_cast {
    // Common expression fields.
    cir_expr_common_t common;
    // Expression to be cast.
    cir_expr_t       *value;
};

// Ternary operator.
struct cir_ternary {
    // Common expression fields.
    cir_expr_common_t common;
    // Condition expression.
    cir_expr_t       *cond;
    // Expression if true; may be NULL.
    cir_expr_t       *if_expr;
    // Expression if false.
    cir_expr_t       *else_expr;
};

// A two-operand calculation that returns one value.
// The return type depends on operand types and operator.
// If `is_assign` is set, `lhs` must be an lvalue; the operator is applied to its current value
// and `rhs`, the result is stored back to `lhs`, and that stored value is the expression result.
// This represents compound assignment (`+=`, `-=`, ...) without duplicating the lvalue.
struct cir_calc {
    // Common expression fields.
    cir_expr_common_t common;
    // Calculation operator.
    cir_calc_op_t     op;
    // Left-hand side operand.
    cir_expr_t       *lhs;
    // Right-hand side operand.
    cir_expr_t       *rhs;
};

// A pre/post increment/decrement operation.
struct cir_inc {
    // Common expression fields.
    cir_expr_common_t common;
    // Value to increment/decrement.
    cir_expr_t       *expr;
    // Is pre-increment/pre-decrement.
    bool              is_pre;
    // Numeric value to add.
    int64_t           increment;
};

// Address-of operator, may be emitted implicitly.
struct cir_addrof {
    // Common expression fields.
    cir_expr_common_t common;
    // Value to take the address of.
    cir_expr_t       *expr;
};

// Pointer dereference operator.
struct cir_deref {
    // Common expression fields.
    cir_expr_common_t common;
    // Pointer to dereference.
    cir_expr_t       *expr;
};

// A temporary value in a `cir_exprs_t`.
struct cir_tmpval {
    // Common expression fields.
    cir_expr_common_t common;
    cir_expr_t       *inner;
};

// A comma-separated expression list; evaluates each in order and yields the last value.
// Must contain at least one expression.
struct cir_exprs {
    // Common expression fields.
    cir_expr_common_t common;
    // Temporary values used by sub-expressions.
    vec_cir_tmpval_t  tmpvals;
    // Sub-expressions in evaluation order.
    vec_cir_expr_t    exprs;
};

// An assignment of an rvalue to an lvalue.
struct cir_assign {
    // Common expression fields.
    cir_expr_common_t common;
    // Destination lvalue.
    cir_expr_t       *lhs;
    // Value to assign.
    cir_expr_t       *rhs;
};

// Any type of expression; tagged union of expression variants plus a wrapped value.
struct cir_expr {
    // Common expression fields.
    cir_expr_common_t common;
    // Active union variant.
    cir_expr_tag_t    tag;
    union {
        cir_value_t   *value;
        cir_call_t    *call;
        cir_cast_t    *cast;
        cir_ternary_t *ternary;
        cir_calc_t    *calc;
        cir_addrof_t  *addrof;
        cir_deref_t   *deref;
        cir_exprs_t   *exprs;
        cir_assign_t  *assign;
        cir_stmt_t    *stmt;
    };
};


// A compound statement (a sequence of statements wrapped in `{}`).
struct cir_stmts {
    // Source location this was compiled from.
    pos_t          pos;
    // Nested scope created by this block.
    cir_scope_t   *scope;
    // Statements to run in order.
    vec_cir_stmt_t stmts;
};

// A for loop.
struct cir_for {
    // Source location this was compiled from.
    pos_t        pos;
    // Initializer scope.
    cir_scope_t *scope;
    // Initializer (nullable).
    cir_stmt_t  *init;
    // Loop condition (nullable; absence means an infinite loop).
    cir_expr_t  *cond;
    // Increment expression (nullable).
    cir_expr_t  *inc;
    // Loop body.
    cir_stmt_t  *body;
};

// A while or do...while loop.
struct cir_while {
    // Source location this was compiled from.
    pos_t        pos;
    // Nested scope created by this while statement.
    cir_scope_t *scope;
    // Loop condition.
    cir_expr_t  *cond;
    // Loop body.
    cir_stmt_t  *body;
    // Is of `do...while` form.
    bool         is_do_while;
};

// A switch statement.
struct cir_switch {
    // Source location this was compiled from.
    pos_t        pos;
    // Nested scope created by this switch statement.
    cir_scope_t *scope;
    // Switch value.
    cir_expr_t  *value;
    // Switch body.
    cir_stmt_t  *body;
};

// A case label.
struct cir_case {
    // Source location this was compiled from.
    pos_t       pos;
    // Exact value or lower bound expression; NULL for a default label.
    cir_expr_t *lo;
    // Higher bound expression (optional).
    cir_expr_t *hi;
    // Labeled statement.
    cir_stmt_t *body;
};

// An if/else statement.
struct cir_if {
    // Source location this was compiled from.
    pos_t       pos;
    // Condition expression.
    cir_expr_t *cond;
    // If-true branch body.
    cir_stmt_t *if_body;
    // Else branch body (nullable; absent for plain `if`).
    cir_stmt_t *else_body;
};

// A labeled statement.
struct cir_label {
    // Source location this was compiled from.
    pos_t       pos;
    // Label name.
    char       *name;
    // Labeled statement.
    cir_stmt_t *body;
};

// A goto label statement.
struct cir_goto {
    // Source location this was compiled from.
    pos_t pos;
    // Label name.
    char *label;
};

// A continue or break statement.
struct cir_break {
    // Source location this was compiled from.
    pos_t pos;
    // Is a `continue` statement of the nearest `while` or `for` loop.
    bool  is_continue;
};

// A return statement.
struct cir_return {
    // Source location this was compiled from.
    pos_t       pos;
    // Return value (nullable for `return;`).
    cir_expr_t *value;
};

// Any type of statement; tagged union of the statement variants above.
struct cir_stmt {
    // Source location this was compiled from.
    pos_t          pos;
    // Active union variant.
    cir_stmt_tag_t tag;
    union {
        cir_stmts_t     *stmts;
        cir_for_t       *for_loop;
        cir_while_t     *while_loop;
        cir_switch_t    *switch_stmt;
        cir_if_t        *if_stmt;
        cir_case_t      *case_stmt;
        cir_label_t     *label;
        cir_goto_t      *goto_stmt;
        cir_break_t     *break_stmt;
        cir_return_t    *return_stmt;
        cir_expr_t      *expr;
        cir_unit_list_t *units;
    };
};


// A variable declaration with an optional initializer.
struct cir_decl {
    // Source location this was compiled from.
    pos_t       pos;
    // Variable type.
    c_type_t    type;
    // Variable name.
    char       *name;
    // Initializer (nullable).
    cir_expr_t *init;
};

// A function definition.
struct cir_func {
    // Source location this was compiled from.
    pos_t          pos;
    // Function name.
    char          *name;
    // Function scope created by this block.
    cir_scope_t   *scope;
    // Function type (also encodes parameters and their names).
    c_type_t       type;
    // Function body.
    // Includes copies of the parameter type decls at the start.
    vec_cir_stmt_t body;
};

// Tagged union of a declaration or function definition.
struct cir_unit {
    // Source location this was compiled from.
    pos_t          pos;
    // Active union variant.
    cir_unit_tag_t tag;
    union {
        cir_decl_t *decl;
        cir_func_t *func;
    };
};

// A a sequence of `cir_unit_t`.
struct cir_unit_list {
    // Source location this was compiled from.
    pos_t          pos;
    // Units in source order.
    vec_cir_unit_t units;
};

// Translation unit; the global scope and a sequence of `cir_unit_t`.
struct cir_trans_unit {
    // The global scope.
    cir_scope_t   *scope;
    // Units in source order.
    vec_cir_unit_t units;
};


// One entry in a scope's value namespace; tagged union of decl, func or enum constant.
// The scope owns this wrapper but the inner pointer is non-owning.
struct cir_scope_val {
    // Active union variant.
    cir_scope_val_tag_t tag;
    union {
        pos_t const      *pos;
        cir_decl_t const *decl;
        cir_func_t const *func;
        cir_const_t      *enum_const;
    };
};

// A typedef and the position it was declared at.
struct cir_typedef {
    pos_t    pos;
    c_type_t type;
};

// A C scope holding non-owning references to the values, typedefs and tags declared within,
// plus (on function scope) the labels namespace. Lookups walk the parent chain except for
// labels, which only resolve against the enclosing function scope.
struct cir_scope {
    // Scope kind.
    cir_scope_type_t type;
    // Parent scope, or `NULL` for global scope.
    cir_scope_t     *parent;
    // Values namespace: `char *` -> `cir_scope_val_t *` (owned share).
    map_t            values;
    // Typedefs namespace: `char *` -> `cir_typedef_t *` (owned).
    map_t            typedefs;
    // Tag namespace (struct/union/enum types): `char *` -> `c_comp_type_t *` (owned share).
    map_t            tags;
    // Labels namespace: `char *` -> `cir_label_t *` (non-owning). Only populated on function scope.
    map_t            labels;
    // All goto statements: `cir_goto_t *` (non-owning). Only populated on function scope.
    set_t            gotos;
};



// Construct a `cir_const` node.
cir_const_t *cir_const_create(pos_t pos, c_prim_t prim, ir_const_t iconst);
// Destroy a `cir_const` node.
void         cir_const_delete(cir_const_t *node);

// Construct a `cir_comp_const` node.
cir_comp_const_t *cir_comp_const_create(pos_t pos, c_type_t type, uint8_t *blob);
// Destroy a `cir_comp_const` node and any owned children.
void              cir_comp_const_delete(cir_comp_const_t *node);

// Construct a `cir_comp_value` node.
cir_comp_value_t *cir_comp_value_create(pos_t pos, c_type_t type, vec_cir_comp_store_t stores);
// Destroy a `cir_comp_value` node and any owned children.
void              cir_comp_value_delete(cir_comp_value_t *node);

// Construct a `cir_value` node with non-owning refernce to a temporary value.
cir_value_t *cir_value_create_tmpval(cir_tmpval_t const *tmpval);
// Construct a `cir_value` node with non-owning refernce to a scoped value.
cir_value_t *cir_value_create_scope_val(pos_t pos, cir_scope_val_t const *scope_val);
// Construct a `cir_value` node wrapping a primitive constant.
cir_value_t *cir_value_create_const(cir_const_t *iconst);
// Construct a `cir_value` node wrapping a compound constant.
cir_value_t *cir_value_create_comp_const(cir_comp_const_t *comp_const);
// Construct a `cir_value` node wrapping a compound value (run-time constant).
cir_value_t *cir_value_create_comp_value(cir_comp_value_t *comp_value);
// Destroy a `cir_value` node and the child it owns.
void         cir_value_delete(cir_value_t *node);

cir_expr_common_t cir_expr_common_clone(cir_expr_common_t const *common);

// Construct a `cir_call` node.
cir_call_t *cir_call_create(cir_expr_common_t common, cir_expr_t *func, vec_cir_expr_t args);
// Destroy a `cir_call` node and any owned children.
void        cir_call_delete(cir_call_t *node);

// Construct a `cir_cast` node.
cir_cast_t *cir_cast_create(cir_expr_common_t common, cir_expr_t *value);
// Destroy a `cir_cast` node and any owned children.
void        cir_cast_delete(cir_cast_t *node);

// Construct a `cir_ternary` node.
cir_ternary_t  *
    cir_ternary_create(cir_expr_common_t common, cir_expr_t *cond, cir_expr_t *if_expr, cir_expr_t *else_expr);
// Destroy a `cir_ternary` node and any owned children.
void cir_ternary_delete(cir_ternary_t *node);

// Construct a `cir_calc` node.
cir_calc_t *cir_calc_create(cir_expr_common_t common, cir_calc_op_t op, cir_expr_t *lhs, cir_expr_t *rhs);
// Destroy a `cir_calc` node and any owned children.
void        cir_calc_delete(cir_calc_t *node);

// Construct a `cir_addrof` node.
cir_addrof_t *cir_addrof_create(cir_expr_common_t common, cir_expr_t *expr);
// Destroy a `cir_addrof` node and any owned children.
void          cir_addrof_delete(cir_addrof_t *node);

// Construct a `cir_deref` node.
cir_deref_t *cir_deref_create(cir_expr_common_t common, cir_expr_t *expr);
// Destroy a `cir_deref` node and any owned children.
void         cir_deref_delete(cir_deref_t *node);

// Construct a `cir_tmpval` node.
cir_tmpval_t *cir_tmpval_create(cir_expr_t *inner);
// Destroy a `cir_tmpval` node and any owned children.
void          cir_tmpval_delete(cir_tmpval_t *node);

// Construct a `cir_exprs` node.
cir_exprs_t *cir_exprs_create(cir_expr_common_t common, vec_cir_expr_t exprs);
// Construct a `cir_exprs` node.
cir_exprs_t *cir_exprs_create2(cir_expr_common_t common, vec_cir_tmpval_t tmpvals, vec_cir_expr_t exprs);
// Destroy a `cir_exprs` node and any owned children.
void         cir_exprs_delete(cir_exprs_t *node);

// Construct a `cir_assign` node.
cir_assign_t *cir_assign_create(cir_expr_t *lhs, cir_expr_t *rhs);
// Destroy a `cir_assign` node and any owned children.
void          cir_assign_delete(cir_assign_t *node);

// Construct a `cir_expr` node wrapping a value.
cir_expr_t *cir_expr_create_value(cir_value_t *value);
// Construct a `cir_expr` node wrapping a function call.
cir_expr_t *cir_expr_create_call(cir_call_t *call);
// Construct a `cir_expr` node wrapping a cast.
cir_expr_t *cir_expr_create_cast(cir_cast_t *cast);
// Construct a `cir_expr` node wrapping a ternary expression.
cir_expr_t *cir_expr_create_ternary(cir_ternary_t *ternary);
// Construct a `cir_expr` node wrapping a calculation.
cir_expr_t *cir_expr_create_calc(cir_calc_t *calc);
// Construct a `cir_expr` node wrapping an address-of.
cir_expr_t *cir_expr_create_addrof(cir_addrof_t *addrof);
// Construct a `cir_expr` node wrapping a pointer dereference.
cir_expr_t *cir_expr_create_deref(cir_deref_t *deref);
// Construct a `cir_expr` node wrapping a comma-separated expression list.
cir_expr_t *cir_expr_create_exprs(cir_exprs_t *exprs);
// Construct a `cir_expr` node wrapping an assignment.
cir_expr_t *cir_expr_create_assign(cir_assign_t *assign);
// Construct a `cir_expr` node wrapping a statement.
cir_expr_t *cir_expr_create_stmt(cir_stmt_t *stmt);
// Destroy a `cir_expr` node and the child it owns.
void        cir_expr_delete(cir_expr_t *node);

// Construct a `cir_for` node.
// Takes ownership of `scope`.
cir_for_t *cir_for_create(
    pos_t pos, cir_scope_t *scope, cir_stmt_t *init, cir_expr_t *cond, cir_expr_t *inc, cir_stmt_t *body
);
// Destroy a `cir_for` node and any owned children.
void cir_for_delete(cir_for_t *node);

// Construct a `cir_while` node.
cir_while_t *cir_while_create(pos_t pos, cir_scope_t *scope, cir_expr_t *cond, cir_stmt_t *body, bool is_do_while);
// Destroy a `cir_while` node and any owned children.
void         cir_while_delete(cir_while_t *node);

// Construct a `cir_switch` node.
cir_switch_t *cir_switch_create(pos_t pos, cir_scope_t *scope, cir_expr_t *value, cir_stmt_t *body);
// Destroy a `cir_switch` node and any owned children.
void          cir_switch_delete(cir_switch_t *node);

// Construct a `cir_case` node.
cir_case_t *cir_case_create(pos_t pos, cir_expr_t *lo, cir_expr_t *hi, cir_stmt_t *body);
// Destroy a `cir_case` node and any owned children.
void        cir_case_delete(cir_case_t *node);

// Construct a `cir_if` node.
cir_if_t *cir_if_create(pos_t pos, cir_expr_t *cond, cir_stmt_t *if_body, cir_stmt_t *else_body);
// Destroy a `cir_if` node and any owned children.
void      cir_if_delete(cir_if_t *node);

// Construct a `cir_label` node.
cir_label_t *cir_label_create(pos_t pos, char *name, cir_stmt_t *body);
// Destroy a `cir_label` node and any owned children.
void         cir_label_delete(cir_label_t *node);

// Construct a `cir_goto` node.
cir_goto_t *cir_goto_create(pos_t pos, char *label);
// Destroy a `cir_goto` node and any owned children.
void        cir_goto_delete(cir_goto_t *node);

// Construct a `cir_break` node.
cir_break_t *cir_break_create(pos_t pos, bool is_continue);
// Destroy a `cir_break` node and any owned children.
void         cir_break_delete(cir_break_t *node);

// Construct a `cir_return` node.
cir_return_t *cir_return_create(pos_t pos, cir_expr_t *value);
// Destroy a `cir_return` node and any owned children.
void          cir_return_delete(cir_return_t *node);

// Construct a `cir_stmts` node.
// Takes ownership of `scope`.
cir_stmts_t *cir_stmts_create(pos_t pos, cir_scope_t *scope, vec_cir_stmt_t stmts);
// Destroy a `cir_stmts` node and any owned children.
void         cir_stmts_delete(cir_stmts_t *node);

// Construct a `cir_stmt` node wrapping a compound statement.
cir_stmt_t *cir_stmt_create_stmts(cir_stmts_t *stmts);
// Construct a `cir_stmt` node wrapping a for loop.
cir_stmt_t *cir_stmt_create_for(cir_for_t *for_loop);
// Construct a `cir_stmt` node wrapping a while/do-while loop.
cir_stmt_t *cir_stmt_create_while(cir_while_t *while_loop);
// Construct a `cir_stmt` node wrapping a switch statement.
cir_stmt_t *cir_stmt_create_switch(cir_switch_t *switch_stmt);
// Construct a `cir_stmt` node wrapping an if/else.
cir_stmt_t *cir_stmt_create_if(cir_if_t *if_stmt);
// Construct a `cir_stmt` node wrapping a case-labeled statement.
cir_stmt_t *cir_stmt_create_case(cir_case_t *cir_case);
// Construct a `cir_stmt` node wrapping a labeled statement.
cir_stmt_t *cir_stmt_create_label(cir_label_t *label);
// Construct a `cir_stmt` node wrapping a goto.
cir_stmt_t *cir_stmt_create_goto(cir_goto_t *goto_stmt);
// Construct a `cir_stmt` node wrapping a break.
cir_stmt_t *cir_stmt_create_break(cir_break_t *break_stmt);
// Construct a `cir_stmt` node wrapping a return.
cir_stmt_t *cir_stmt_create_return(cir_return_t *return_stmt);
// Construct a `cir_stmt` node wrapping an expression statement.
cir_stmt_t *cir_stmt_create_expr(cir_expr_t *expr);
// Construct a `cir_stmt` node wrapping a unit list.
cir_stmt_t *cir_stmt_create_units(cir_unit_list_t *units);
// Construct an empty `cir_stmt` node.
cir_stmt_t *cir_stmt_create_nop(pos_t pos);
// Destroy a `cir_stmt` node and the child it owns.
void        cir_stmt_delete(cir_stmt_t *node);

// Construct a `cir_decl` node.
cir_decl_t *cir_decl_create(pos_t pos, c_type_t type, char *name, cir_expr_t *init);
// Destroy a `cir_decl` node and any owned children.
void        cir_decl_delete(cir_decl_t *node);

// Construct a `cir_func` node.
cir_func_t *cir_func_create(pos_t pos, cir_scope_t *scope, c_type_t type, char *name, vec_cir_stmt_t body);
// Destroy a `cir_func` node and any owned children.
void        cir_func_delete(cir_func_t *node);

// Construct a `cir_unit` node wrapping a declaration.
cir_unit_t *cir_unit_create_decl(cir_decl_t *decl);
// Construct a `cir_unit` node wrapping a function.
cir_unit_t *cir_unit_create_func(cir_func_t *func);
// Destroy a `cir_unit` node and the child it owns.
void        cir_unit_delete(cir_unit_t *node);

// Construct a `cir_unit_list` node.
cir_unit_list_t *cir_unit_list_create(pos_t pos, vec_cir_unit_t units);
// Destroy a `cir_unit_list` node and any owned children.
void             cir_unit_list_delete(cir_unit_list_t *node);

// Construct a `cir_trans_unit` node.
cir_trans_unit_t *cir_trans_unit_create(cir_scope_t *scope, vec_cir_unit_t units);
// Destroy a `cir_trans_unit` node and any owned children.
void              cir_trans_unit_delete(cir_trans_unit_t *node);


// Create a new scope. `parent` must be `NULL` iff `kind == CIR_SCOPE_GLOBAL`.
cir_scope_t *cir_scope_create(cir_scope_type_t kind, cir_scope_t *parent);
// Destroy a scope. Frees the owned value-entry wrappers and releases the typedef and tags;
// the referenced nodes, the parent, and any child scopes are untouched.
void         cir_scope_delete(cir_scope_t *scope);
// Walk up to the enclosing function scope, or `NULL` if none.
cir_scope_t *cir_scope_func(cir_scope_t *scope);

// Add a variable declaration to the value namespace of `scope`.
// Returns `false` if `name` already exists in *this* scope (shadowing a parent is allowed).
bool cir_scope_add_decl(cctx_t *ctx, cir_scope_t *scope, cir_decl_t const *decl);
// Add a function to the value namespace of `scope`.
// Returns `false` if `name` already exists in *this* scope.
bool cir_scope_add_func(cctx_t *ctx, cir_scope_t *scope, cir_func_t const *func);
// Add an enum constant to the value namespace of `scope`.
// Returns `false` if `name` already exists in *this* scope.
bool cir_scope_add_enum_const(cctx_t *ctx, cir_scope_t *scope, char const *name, cir_const_t *enum_const);
// Add a typedef to `scope`. Takes ownership of the passed type.
// Returns `false` if `name` already exists in *this* scope.
bool cir_scope_add_typedef(cctx_t *ctx, cir_scope_t *scope, char const *name, pos_t pos, c_type_t type);
// Add a tag (struct/union/enum type) to `scope`. Takes ownership of the passed type.
// Returns `false` if `name` already exists in *this* scope.
bool cir_scope_add_tag(cctx_t *ctx, cir_scope_t *scope, c_comp_type_t *type);
// Add a label to the enclosing function scope, walking up from `scope`.
// Returns `false` if no function scope is found, or if `name` is already a label in it.
bool cir_scope_add_label(cctx_t *ctx, cir_scope_t *scope, cir_label_t *label);

// Look up a value by `name`, walking parent scopes. Returns `NULL` if not found.
cir_scope_val_t     *cir_scope_lookup_value(cir_scope_t const *scope, char const *name);
// Look up a typedef by `name`, walking parent scopes. Returns `NULL` if not found.
cir_typedef_t const *cir_scope_lookup_typedef(cir_scope_t const *scope, char const *name);
// Look up a tag by `name`, walking parent scopes. Returns `NULL` if not found.
// Returns a non-owning pointer.
c_comp_type_t       *cir_scope_lookup_tag(cir_scope_t const *scope, char const *name);
// Look up a label by `name` in the enclosing function scope only (no parent walk past it).
// Returns `NULL` if no function scope is found or the label is not defined there.
cir_label_t         *cir_scope_lookup_label(cir_scope_t const *scope, char const *name);



// Debug-print a `cir_scope_t` C IR node.
void cir_scope_dbg(cir_scope_t const *scope, int indent, FILE *to);
// Debug-print a `cir_scope_val_t` C IR node.
void cir_scope_val_dbg(cir_scope_val_t const *scope_val, int indent, FILE *to);
// Debug-print a `cir_typedef_t` C IR node.
void cir_typedef_dbg(cir_typedef_t const *cir_typedef, int indent, FILE *to);

// Debug-print a `cir_const_t` C IR node.
void cir_const_dbg(cir_const_t const *iconst, int indent, FILE *to);
// Debug-print a `cir_comp_const_t` C IR node.
void cir_comp_const_dbg(cir_comp_const_t const *comp_const, int indent, FILE *to);
// Debug-print a `cir_comp_store_t` C IR node.
void cir_comp_store_dbg(cir_comp_store_t const *comp_store, int indent, FILE *to);
// Debug-print a `cir_comp_value_t` C IR node.
void cir_comp_value_dbg(cir_comp_value_t const *comp_value, int indent, FILE *to);
// Debug-print a `cir_value_t` C IR node.
void cir_value_dbg(cir_value_t const *value, int indent, FILE *to);

// Debug-print a `cir_call_t` C IR node.
void cir_call_dbg(cir_call_t const *call, int indent, FILE *to);
// Debug-print a `cir_cast_t` C IR node.
void cir_cast_dbg(cir_cast_t const *cast, int indent, FILE *to);
// Debug-print a `cir_ternary_t` C IR node.
void cir_ternary_dbg(cir_ternary_t const *ternary, int indent, FILE *to);
// Debug-print a `cir_calc_t` C IR node.
void cir_calc_dbg(cir_calc_t const *calc, int indent, FILE *to);
// Debug-print a `cir_addrof_t` C IR node.
void cir_addrof_dbg(cir_addrof_t const *addrof, int indent, FILE *to);
// Debug-print a `cir_deref_t` C IR node.
void cir_deref_dbg(cir_deref_t const *deref, int indent, FILE *to);
// Debug-print a `cir_tmpval_t` C IR node.
void cir_tmpval_dbg(cir_tmpval_t const *tmpval, int indent, FILE *to);
// Debug-print a `cir_exprs_t` C IR node.
void cir_exprs_dbg(cir_exprs_t const *exprs, int indent, FILE *to);
// Debug-print a `cir_assign_t` C IR node.
void cir_assign_dbg(cir_assign_t const *assign, int indent, FILE *to);
// Debug-print a `cir_expr_t` C IR node.
void cir_expr_dbg(cir_expr_t const *expr, int indent, FILE *to);

// Debug-print a `cir_stmts_t` C IR node.
void cir_stmts_dbg(cir_stmts_t const *stmts, int indent, FILE *to);
// Debug-print a `(cir_f_t` C IR node.
void cir_for_dbg(cir_for_t const *cir_for, int indent, FILE *to);
// Debug-print a `cir_while_t` C IR node.
void cir_while_dbg(cir_while_t const *cir_while, int indent, FILE *to);
// Debug-print a `cir_switch_t` C IR node.
void cir_switch_dbg(cir_switch_t const *cir_switch, int indent, FILE *to);
// Debug-print a `cir_case_t` C IR node.
void cir_case_dbg(cir_case_t const *cir_case, int indent, FILE *to);
// Debug-print a `cir__t` C IR node.
void cir_if_dbg(cir_if_t const *cir_if, int indent, FILE *to);
// Debug-print a `cir_label_t` C IR node.
void cir_label_dbg(cir_label_t const *label, int indent, FILE *to);
// Debug-print a `cir_goto_t` C IR node.
void cir_goto_dbg(cir_goto_t const *goto_stmt, int indent, FILE *to);
// Debug-print a `cir_break_t` C IR node.
void cir_break_dbg(cir_break_t const *break_stmt, int indent, FILE *to);
// Debug-print a `cir_return_t` C IR node.
void cir_return_dbg(cir_return_t const *cir_return, int indent, FILE *to);
// Debug-print a `cir_stmt_t` C IR node.
void cir_stmt_dbg(cir_stmt_t const *stmt, int indent, FILE *to);

// Debug-print a `cir_decl_t` C IR node.
void cir_decl_dbg(cir_decl_t const *decl, int indent, FILE *to);
// Debug-print a `cir_func_t` C IR node.
void cir_func_dbg(cir_func_t const *func, int indent, FILE *to);
// Debug-print a `cir_unit_t` C IR node.
void cir_unit_dbg(cir_unit_t const *unit, int indent, FILE *to);
// Debug-print a `cir_unit_list_t` C IR node.
void cir_unit_list_dbg(cir_unit_list_t const *unit_list, int indent, FILE *to);
// Debug-print a `cir_trans_unit_t` C IR node.
void cir_trans_unit_dbg(cir_trans_unit_t const *trans_unit, int indent, FILE *to);
