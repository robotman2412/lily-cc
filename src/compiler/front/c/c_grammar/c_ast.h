
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once



#include "c_tokenizer.h"
#include "c_types1.h"
#include "compiler.h"
#include "vec.h"



// Needed a `char *` type but also for it to be one identifier.
typedef char *c_ast_cstr_t;



// Union tags.
#define C_AST_UNION_DEF(name, ...)                       typedef enum { __VA_ARGS__ } c_ast_##name##_tag_t;
#define C_AST_UNION_FIELD(parent, type, name, union_tag) C_AST_TAG_##union_tag,
#include "c_ast.inc"

// Forward declarations.
#define C_AST_DEF(name) typedef struct c_ast_##name c_ast_##name##_t;
#include "c_ast.inc"

// Vector definitions.
#define C_AST_LIST_DEF(name) VEC_TYPE_DEF(vec_c_ast_##name##_t, c_ast_##name##_t *)
#include "c_ast.inc"

// Struct declarations.
#define C_AST_STRUCT_DEF(name, ...)                                                                                    \
    struct c_ast_##name {                                                                                              \
        /* Combined AST node position. */                                                                              \
        pos_t pos;                                                                                                     \
        __VA_ARGS__                                                                                                    \
    };
#define C_AST_STRUCT_FIELD(parent, type, name) type name;
#include "c_ast.inc"

// Union declarations.
#define C_AST_UNION_DEF(name, ...)                                                                                     \
    struct c_ast_##name {                                                                                              \
        /* Combined AST node position. */                                                                              \
        pos_t                pos;                                                                                      \
        /* Union tag. */                                                                                               \
        c_ast_##name##_tag_t tag;                                                                                      \
        union {                                                                                                        \
            __VA_ARGS__                                                                                                \
        };                                                                                                             \
    };
#define C_AST_UNION_FIELD(parent, type, name, union_tag) type parent##_##name;
#include "c_ast.inc"

// List declarations.
#define C_AST_LIST_DEF(name)                                                                                           \
    struct c_ast_##name##_list {                                                                                       \
        /* Combined AST node position. */                                                                              \
        pos_t                pos;                                                                                      \
        vec_c_ast_##name##_t items;                                                                                    \
    };
#include "c_ast.inc"



// Struct constructor functions.
#define C_AST_STRUCT_DEF(name, ...)                                                                                    \
    /* Construct a struct AST node. */                                                                                 \
    c_ast_##name##_t *c_ast_##name##_create(pos_t pos __VA_ARGS__);
#define C_AST_STRUCT_FIELD(parent, type, name) , type name
#include "c_ast.inc"

// Union constructor functions.
#define C_AST_UNION_DEF(name, ...) __VA_ARGS__
#define C_AST_UNION_FIELD(parent, type, name, union_tag)                                                               \
    /* Construct a union AST node. */                                                                                  \
    c_ast_##parent##_t *c_ast_##parent##_create_##name(pos_t pos, type parent##_##name);
// Union constructor functions.
#define C_AST_UNION_CHILD(parent, type, name, union_tag)                                                               \
    /* Construct a union AST node. */                                                                                  \
    c_ast_##parent##_t *c_ast_##parent##_create_##name(c_ast_##type##_t *parent##_##name);
#include "c_ast.inc"

// List constructor functions.
#define C_AST_LIST_DEF(name)                                                                                           \
    /* Construct a list AST node. */                                                                                   \
    c_ast_##name##_list_t *c_ast_##name##_list_create(pos_t pos, vec_c_ast_##name##_t items);
#include "c_ast.inc"

// Common function declarations.
#define C_AST_DEF(name)                                                                                                \
    /* Debug-print the given AST node. */                                                                              \
    void c_ast_##name##_dbg(c_ast_##name##_t const *ast, int indent, FILE *to);                                        \
    /* Destroy an AST node. */                                                                                         \
    void c_ast_##name##_delete(c_ast_##name##_t *ast);
#include "c_ast.inc"
