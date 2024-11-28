
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#pragma once

#include "list.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>



// Diagnostic message severity level.
typedef enum {
    DIAG_HINT,
    DIAG_INFO,
    DIAG_WARN,
    DIAG_ERR,
} diag_lvl_t;

// Token type.
typedef enum {
    // Keywords.
    TOKENTYPE_KEYWORD,
    // Identifier (variable/label/function/etc name).
    TOKENTYPE_IDENT,
    // Integer constant.
    TOKENTYPE_ICONST,
    // Character constant.
    TOKENTYPE_CCONST,
    // String constant.
    TOKENTYPE_SCONST,
    // Uncategorized legal characters (operators/brackets/etc).
    TOKENTYPE_OTHER,
    // Garbage (illegal characters).
    TOKENTYPE_GARBAGE,
    // End of line (for languages where that is important).
    TOKENTYPE_EOL,
    // End of file.
    TOKENTYPE_EOF,
    // AST node.
    TOKENTYPE_AST,
} tokentype_t;

#ifndef NDEBUG
// Enum names of `tokentype_t` values.
extern char const *const tokentype_names[];
#endif


// Source file.
typedef struct srcfile    srcfile_t;
// Position in a source file.
typedef struct pos        pos_t;
// Include file instance.
typedef struct incfile    incfile_t;
// Compilation context.
typedef struct cctx       cctx_t;
// Diagnostic message.
typedef struct diagnostic diagnostic_t;
// Token data.
typedef struct token      token_t;


// Source file.
struct srcfile {
    // Associated frontend context.
    cctx_t  *ctx;
    // File path.
    char    *path;
    // File name; view of name in file path.
    char    *name;
    // Is this stored in RAM (as opposed to on disk)?
    bool     is_ram_file;
    // Current offset of file descriptor (for file on disk).
    int      fd_off;
    // File descriptor (for files on disk).
    FILE    *fd;
    // File content size (for files in RAM).
    int      content_len;
    // File content pointer (for files in RAM).
    uint8_t *content;
};

// Position in a source file.
struct pos {
    // Source file from whence this came.
    srcfile_t *srcfile;
    // Include file from whence this came, if any.
    incfile_t *incfile;
    // Zero-indexed line.
    int        line;
    // Zero-indexed column.
    int        col;
    // File offset in bytes.
    int        off;
    // Length in bytes.
    int        len;
};

// Include file instance.
struct incfile {
    // Associated source file.
    srcfile_t *srcfile;
    // Position of whatever directive caused the inclusion.
    pos_t      inc_from;
};

// Compilation context.
struct cctx {
    // Number of open source files.
    size_t      srcs_len;
    // Capacity for open source files.
    size_t      srcs_cap;
    // Array of open source files.
    srcfile_t **srcs;
    // Linked list of diagnostics.
    dlist_t     diagnostics;
};

// Diagnostic message.
struct diagnostic {
    // Linked list node.
    dlist_node_t node;
    // Location of the message.
    pos_t        pos;
    // Diagnostic message severity.
    diag_lvl_t   lvl;
    // Diagnostic message.
    char        *msg;
};

// Token data.
struct token {
    // Token position.
    pos_t       pos;
    // Token type.
    tokentype_t type;
    // Language-specific subtype.
    int         subtype;
    // Identifier or string constant value.
    char       *strval;
    // Length of string constant value.
    size_t      strval_len;
    // Integer constant value.
    uint64_t    ival;
    // Number of parameters for AST node.
    size_t      params_len;
    // Parameters for AST node.
    token_t    *params;
};



// Get position from start to end (exclusive).
pos_t pos_between(pos_t start, pos_t end);

// Create new compiler context.
cctx_t       *cctx_create();
// Delete compiler context (and therefor all compiler resources).
void          cctx_delete(cctx_t *ctx);
// Create a formatted diagnostic message.
// Returns diagnostic created, or NULL on failure.
diagnostic_t *cctx_diagnostic(cctx_t *ctx, pos_t pos, diag_lvl_t lvl, char const *fmt, ...)
    __attribute__((format(printf, 4, 5)));
// Print a diagnostic.
void print_diagnostic(diagnostic_t const *diag);

// Open or get a source file from compiler context.
srcfile_t *srcfile_open(cctx_t *ctx, char const *path);
// Create a source file from binary data.
srcfile_t *srcfile_create(cctx_t *ctx, char const *virt_path, void const *data, size_t len);
// Read a character from a source file and update position.
int        srcfile_getc(srcfile_t *file, pos_t *pos);

// Create an AST token with a fixed number of param tokens.
token_t ast_from_va(int subtype, size_t n_param, ...);
// Create an AST token with a fixed number of param tokens.
token_t ast_from(int subtype, size_t n_param, token_t *params);

// Append one parameter to an AST node.
void ast_append_param(token_t *ast, token_t param);
// Append one or more parameters to an AST node.
void ast_append_params_va(token_t *ast, size_t n_param, ...);
// Append one or more parameters to an AST node.
void ast_append_params(token_t *ast, size_t n_param, token_t *params);


// Set an AST node's strval.
static inline token_t ast_with_strval(token_t ast, char *strval) {
    if (ast.strval) {
        free(ast.strval);
    }
    ast.strval = strval;
    return ast;
}

// Set an AST node's ival.
static inline token_t ast_with_ival(token_t ast, uint64_t ival) {
    ast.ival = ival;
    return ast;
}

// Set a token's pos.
static inline token_t tkn_with_pos(token_t tkn, pos_t pos) {
    tkn.pos = pos;
    return tkn;
}
