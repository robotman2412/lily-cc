
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#pragma once

#include "frontend.h"
#include "list.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>



// Diagnostic message severity level.
typedef enum {
    DIAG_HINT,
    DIAG_INFO,
    DIAG_WARN,
    DIAG_ERR,
} diag_lvl_t;


// Source file.
typedef struct srcfile    srcfile_t;
// Position in a source file.
typedef struct pos        pos_t;
// Include file instance.
typedef struct incfile    incfile_t;
// Frontend context.
typedef struct front_ctx  front_ctx_t;
// Diagnostic message.
typedef struct diagnostic diagnostic_t;


// Source file.
struct srcfile {
    // Associated frontend context.
    front_ctx_t *ctx;
    // File path.
    char        *path;
    // File name; view of name in file path.
    char        *name;
    // Is this stored in RAM (as opposed to on disk)?
    bool         is_ram_file;
    // Current offset of file descriptor (for file on disk).
    int          fd_off;
    // File descriptor (for files on disk).
    FILE        *fd;
    // File content size (for files in RAM).
    int          content_len;
    // File content pointer (for files in RAM).
    uint8_t     *content;
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

// Frontend context.
struct front_ctx {
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



// Get position from start to end (exclusive).
pos_t pos_between(pos_t start, pos_t end);

// Create new frontend context.
front_ctx_t  *front_create();
// Delete frontend context (and therefor all frondend resources).
void          front_delete(front_ctx_t *ctx);
// Create a formatted diagnostic message.
// Returns diagnostic created, or NULL on failure.
diagnostic_t *front_diagnostic(front_ctx_t *ctx, pos_t pos, diag_lvl_t lvl, char const *fmt, ...)
    __attribute__((format(printf, 4, 5)));

// Open or get a source file from frontend context.
srcfile_t *srcfile_open(front_ctx_t *ctx, char const *path);
// Create a source file from binary data.
srcfile_t *srcfile_create(front_ctx_t *ctx, char const *virt_path, void const *data, size_t len);
// Read a character from a source file and update position.
int        srcfile_getc(srcfile_t *file, pos_t *pos);
