
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "c_types.h"
#include "compiler.h"
#include "ir_types.h"
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

// Tag for `cir_value_t`.
typedef enum {
    CIR_VALUE_IDENT,
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
    CIR_EXPR_INC,
    CIR_EXPR_ADDROF,
    CIR_EXPR_DEREF,
} cir_expr_tag_t;

// Tag for `cir_stmt_t`.
typedef enum {
    CIR_STMT_STMTS,
    CIR_STMT_FOR,
    CIR_STMT_WHILE,
    CIR_STMT_IF,
    CIR_STMT_LABEL,
    CIR_STMT_RETURN,
    CIR_STMT_EXPR,
} cir_stmt_tag_t;

// Tag for `cir_unit_t`.
typedef enum {
    CIR_UNIT_DECL,
    CIR_UNIT_FUNC,
} cir_unit_tag_t;

// A value looked up by identifier.
typedef struct cir_ident      cir_ident_t;
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

// Function call operator.
typedef struct cir_call    cir_call_t;
// A casting expression.
typedef struct cir_cast    cir_cast_t;
// A ternary expression that takes a truth value and conditionally evaluates either branch.
typedef struct cir_ternary cir_ternary_t;
// A two-operand calculation that returns one value.
// The return type depends on operand types and operator.
typedef struct cir_calc    cir_calc_t;
// A pre/post increment/decrement operation.
typedef struct cir_inc     cir_inc_t;
// Address-of operator, may be emitted implicitly.
typedef struct cir_addrof  cir_addrof_t;
// Pointer dereference operator.
typedef struct cir_deref   cir_deref_t;
// Any type of expression; tagged union of structs above as well as `cir_value_t`.
typedef struct cir_expr    cir_expr_t;

// A compound statement (a sequence of statements wrapped in `{}`).
typedef struct cir_stmts     cir_stmts_t;
// A for loop.
typedef struct cir_for       cir_for_t;
// A while or do...while loop.
typedef struct cir_while     cir_while_t;
// An if/else statement.
typedef struct cir_if        cir_if_t;
// A labeled statement.
typedef struct cir_label     cir_label_t;
// A return statement.
typedef struct cir_return    cir_return_t;
// A statement that runs an expression.
typedef struct cir_stmt_expr cir_stmt_expr_t;
// Any type of statement; tagged union of structs above.
typedef struct cir_stmt      cir_stmt_t;

// A variable declaration with an optional initializer.
typedef struct cir_decl       cir_decl_t;
// A function definition or declaration.
typedef struct cir_func       cir_func_t;
// A global unit; tagged union of `cir_decl_t` or `cir_func_t`.
typedef struct cir_unit       cir_unit_t;
// A translation unit; a sequence of global units.
typedef struct cir_trans_unit cir_trans_unit_t;

VEC_TYPE_DEF(vec_cir_comp_store_t, cir_comp_store_t);
VEC_TYPE_DEF(vec_cir_expr_t, cir_expr_t *);
VEC_TYPE_DEF(vec_cir_stmt_t, cir_stmt_t *);
VEC_TYPE_DEF(vec_cir_unit_t, cir_unit_t *);



// A value looked up by identifier.
struct cir_ident {
    // Source location this was compiled from.
    pos_t pos;
    // Identifier name.
    char *ident;
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
    // Compound type; refcount ptr of `c_type_t`.
    rc_t     type_rc;
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
    // Compound type; refcount ptr of `c_type_t`.
    rc_t                 type_rc;
    // Vector of expressions and store offsets.
    vec_cir_comp_store_t stores;
};

// A C value; tagged union of value variants above.
struct cir_value {
    // Source location this was compiled from.
    pos_t           pos;
    // Active union variant.
    cir_value_tag_t tag;
    union {
        cir_ident_t      *ident;
        cir_const_t      *iconst;
        cir_comp_const_t *comp_const;
        cir_comp_value_t *comp_value;
    };
};


// Function call operator.
struct cir_call {
    // Source location this was compiled from.
    pos_t          pos;
    // Function expression.
    cir_expr_t    *func;
    // Function parameters.
    vec_cir_expr_t args;
};

// A casting expression.
struct cir_cast {
    // Source location this was compiled from.
    pos_t       pos;
    // Cast target type; refcount ptr of `c_type_t`.
    rc_t        type_rc;
    // Expression to be cast.
    cir_expr_t *value;
};

// A ternary expression that takes a truth value and conditionally evaluates either branch.
struct cir_ternary {
    // Source location this was compiled from.
    pos_t       pos;
    // Condition expression.
    cir_expr_t *cond;
    // Expression if true.
    cir_expr_t *if_expr;
    // Expression if false.
    cir_expr_t *else_expr;
};

// A two-operand calculation that returns one value.
// The return type depends on operand types and operator.
struct cir_calc {
    // Source location this was compiled from.
    pos_t         pos;
    // Calculation operator.
    cir_calc_op_t op;
    // Left-hand side operand.
    cir_expr_t   *lhs;
    // Right-hand side operand.
    cir_expr_t   *rhs;
};

// A pre/post increment/decrement operation.
struct cir_inc {
    // Source location this was compiled from.
    pos_t       pos;
    // Value to increment/decrement.
    cir_expr_t *expr;
    // Is pre-increment/pre-decrement.
    bool        is_pre;
    // Is decrement/pre-decrement.
    bool        is_dec;
};

// Address-of operator, may be emitted implicitly.
struct cir_addrof {
    // Source location this was compiled from.
    pos_t       pos;
    // Value to take the address of.
    cir_expr_t *expr;
};

// Pointer dereference operator.
struct cir_deref {
    // Source location this was compiled from.
    pos_t       pos;
    // Pointer to dereference.
    cir_expr_t *expr;
};

// Any type of expression; tagged union of expression variants plus a wrapped value.
struct cir_expr {
    // Source location this was compiled from.
    pos_t          pos;
    // Active union variant.
    cir_expr_tag_t tag;
    union {
        cir_value_t   *value;
        cir_call_t    *call;
        cir_cast_t    *cast;
        cir_ternary_t *ternary;
        cir_calc_t    *calc;
        cir_inc_t     *inc;
        cir_addrof_t  *addrof;
        cir_deref_t   *deref;
    };
};


// A compound statement (a sequence of statements wrapped in `{}`).
struct cir_stmts {
    // Source location this was compiled from.
    pos_t          pos;
    // Statements to run in order.
    vec_cir_stmt_t stmts;
};

// A for loop.
struct cir_for {
    // Source location this was compiled from.
    pos_t       pos;
    // Initializer (nullable).
    cir_stmt_t *init;
    // Loop condition (nullable; absence means an infinite loop).
    cir_expr_t *cond;
    // Increment expression (nullable).
    cir_expr_t *inc;
    // Loop body.
    cir_stmt_t *body;
};

// A while or do...while loop.
struct cir_while {
    // Source location this was compiled from.
    pos_t       pos;
    // Loop condition.
    cir_expr_t *cond;
    // Loop body.
    cir_stmt_t *body;
    // Is of `do...while` form.
    bool        is_do_while;
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

// A return statement.
struct cir_return {
    // Source location this was compiled from.
    pos_t       pos;
    // Return value (nullable for `return;`).
    cir_expr_t *value;
};

// A statement that runs an expression.
struct cir_stmt_expr {
    // Source location this was compiled from.
    pos_t       pos;
    // Expression to run.
    cir_expr_t *expr;
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
        cir_if_t        *if_stmt;
        cir_label_t     *label;
        cir_return_t    *return_stmt;
        cir_stmt_expr_t *expr_stmt;
    };
};


// A variable declaration with an optional initializer.
struct cir_decl {
    // Source location this was compiled from.
    pos_t       pos;
    // Variable type; refcount ptr of `c_type_t`.
    rc_t        type_rc;
    // Variable name.
    char       *name;
    // Initializer (nullable).
    cir_expr_t *init;
};

// A function definition or declaration.
struct cir_func {
    // Source location this was compiled from.
    pos_t       pos;
    // Function type; refcount ptr of `c_type_t`.
    rc_t        type_rc;
    // Function name.
    char       *name;
    // Parameter names (owned, NUL-terminated). Order and length match the parameter list of `type_rc`.
    vec_cstr_t  param_names;
    // Function body (nullable; absent for forward declarations).
    cir_stmt_t *body;
};

// A global unit; tagged union of a declaration or function definition.
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

// A translation unit; a sequence of global units.
struct cir_trans_unit {
    // Source location this was compiled from.
    pos_t           pos;
    // Global units in source order.
    vec_cir_unit_t  units;
};



// Construct a `cir_ident` node.
cir_ident_t *cir_ident_create(pos_t pos, char *ident);
// Destroy a `cir_ident` node and any owned children.
void         cir_ident_delete(cir_ident_t *node);

// Construct a `cir_const` node.
cir_const_t *cir_const_create(pos_t pos, c_prim_t prim, ir_const_t iconst);
// Destroy a `cir_const` node.
void         cir_const_delete(cir_const_t *node);

// Construct a `cir_comp_const` node.
cir_comp_const_t *cir_comp_const_create(pos_t pos, rc_t type_rc, uint8_t *blob);
// Destroy a `cir_comp_const` node and any owned children.
void              cir_comp_const_delete(cir_comp_const_t *node);

// Construct a `cir_comp_value` node.
cir_comp_value_t *cir_comp_value_create(pos_t pos, rc_t type_rc, vec_cir_comp_store_t stores);
// Destroy a `cir_comp_value` node and any owned children.
void              cir_comp_value_delete(cir_comp_value_t *node);

// Construct a `cir_value` node wrapping an identifier.
cir_value_t *cir_value_create_ident(cir_ident_t *ident);
// Construct a `cir_value` node wrapping a primitive constant.
cir_value_t *cir_value_create_const(cir_const_t *iconst);
// Construct a `cir_value` node wrapping a compound constant.
cir_value_t *cir_value_create_comp_const(cir_comp_const_t *comp_const);
// Construct a `cir_value` node wrapping a compound value.
cir_value_t *cir_value_create_comp_value(cir_comp_value_t *comp_value);
// Destroy a `cir_value` node and the child it owns.
void         cir_value_delete(cir_value_t *node);

// Construct a `cir_call` node.
cir_call_t *cir_call_create(pos_t pos, cir_expr_t *func, vec_cir_expr_t args);
// Destroy a `cir_call` node and any owned children.
void        cir_call_delete(cir_call_t *node);

// Construct a `cir_cast` node.
cir_cast_t *cir_cast_create(pos_t pos, rc_t type_rc, cir_expr_t *value);
// Destroy a `cir_cast` node and any owned children.
void        cir_cast_delete(cir_cast_t *node);

// Construct a `cir_ternary` node.
cir_ternary_t *cir_ternary_create(pos_t pos, cir_expr_t *cond, cir_expr_t *if_expr, cir_expr_t *else_expr);
// Destroy a `cir_ternary` node and any owned children.
void           cir_ternary_delete(cir_ternary_t *node);

// Construct a `cir_calc` node.
cir_calc_t *cir_calc_create(pos_t pos, cir_calc_op_t op, cir_expr_t *lhs, cir_expr_t *rhs);
// Destroy a `cir_calc` node and any owned children.
void        cir_calc_delete(cir_calc_t *node);

// Construct a `cir_inc` node.
cir_inc_t *cir_inc_create(pos_t pos, cir_expr_t *expr, bool is_pre, bool is_dec);
// Destroy a `cir_inc` node and any owned children.
void       cir_inc_delete(cir_inc_t *node);

// Construct a `cir_addrof` node.
cir_addrof_t *cir_addrof_create(pos_t pos, cir_expr_t *expr);
// Destroy a `cir_addrof` node and any owned children.
void          cir_addrof_delete(cir_addrof_t *node);

// Construct a `cir_deref` node.
cir_deref_t *cir_deref_create(pos_t pos, cir_expr_t *expr);
// Destroy a `cir_deref` node and any owned children.
void         cir_deref_delete(cir_deref_t *node);

// Construct a `cir_expr` node wrapping a value.
cir_expr_t *cir_expr_create_value(cir_value_t *value);
// Construct a `cir_expr` node wrapping a function call.
cir_expr_t *cir_expr_create_call(cir_call_t *call);
// Construct a `cir_expr` node wrapping a cast.
cir_expr_t *cir_expr_create_cast(cir_cast_t *cast);
// Construct a `cir_expr` node wrapping a ternary.
cir_expr_t *cir_expr_create_ternary(cir_ternary_t *ternary);
// Construct a `cir_expr` node wrapping a calculation.
cir_expr_t *cir_expr_create_calc(cir_calc_t *calc);
// Construct a `cir_expr` node wrapping an increment/decrement.
cir_expr_t *cir_expr_create_inc(cir_inc_t *inc);
// Construct a `cir_expr` node wrapping an address-of.
cir_expr_t *cir_expr_create_addrof(cir_addrof_t *addrof);
// Construct a `cir_expr` node wrapping a pointer dereference.
cir_expr_t *cir_expr_create_deref(cir_deref_t *deref);
// Destroy a `cir_expr` node and the child it owns.
void        cir_expr_delete(cir_expr_t *node);

// Construct a `cir_for` node.
cir_for_t *cir_for_create(pos_t pos, cir_stmt_t *init, cir_expr_t *cond, cir_expr_t *inc, cir_stmt_t *body);
// Destroy a `cir_for` node and any owned children.
void       cir_for_delete(cir_for_t *node);

// Construct a `cir_while` node.
cir_while_t *cir_while_create(pos_t pos, cir_expr_t *cond, cir_stmt_t *body, bool is_do_while);
// Destroy a `cir_while` node and any owned children.
void         cir_while_delete(cir_while_t *node);

// Construct a `cir_if` node.
cir_if_t *cir_if_create(pos_t pos, cir_expr_t *cond, cir_stmt_t *if_body, cir_stmt_t *else_body);
// Destroy a `cir_if` node and any owned children.
void      cir_if_delete(cir_if_t *node);

// Construct a `cir_label` node.
cir_label_t *cir_label_create(pos_t pos, char *name, cir_stmt_t *body);
// Destroy a `cir_label` node and any owned children.
void         cir_label_delete(cir_label_t *node);

// Construct a `cir_return` node.
cir_return_t *cir_return_create(pos_t pos, cir_expr_t *value);
// Destroy a `cir_return` node and any owned children.
void          cir_return_delete(cir_return_t *node);

// Construct a `cir_stmts` node.
cir_stmts_t     *cir_stmts_create(pos_t pos, vec_cir_stmt_t stmts);
// Destroy a `cir_stmts` node and any owned children.
void             cir_stmts_delete(cir_stmts_t *node);

// Construct a `cir_stmt_expr` node.
cir_stmt_expr_t *cir_stmt_expr_create(pos_t pos, cir_expr_t *expr);
// Destroy a `cir_stmt_expr` node and any owned children.
void             cir_stmt_expr_delete(cir_stmt_expr_t *node);

// Construct a `cir_stmt` node wrapping a compound statement.
cir_stmt_t *cir_stmt_create_stmts(cir_stmts_t *stmts);
// Construct a `cir_stmt` node wrapping a for loop.
cir_stmt_t *cir_stmt_create_for(cir_for_t *for_loop);
// Construct a `cir_stmt` node wrapping a while/do-while loop.
cir_stmt_t *cir_stmt_create_while(cir_while_t *while_loop);
// Construct a `cir_stmt` node wrapping an if/else.
cir_stmt_t *cir_stmt_create_if(cir_if_t *if_stmt);
// Construct a `cir_stmt` node wrapping a labeled statement.
cir_stmt_t *cir_stmt_create_label(cir_label_t *label);
// Construct a `cir_stmt` node wrapping a return.
cir_stmt_t *cir_stmt_create_return(cir_return_t *return_stmt);
// Construct a `cir_stmt` node wrapping an expression statement.
cir_stmt_t *cir_stmt_create_expr(cir_stmt_expr_t *expr_stmt);
// Destroy a `cir_stmt` node and the child it owns.
void        cir_stmt_delete(cir_stmt_t *node);

// Construct a `cir_decl` node.
cir_decl_t *cir_decl_create(pos_t pos, rc_t type_rc, char *name, cir_expr_t *init);
// Destroy a `cir_decl` node and any owned children.
void        cir_decl_delete(cir_decl_t *node);

// Construct a `cir_func` node.
cir_func_t *cir_func_create(pos_t pos, rc_t type_rc, char *name, vec_cstr_t param_names, cir_stmt_t *body);
// Destroy a `cir_func` node and any owned children.
void        cir_func_delete(cir_func_t *node);

// Construct a `cir_unit` node wrapping a declaration.
cir_unit_t       *cir_unit_create_decl(cir_decl_t *decl);
// Construct a `cir_unit` node wrapping a function.
cir_unit_t       *cir_unit_create_func(cir_func_t *func);
// Destroy a `cir_unit` node and the child it owns.
void              cir_unit_delete(cir_unit_t *node);

// Construct a `cir_trans_unit` node.
cir_trans_unit_t *cir_trans_unit_create(pos_t pos, vec_cir_unit_t units);
// Destroy a `cir_trans_unit` node and any owned children.
void              cir_trans_unit_delete(cir_trans_unit_t *node);
