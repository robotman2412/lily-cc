
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_ast.h"

#include "c_compiler.h"
#include "c_tokenizer.h"
#include "c_types.h"
#include "ir_serialization.h"
#include "lilycc_malloc.h"

#include <stdio.h>



// Helper that prints some spaces for indentation.
static void pindent(int indent, FILE *to) {
    for (int i = 0; i < indent; i++) {
        fputs("  ", to);
    }
}


// Construct an infix expression; the position spans from `lhs` through `rhs`.
c_ast_expr_infix_t *c_ast_expr_infix_create(c_ast_expr_t *lhs, pos_t oper_pos, c_tokentype_t oper, c_ast_expr_t *rhs) {
    c_ast_expr_infix_t *ast = lilycc_malloc(sizeof(c_ast_expr_infix_t));
    ast->pos                = pos_including(lhs->pos, rhs->pos);
    ast->lhs                = lhs;
    ast->oper               = oper;
    ast->oper_pos           = oper_pos;
    ast->rhs                = rhs;
    return ast;
}

// Print an infix expression.
void c_ast_expr_infix_print(c_ast_expr_infix_t const *ast, int indent, FILE *to) {
    indent++;
    fputs("expr_infix\n", to);

    pindent(indent, to);
    fputs("lhs: ", to);
    c_ast_expr_print(ast->lhs, indent, to);

    pindent(indent, to);
    fprintf(to, "oper: %s\n", c_token_name[ast->oper]);

    pindent(indent, to);
    fputs("rhs: ", to);
    c_ast_expr_print(ast->rhs, indent, to);
}

// Destroy an infix expression.
void c_ast_expr_infix_destroy(c_ast_expr_infix_t *ast) {
    c_ast_expr_destroy(ast->lhs);
    c_ast_expr_destroy(ast->rhs);
    lilycc_free(ast);
}


// Construct a prefix expression; the position spans from `oper_pos` through `val`.
c_ast_expr_prefix_t *c_ast_expr_prefix_create(pos_t oper_pos, c_tokentype_t oper, c_ast_expr_t *val) {
    c_ast_expr_prefix_t *ast = lilycc_malloc(sizeof(c_ast_expr_prefix_t));
    ast->pos                 = pos_including(oper_pos, val->pos);
    ast->oper                = oper;
    ast->oper_pos            = oper_pos;
    ast->val                 = val;
    return ast;
}

// Print a prefix expression.
void c_ast_expr_prefix_print(c_ast_expr_prefix_t const *ast, int indent, FILE *to) {
    indent++;
    fputs("expr_prefix\n", to);

    pindent(indent, to);
    fprintf(to, "oper: %s\n", c_token_name[ast->oper]);

    pindent(indent, to);
    fputs("val: ", to);
    c_ast_expr_print(ast->val, indent, to);
}

// Destroy a prefix expression.
void c_ast_expr_prefix_destroy(c_ast_expr_prefix_t *ast) {
    c_ast_expr_destroy(ast->val);
    lilycc_free(ast);
}


// Construct a suffix expression; the position spans from `val` through `oper_pos`.
c_ast_expr_suffix_t *c_ast_expr_suffix_create(c_ast_expr_t *val, pos_t oper_pos, c_tokentype_t oper) {
    c_ast_expr_suffix_t *ast = lilycc_malloc(sizeof(c_ast_expr_suffix_t));
    ast->pos                 = pos_including(val->pos, oper_pos);
    ast->oper                = oper;
    ast->oper_pos            = oper_pos;
    ast->val                 = val;
    return ast;
}

// Print a suffix expression.
void c_ast_expr_suffix_print(c_ast_expr_suffix_t const *ast, int indent, FILE *to) {
    indent++;
    fputs("expr_suffix\n", to);

    pindent(indent, to);
    fputs("val: ", to);
    c_ast_expr_print(ast->val, indent, to);

    pindent(indent, to);
    fprintf(to, "oper: %s\n", c_token_name[ast->oper]);
}

// Destroy a suffix expression.
void c_ast_expr_suffix_destroy(c_ast_expr_suffix_t *ast) {
    c_ast_expr_destroy(ast->val);
    lilycc_free(ast);
}


// Construct an index expression; the position spans from `lhs` through `close_pos` (the `]`).
c_ast_expr_index_t *c_ast_expr_index_create(c_ast_expr_t *lhs, c_ast_expr_t *rhs, pos_t close_pos) {
    c_ast_expr_index_t *ast = lilycc_malloc(sizeof(c_ast_expr_index_t));
    ast->pos                = pos_including(lhs->pos, close_pos);
    ast->lhs                = lhs;
    ast->rhs                = rhs;
    return ast;
}

// Print an index expression.
void c_ast_expr_index_print(c_ast_expr_index_t const *ast, int indent, FILE *to) {
    indent++;
    fputs("expr_index\n", to);

    pindent(indent, to);
    fputs("lhs: ", to);
    c_ast_expr_print(ast->lhs, indent, to);

    pindent(indent, to);
    fputs("rhs: ", to);
    c_ast_expr_print(ast->rhs, indent, to);
}

// Destroy an index expression.
void c_ast_expr_index_destroy(c_ast_expr_index_t *ast) {
    c_ast_expr_destroy(ast->lhs);
    c_ast_expr_destroy(ast->rhs);
    lilycc_free(ast);
}


// Construct a cast expression; the position spans from `open_paren_pos` through `val`.
// Takes ownership of `type_rc`.
c_ast_expr_cast_t *c_ast_expr_cast_create(pos_t open_paren_pos, pos_t type_pos, rc_t type_rc, c_ast_expr_t *val) {
    c_ast_expr_cast_t *ast = lilycc_malloc(sizeof(c_ast_expr_cast_t));
    ast->pos               = pos_including(open_paren_pos, val->pos);
    ast->type_rc           = type_rc;
    ast->type_pos          = type_pos;
    ast->val               = val;
    return ast;
}

// Print a cast expression.
void c_ast_expr_cast_print(c_ast_expr_cast_t const *ast, int indent, FILE *to) {
    indent++;
    fputs("expr_cast\n", to);

    pindent(indent, to);
    fputs("type: ", to);
    c_type_explain(ast->type_rc->data, to);

    pindent(indent, to);
    fputs("val: ", to);
    c_ast_expr_print(ast->val, indent, to);
}

// Destroy a cast expression.
void c_ast_expr_cast_destroy(c_ast_expr_cast_t *ast) {
    rc_delete(ast->type_rc);
    c_ast_expr_destroy(ast->val);
    lilycc_free(ast);
}


// Construct a call expression; the position spans from `func` through `args`.
c_ast_expr_call_t *c_ast_expr_call_create(c_ast_expr_t *func, c_ast_exprs_t *args) {
    c_ast_expr_call_t *ast = lilycc_malloc(sizeof(c_ast_expr_call_t));
    ast->pos               = pos_including(func->pos, args->pos);
    ast->func              = func;
    ast->args              = args;
    return ast;
}

// Print a call expression.
void c_ast_expr_call_print(c_ast_expr_call_t const *ast, int indent, FILE *to) {
    indent++;
    fputs("expr_call\n", to);

    pindent(indent, to);
    fputs("func: ", to);
    c_ast_expr_print(ast->func, indent, to);

    pindent(indent, to);
    fputs("args: ", to);
    c_ast_exprs_print(ast->args, indent, to);
}

// Destroy a call expression.
void c_ast_expr_call_destroy(c_ast_expr_call_t *ast) {
    c_ast_expr_destroy(ast->func);
    c_ast_exprs_destroy(ast->args);
    lilycc_free(ast);
}


// Construct an identifier expression at position `pos`. Takes ownership of `name`.
c_ast_expr_ident_t *c_ast_expr_ident_create(pos_t pos, char *name) {
    c_ast_expr_ident_t *ast = lilycc_malloc(sizeof(c_ast_expr_ident_t));
    ast->pos                = pos;
    ast->name               = name;
    return ast;
}

// Print an identifier expression.
void c_ast_expr_ident_print(c_ast_expr_ident_t const *ast, int indent, FILE *to) {
    indent++;
    fputs("expr_ident\n", to);

    pindent(indent, to);
    fprintf(to, "name: %s\n", ast->name);
}

// Destroy an identifier expression.
void c_ast_expr_ident_destroy(c_ast_expr_ident_t *ast) {
    lilycc_free(ast->name);
    lilycc_free(ast);
}


// Construct a numeric constant expression at position `pos`.
c_ast_expr_iconst_t *c_ast_expr_iconst_create(pos_t pos, c_prim_t prim, ir_const_t value) {
    c_ast_expr_iconst_t *ast = lilycc_malloc(sizeof(c_ast_expr_iconst_t));
    ast->pos                 = pos;
    ast->prim                = prim;
    ast->value               = value;
    return ast;
}

// Print a numeric constant expression.
void c_ast_expr_iconst_print(c_ast_expr_iconst_t const *ast, int indent, FILE *to) {
    indent++;
    fputs("expr_iconst\n", to);

    pindent(indent, to);
    fprintf(to, "prim: %s\n", c_prim_name[ast->prim]);

    pindent(indent, to);
    fputs("value: ", to);
    ir_const_serialize(ast->value, to);
    fputc('\n', to);
}

// Destroy a numeric constant expression.
void c_ast_expr_iconst_destroy(c_ast_expr_iconst_t *ast) {
    lilycc_free(ast);
}


// Construct a string constant expression at position `pos`. Takes ownership of `value`.
c_ast_expr_sconst_t *c_ast_expr_sconst_create(pos_t pos, vec_char_t value) {
    c_ast_expr_sconst_t *ast = lilycc_malloc(sizeof(c_ast_expr_sconst_t));
    ast->pos                 = pos;
    ast->value               = value;
    return ast;
}

// Print a string constant expression.
void c_ast_expr_sconst_print(c_ast_expr_sconst_t const *ast, int indent, FILE *to) {
    indent++;
    fputs("expr_sconst\n", to);

    pindent(indent, to);
    fprintf(to, "value: \"%.*s\"\n", (int)ast->value.len, ast->value.arr);

    pindent(indent, to);
    fprintf(to, "length: %zu\n", ast->value.len);
}

// Destroy a string constant expression.
void c_ast_expr_sconst_destroy(c_ast_expr_sconst_t *ast) {
    vec_clear(&ast->value);
    lilycc_free(ast);
}


// Construct an expression list at position `pos`. Takes ownership of `exprs`.
c_ast_exprs_t *c_ast_exprs_create(pos_t pos, vec_c_ast_expr_t exprs) {
    c_ast_exprs_t *ast = lilycc_malloc(sizeof(c_ast_exprs_t));
    ast->pos           = pos;
    ast->exprs         = exprs;
    return ast;
}

// Print an expression list.
void c_ast_exprs_print(c_ast_exprs_t const *ast, int indent, FILE *to) {
    indent++;
    fputs("exprs\n", to);
    for (size_t i = 0; i < ast->exprs.len; i++) {
        pindent(indent, to);
        fprintf(to, "[%zu]: ", i);
        c_ast_expr_print(ast->exprs.arr[i], indent, to);
    }
}

// Destroy an expression list.
void c_ast_exprs_destroy(c_ast_exprs_t *ast) {
    for (size_t i = 0; i < ast->exprs.len; i++) {
        c_ast_expr_destroy(ast->exprs.arr[i]);
    }
    vec_clear(&ast->exprs);
    lilycc_free(ast);
}


// Wrap a leaf node in a `c_ast_expr_t`; the position is inherited from the leaf.
// `variant` must point to the matching leaf type for `tag`, and ownership is transferred.
c_ast_expr_t *c_ast_expr_create(c_ast_expr_tag_t tag, void *variant) {
    c_ast_expr_t *ast = lilycc_malloc(sizeof(c_ast_expr_t));
    ast->pos          = *(pos_t *)variant;
    ast->tag          = tag;
    switch (tag) {
        case C_AST_TAG_EXPR_INFIX: ast->expr_infix = variant; break;
        case C_AST_TAG_EXPR_PREFIX: ast->expr_prefix = variant; break;
        case C_AST_TAG_EXPR_SUFFIX: ast->expr_suffix = variant; break;
        case C_AST_TAG_EXPR_CAST: ast->expr_cast = variant; break;
        case C_AST_TAG_EXPR_CALL: ast->expr_call = variant; break;
        case C_AST_TAG_EXPR_IDENT: ast->expr_ident = variant; break;
        case C_AST_TAG_EXPR_ICONST: ast->expr_iconst = variant; break;
        case C_AST_TAG_EXPR_SCONST: ast->expr_sconst = variant; break;
        case C_AST_TAG_EXPRS: ast->exprs = variant; break;
    }
    return ast;
}

// Print an expression.
void c_ast_expr_print(c_ast_expr_t const *ast, int indent, FILE *to) {
    switch (ast->tag) {
        case C_AST_TAG_EXPR_INFIX: c_ast_expr_infix_print(ast->expr_infix, indent, to); break;
        case C_AST_TAG_EXPR_PREFIX: c_ast_expr_prefix_print(ast->expr_prefix, indent, to); break;
        case C_AST_TAG_EXPR_SUFFIX: c_ast_expr_suffix_print(ast->expr_suffix, indent, to); break;
        case C_AST_TAG_EXPR_CAST: c_ast_expr_cast_print(ast->expr_cast, indent, to); break;
        case C_AST_TAG_EXPR_CALL: c_ast_expr_call_print(ast->expr_call, indent, to); break;
        case C_AST_TAG_EXPR_IDENT: c_ast_expr_ident_print(ast->expr_ident, indent, to); break;
        case C_AST_TAG_EXPR_ICONST: c_ast_expr_iconst_print(ast->expr_iconst, indent, to); break;
        case C_AST_TAG_EXPR_SCONST: c_ast_expr_sconst_print(ast->expr_sconst, indent, to); break;
        case C_AST_TAG_EXPRS: c_ast_exprs_print(ast->exprs, indent, to); break;
    }
}

// Destroy an expression.
void c_ast_expr_destroy(c_ast_expr_t *ast) {
    switch (ast->tag) {
        case C_AST_TAG_EXPR_INFIX: c_ast_expr_infix_destroy(ast->expr_infix); break;
        case C_AST_TAG_EXPR_PREFIX: c_ast_expr_prefix_destroy(ast->expr_prefix); break;
        case C_AST_TAG_EXPR_SUFFIX: c_ast_expr_suffix_destroy(ast->expr_suffix); break;
        case C_AST_TAG_EXPR_CAST: c_ast_expr_cast_destroy(ast->expr_cast); break;
        case C_AST_TAG_EXPR_CALL: c_ast_expr_call_destroy(ast->expr_call); break;
        case C_AST_TAG_EXPR_IDENT: c_ast_expr_ident_destroy(ast->expr_ident); break;
        case C_AST_TAG_EXPR_ICONST: c_ast_expr_iconst_destroy(ast->expr_iconst); break;
        case C_AST_TAG_EXPR_SCONST: c_ast_expr_sconst_destroy(ast->expr_sconst); break;
        case C_AST_TAG_EXPRS: c_ast_exprs_destroy(ast->exprs); break;
    }
    lilycc_free(ast);
}
