
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_ast.h"

#include "arith128.h"
#include "c_tokenizer.h"
#include "c_types.h"
#include "char_repr.h"
#include "lilycc_malloc.h"

#include <stdio.h>


// Struct constructor functions.
#define C_AST_META_STRUCT_DEF(name)                                                                                    \
    C_AST_STRUCT_DEF_##name(C_AST_IMPL_FUNCSIG, C_AST_IMPL_FUNCSIG_FIELD, C_AST_IMPL_FUNCSIG_CHILD)                    \
        C_AST_STRUCT_DEF_##name(C_AST_IMPL_FUNCBODY, C_AST_IMPL_FUNCBODY_FIELD, C_AST_IMPL_FUNCBODY_CHILD)
#define C_AST_IMPL_FUNCSIG(name, ...)                                                                                  \
    /* Construct a struct AST node. */                                                                                 \
    c_ast_##name##_t *c_ast_##name##_create(pos_t pos __VA_ARGS__)
#define C_AST_IMPL_FUNCSIG_FIELD(parent, type, name) , type name
#define C_AST_IMPL_FUNCSIG_CHILD(parent, type, name) C_AST_IMPL_FUNCSIG_FIELD(parent, c_ast_##type##_t *, name)
#define C_AST_IMPL_FUNCBODY(name, ...)                                                                                 \
    {                                                                                                                  \
        c_ast_##name##_t *ast = lilycc_malloc(sizeof(c_ast_##name##_t));                                               \
        ast->pos              = pos;                                                                                   \
        __VA_ARGS__                                                                                                    \
        return ast;                                                                                                    \
    }
#define C_AST_IMPL_FUNCBODY_FIELD(parent, type, name) ast->name = name;
#define C_AST_IMPL_FUNCBODY_CHILD(parent, type, name) C_AST_IMPL_FUNCBODY_FIELD(parent, c_ast_##type##_t *, name)
#include "c_ast.inc"

// Union constructor functions.
#define C_AST_UNION_DEF(name, ...) __VA_ARGS__
#define C_AST_UNION_FIELD(parent, type, name, union_tag)                                                               \
    /* Construct a union AST node. */                                                                                  \
    c_ast_##parent##_t *c_ast_##parent##_create_##name(pos_t pos, type parent##_##name) {                              \
        c_ast_##parent##_t *ast = lilycc_malloc(sizeof(c_ast_##parent##_t));                                           \
        ast->pos                = pos;                                                                                 \
        ast->tag                = C_AST_TAG_##union_tag;                                                               \
        ast->parent##_##name    = parent##_##name;                                                                     \
        return ast;                                                                                                    \
    }
// Union constructor functions.
#define C_AST_UNION_CHILD(parent, type, name, union_tag)                                                               \
    /* Construct a union AST node. */                                                                                  \
    c_ast_##parent##_t *c_ast_##parent##_create_##name(c_ast_##type##_t *parent##_##name) {                            \
        c_ast_##parent##_t *ast = lilycc_malloc(sizeof(c_ast_##parent##_t));                                           \
        ast->pos                = parent##_##name->pos;                                                                \
        ast->tag                = C_AST_TAG_##union_tag;                                                               \
        ast->parent##_##name    = parent##_##name;                                                                     \
        return ast;                                                                                                    \
    }
#include "c_ast.inc"

// List constructor functions.
#define C_AST_LIST_DEF(name)                                                                                           \
    /* Construct a list AST node. */                                                                                   \
    c_ast_##name##_list_t *c_ast_##name##_list_create(pos_t pos, vec_c_ast_##name##_t items) {                         \
        c_ast_##name##_list_t *ast = lilycc_malloc(sizeof(c_ast_##name##_list_t));                                     \
        ast->pos                   = pos;                                                                              \
        ast->items                 = items;                                                                            \
        return ast;                                                                                                    \
    }
#include "c_ast.inc"



// Indentation printing helper.
static void pindent(FILE *to, int indent) {
    while (indent) {
        fputs("  ", to);
        indent--;
    }
}

// String printing helper.
static void pc_ast_cstr_t(c_ast_cstr_t const *value, FILE *to, int indent) {
    (void)indent;
    fputs(*value, to);
    fputc('\n', to);
}

// C string constant printing helper.
static void pvec_char_t(vec_char_t const *value, FILE *to, int indent) {
    (void)indent;
    fputc('\"', to);
    print_cstr_repr(value->arr, value->len, to);
    fputc('\"', to);
    fputc('\n', to);
}

// Position printing helper.
static void ppos_t(pos_t const *pos, FILE *to, int indent) {
    (void)indent;
    if (pos->srcfile) {
        fputs(pos->srcfile->path, to);
    } else {
        fputs("???", to);
    }
    fprintf(to, ":%d:%d (%lld bytes)\n", pos->line + 1, pos->col + 1, (long long)pos->len);
}

// C token type printing helper.
static void pc_tokentype_t(c_tokentype_t const *value, FILE *to, int indent) {
    (void)indent;
    fputs(c_token_id[*value], to);
    fputc('\n', to);
}

// C primitive type printing helper.
static void pc_prim_t(c_prim_t const *value, FILE *to, int indent) {
    (void)indent;
    fputs(c_prim_name[*value], to);
    fputc('\n', to);
}

// C primitive type printing helper.
static void pc_keyw_t(c_keyw_t const *value, FILE *to, int indent) {
    (void)indent;
    fputs(c_keyw_name[*value], to);
    fputc('\n', to);
}

// Bool printing helper.
static void pbool(bool const *value, FILE *to, int indent) {
    (void)indent;
    fputs(*value ? "true\n" : "false\n", to);
}

// IR constant printing helper.
static void pi128_t(i128_t const *value, FILE *to, int indent) {
    (void)indent;
    char buf[40];
    itoa128(*value, 0, buf);
    fputs(buf, to);
    fputc('\n', to);
}

// Struct printing functions.
#define C_AST_STRUCT_DEF(name, ...)                                                                                    \
    /* Debug-print the given AST node. */                                                                              \
    void c_ast_##name##_print(c_ast_##name##_t const *ast, FILE *to, int indent) {                                     \
        indent++;                                                                                                      \
        fputs(#name "\n", to);                                                                                         \
        pindent(to, indent);                                                                                           \
        fputs("pos: ", to);                                                                                            \
        ppos_t(&ast->pos, to, indent);                                                                                 \
        __VA_ARGS__                                                                                                    \
    }
#define C_AST_STRUCT_FIELD(parent, type, name)                                                                         \
    pindent(to, indent);                                                                                               \
    fputs(#name ": ", to);                                                                                             \
    p##type(&ast->name, to, indent);
#define C_AST_STRUCT_CHILD(parent, type, name)                                                                         \
    if (ast->name) {                                                                                                   \
        pindent(to, indent);                                                                                           \
        fputs(#name ": ", to);                                                                                         \
        c_ast_##type##_print(ast->name, to, indent);                                                                   \
    }
#include "c_ast.inc"

// Union printing helpers.
#define C_AST_UNION_DEF(name, ...)                                                                                     \
    /* Debug-print the given AST node. */                                                                              \
    void c_ast_##name##_print(c_ast_##name##_t const *ast, FILE *to, int indent) {                                     \
        fputs(#name, to);                                                                                              \
        switch (ast->tag) { __VA_ARGS__ }                                                                              \
    }
#define C_AST_UNION_FIELD(parent, type, name, union_tag)                                                               \
    case C_AST_TAG_##union_tag:                                                                                        \
        indent++;                                                                                                      \
        fputc('\n', to);                                                                                               \
        pindent(to, indent);                                                                                           \
        fputs("pos: ", to);                                                                                            \
        ppos_t(&ast->pos, to, indent);                                                                                 \
        pindent(to, indent);                                                                                           \
        fputs(#parent "_" #name ": ", to);                                                                             \
        p##type(&ast->parent##_##name, to, indent);                                                                    \
        break;
#define C_AST_UNION_CHILD(parent, type, name, union_tag)                                                               \
    case C_AST_TAG_##union_tag:                                                                                        \
        fputc(' ', to);                                                                                                \
        c_ast_##type##_print(ast->parent##_##name, to, indent);                                                        \
        break;
#include "c_ast.inc"

// List printing helpers.
#define C_AST_LIST_DEF(name)                                                                                           \
    /* Debug-print the given AST node. */                                                                              \
    void c_ast_##name##_list_print(c_ast_##name##_list_t const *ast, FILE *to, int indent) {                           \
        indent++;                                                                                                      \
        fputs(#name "_list\n", to);                                                                                    \
        pindent(to, indent);                                                                                           \
        fputs("pos: ", to);                                                                                            \
        ppos_t(&ast->pos, to, indent);                                                                                 \
        for (size_t i = 0; i < ast->items.len; i++) {                                                                  \
            pindent(to, indent);                                                                                       \
            fprintf(to, "%zu: ", i);                                                                                   \
            c_ast_##name##_print(ast->items.arr[i], to, indent);                                                       \
        }                                                                                                              \
    }
#include "c_ast.inc"



// String deletion helper.
static void delc_ast_cstr_t(c_ast_cstr_t value) {
    lilycc_free(value);
}

// C string constant deletion helper.
static void delvec_char_t(vec_char_t value) {
    vec_clear(&value);
}

// Position deletion helper.
static void delpos_t(pos_t value) {
    (void)value;
}

// C token type deletion helper.
static void delc_tokentype_t(c_tokentype_t value) {
    (void)value;
}

// C primitive type deletion helper.
static void delc_prim_t(c_prim_t value) {
    (void)value;
}

// C primitive type deletion helper.
static void delc_keyw_t(c_keyw_t value) {
    (void)value;
}

// Bool deletion helper.
static void delbool(bool value) {
    (void)value;
}

// IR constant deletion helper.
static void deli128_t(i128_t value) {
    (void)value;
}

// Struct deletion functions.
#define C_AST_STRUCT_DEF(name, ...)                                                                                    \
    /* Destroy an AST node. */                                                                                         \
    void c_ast_##name##_delete(c_ast_##name##_t *ast) {                                                                \
        __VA_ARGS__                                                                                                    \
        lilycc_free(ast);                                                                                              \
    }
#define C_AST_STRUCT_FIELD(parent, type, name) del##type(ast->name);
#define C_AST_STRUCT_CHILD(parent, type, name)                                                                         \
    if (ast->name) {                                                                                                   \
        c_ast_##type##_delete(ast->name);                                                                              \
    }
#include "c_ast.inc"

// Union deletion functions.
#define C_AST_UNION_DEF(name, ...)                                                                                     \
    /* Destroy an AST node. */                                                                                         \
    void c_ast_##name##_delete(c_ast_##name##_t *ast) {                                                                \
        switch (ast->tag) { __VA_ARGS__ }                                                                              \
        lilycc_free(ast);                                                                                              \
    }
#define C_AST_UNION_FIELD(parent, type, name, union_tag)                                                               \
    case C_AST_TAG_##union_tag: del##type(ast->parent##_##name); break;
#define C_AST_UNION_CHILD(parent, type, name, union_tag)                                                               \
    case C_AST_TAG_##union_tag: c_ast_##type##_delete(ast->parent##_##name); break;
#include "c_ast.inc"

// List deletion functions.
#define C_AST_LIST_DEF(name)                                                                                           \
    /* Destroy an AST node. */                                                                                         \
    void c_ast_##name##_list_delete(c_ast_##name##_list_t *ast) {                                                      \
        for (size_t i = 0; i < ast->items.len; i++) {                                                                  \
            c_ast_##name##_delete(ast->items.arr[i]);                                                                  \
        }                                                                                                              \
        vec_clear(&ast->items);                                                                                        \
        lilycc_free(ast);                                                                                              \
    }
#include "c_ast.inc"
