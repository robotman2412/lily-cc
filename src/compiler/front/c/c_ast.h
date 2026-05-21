
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once



#include "c_tokenizer.h"
#include "c_types.h"
#include "compiler.h"
#include "ir_types.h"
#include "refcount.h"
#include "vec.h"

#include <stdio.h>



// Tag of `c_ast_expr_t`.
typedef enum {
    C_AST_TAG_EXPR_INFIX,
    C_AST_TAG_EXPR_PREFIX,
    C_AST_TAG_EXPR_SUFFIX,
    C_AST_TAG_EXPR_CAST,
    C_AST_TAG_EXPR_CALL,
    C_AST_TAG_EXPR_IDENT,
    C_AST_TAG_EXPR_ICONST,
    C_AST_TAG_EXPR_SCONST,
    C_AST_TAG_EXPRS,
} c_ast_expr_tag_t;



// Infix expression (e.g. `1 + 2`).
typedef struct c_ast_expr_infix  c_ast_expr_infix_t;
// Prefix expression (e.g. `~3`).
typedef struct c_ast_expr_prefix c_ast_expr_prefix_t;
// Suffix expression (`expr++` or `expr--`).
typedef struct c_ast_expr_suffix c_ast_expr_suffix_t;
// Index expression (`ptr[index]`).
typedef struct c_ast_expr_index  c_ast_expr_index_t;
// Cast expression (`(type) expr`).
typedef struct c_ast_expr_cast   c_ast_expr_cast_t;
// Function call expression (`expr(args)`).
typedef struct c_ast_expr_call   c_ast_expr_call_t;
// Identifier expression (e.g. `foo`).
typedef struct c_ast_expr_ident  c_ast_expr_ident_t;
// Numeric constant expression (integer or character constant).
typedef struct c_ast_expr_iconst c_ast_expr_iconst_t;
// String constant expression.
typedef struct c_ast_expr_sconst c_ast_expr_sconst_t;
// Expression list (e.g. `(1, 2 + 3, 4)`, or function call arguments).
typedef struct c_ast_exprs       c_ast_exprs_t;
// One expression; tagged union of the variants above.
typedef struct c_ast_expr        c_ast_expr_t;

VEC_TYPE_DEF(vec_c_ast_expr_t, c_ast_expr_t *)



// Infix expression (e.g. `1 + 2`).
struct c_ast_expr_infix {
    // Position of this node.
    pos_t         pos;
    // Left-hand side.
    c_ast_expr_t *lhs;
    // Operator token.
    c_tokentype_t oper;
    // Operator token position.
    pos_t         oper_pos;
    // Right-hand side.
    c_ast_expr_t *rhs;
};

// Prefix expression (e.g. `~3`).
struct c_ast_expr_prefix {
    // Position of this node.
    pos_t         pos;
    // Operator token.
    c_tokentype_t oper;
    // Operator token position.
    pos_t         oper_pos;
    // Operand.
    c_ast_expr_t *val;
};

// Suffix expression (`expr++` or `expr--`).
struct c_ast_expr_suffix {
    // Position of this node.
    pos_t         pos;
    // Operator token.
    c_tokentype_t oper;
    // Operator token position.
    pos_t         oper_pos;
    // Operand.
    c_ast_expr_t *val;
};

// Index expression (`ptr[index]`).
struct c_ast_expr_index {
    // Position of this node.
    pos_t         pos;
    // Left-hand side (the thing being indexed).
    c_ast_expr_t *lhs;
    // Right-hand side (the index).
    c_ast_expr_t *rhs;
};

// Cast expression (`(type) expr`).
struct c_ast_expr_cast {
    // Position of this node.
    pos_t         pos;
    // Refcount pointer of `c_type_t`; target type of the cast.
    rc_t          type_rc;
    // Type name position.
    pos_t         type_pos;
    // Value being cast.
    c_ast_expr_t *val;
};

// Function call expression (`expr(args)`).
struct c_ast_expr_call {
    // Position of this node.
    pos_t          pos;
    // Function being called.
    c_ast_expr_t  *func;
    // Call arguments.
    c_ast_exprs_t *args;
};

// Identifier expression (e.g. `foo`).
struct c_ast_expr_ident {
    // Position of this node.
    pos_t pos;
    // Identifier name (owned, NUL-terminated).
    char *name;
};

// Numeric constant expression (integer or character constant).
struct c_ast_expr_iconst {
    // Position of this node.
    pos_t      pos;
    // C type of this literal.
    c_prim_t   prim;
    // Constant value.
    ir_const_t value;
};

// String constant expression.
struct c_ast_expr_sconst {
    // Position of this node.
    pos_t      pos;
    // String bytes (owned; may contain embedded NULs).
    vec_char_t value;
};

// Expression list (e.g. `(1, 2 + 3, 4)`, or function call arguments).
struct c_ast_exprs {
    // Position of this node.
    pos_t            pos;
    // Expressions in the list.
    vec_c_ast_expr_t exprs;
};

// One expression; tagged union of the variants above.
struct c_ast_expr {
    // Position of this node.
    pos_t            pos;
    // Which variant is active.
    c_ast_expr_tag_t tag;
    union {
        c_ast_expr_infix_t  *expr_infix;
        c_ast_expr_prefix_t *expr_prefix;
        c_ast_expr_suffix_t *expr_suffix;
        c_ast_expr_cast_t   *expr_cast;
        c_ast_expr_call_t   *expr_call;
        c_ast_expr_ident_t  *expr_ident;
        c_ast_expr_iconst_t *expr_iconst;
        c_ast_expr_sconst_t *expr_sconst;
        c_ast_exprs_t       *exprs;
    };
};



// Construct an infix expression; the position spans from `lhs` through `rhs`.
c_ast_expr_infix_t *c_ast_expr_infix_create(c_ast_expr_t *lhs, pos_t oper_pos, c_tokentype_t oper, c_ast_expr_t *rhs);
// Print an infix expression.
void                c_ast_expr_infix_print(c_ast_expr_infix_t const *ast, int indent, FILE *to);
// Destroy an infix expression.
void                c_ast_expr_infix_destroy(c_ast_expr_infix_t *ast);

// Construct a prefix expression; the position spans from `oper_pos` through `val`.
c_ast_expr_prefix_t *c_ast_expr_prefix_create(pos_t oper_pos, c_tokentype_t oper, c_ast_expr_t *val);
// Print a prefix expression.
void                 c_ast_expr_prefix_print(c_ast_expr_prefix_t const *ast, int indent, FILE *to);
// Destroy a prefix expression.
void                 c_ast_expr_prefix_destroy(c_ast_expr_prefix_t *ast);

// Construct a suffix expression; the position spans from `val` through `oper_pos`.
c_ast_expr_suffix_t *c_ast_expr_suffix_create(c_ast_expr_t *val, pos_t oper_pos, c_tokentype_t oper);
// Print a suffix expression.
void                 c_ast_expr_suffix_print(c_ast_expr_suffix_t const *ast, int indent, FILE *to);
// Destroy a suffix expression.
void                 c_ast_expr_suffix_destroy(c_ast_expr_suffix_t *ast);

// Construct an index expression; the position spans from `lhs` through `close_pos` (the `]`).
c_ast_expr_index_t *c_ast_expr_index_create(c_ast_expr_t *lhs, c_ast_expr_t *rhs, pos_t close_pos);
// Print an index expression.
void                c_ast_expr_index_print(c_ast_expr_index_t const *ast, int indent, FILE *to);
// Destroy an index expression.
void                c_ast_expr_index_destroy(c_ast_expr_index_t *ast);

// Construct a cast expression; the position spans from `open_paren_pos` through `val`.
// Takes ownership of `type_rc`.
c_ast_expr_cast_t *c_ast_expr_cast_create(pos_t open_paren_pos, pos_t type_pos, rc_t type_rc, c_ast_expr_t *val);
// Print a cast expression.
void               c_ast_expr_cast_print(c_ast_expr_cast_t const *ast, int indent, FILE *to);
// Destroy a cast expression.
void               c_ast_expr_cast_destroy(c_ast_expr_cast_t *ast);

// Construct a call expression; the position spans from `func` through `args`.
c_ast_expr_call_t *c_ast_expr_call_create(c_ast_expr_t *func, c_ast_exprs_t *args);
// Print a call expression.
void               c_ast_expr_call_print(c_ast_expr_call_t const *ast, int indent, FILE *to);
// Destroy a call expression.
void               c_ast_expr_call_destroy(c_ast_expr_call_t *ast);

// Construct an identifier expression at position `pos`. Takes ownership of `name`.
c_ast_expr_ident_t *c_ast_expr_ident_create(pos_t pos, char *name);
// Print an identifier expression.
void                c_ast_expr_ident_print(c_ast_expr_ident_t const *ast, int indent, FILE *to);
// Destroy an identifier expression.
void                c_ast_expr_ident_destroy(c_ast_expr_ident_t *ast);

// Construct a numeric constant expression at position `pos`.
c_ast_expr_iconst_t *c_ast_expr_iconst_create(pos_t pos, c_prim_t prim, ir_const_t value);
// Print a numeric constant expression.
void                 c_ast_expr_iconst_print(c_ast_expr_iconst_t const *ast, int indent, FILE *to);
// Destroy a numeric constant expression.
void                 c_ast_expr_iconst_destroy(c_ast_expr_iconst_t *ast);

// Construct a string constant expression at position `pos`. Takes ownership of `value`.
c_ast_expr_sconst_t *c_ast_expr_sconst_create(pos_t pos, vec_char_t value);
// Print a string constant expression.
void                 c_ast_expr_sconst_print(c_ast_expr_sconst_t const *ast, int indent, FILE *to);
// Destroy a string constant expression.
void                 c_ast_expr_sconst_destroy(c_ast_expr_sconst_t *ast);

// Construct an expression list at position `pos`. Takes ownership of `exprs`.
c_ast_exprs_t *c_ast_exprs_create(pos_t pos, vec_c_ast_expr_t exprs);
// Print an expression list.
void           c_ast_exprs_print(c_ast_exprs_t const *ast, int indent, FILE *to);
// Destroy an expression list.
void           c_ast_exprs_destroy(c_ast_exprs_t *ast);

// Wrap a leaf node in a `c_ast_expr_t`; the position is inherited from the leaf.
// `variant` must point to the matching leaf type for `tag`, and ownership is transferred.
c_ast_expr_t *c_ast_expr_create(c_ast_expr_tag_t tag, void *variant);
// Print an expression.
void          c_ast_expr_print(c_ast_expr_t const *ast, int indent, FILE *to);
// Destroy an expression.
void          c_ast_expr_destroy(c_ast_expr_t *ast);
