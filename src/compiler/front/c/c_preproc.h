
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "c_tokenizer.h"
#include "compiler.h"
#include "map.h"
#include "set.h"
#include "tokenizer.h"



// C compiler context.
typedef struct c_compiler c_compiler_t;

// C preprocessor state.
typedef struct c_preproc c_preproc_t;
// Include-file stack entry.
typedef struct c_incfile c_incfile_t;
// If-directive stack entry.
typedef struct c_ifdir   c_ifdir_t;
// A macro definition.
typedef struct c_macro   c_macro_t;



// C preprocessor state.
struct c_preproc {
    // Base tokenizer.
    tokenizer_t  base;
    // Parent compiler context.
    cctx_t      *cctx;
    // Macro definitions by name.
    // Map of `char *` -> `c_macro_t *`.
    map_t        macros;
    // How many tokens from `expand` have been used so far.
    size_t       expand_index;
    // Queue of tokens to emit from macro expansions.
    size_t       expand_len, expand_cap;
    // Queue of tokens to emit from macro expansions.
    token_t     *expand;
    // Include-file tokenizer stack, bottom is the original file.
    size_t       stack_len, stack_cap;
    // Include-file tokenizer stack, bottom is the original file.
    c_incfile_t *stack;
    // All files in order of first opened.
    size_t       files_len, files_cap;
    // All files in order of first opened.
    srcfile_t   *files;
    // Set of files which have already executed a `#pragma once`.
    set_t        once_files;
    // Current C standard.
    int          c_std;
    // Whether the current line has non-whitespace tokens on it.
    bool         blank_line;
};

// Include-file stack entry.
struct c_incfile {
    // Associated tokenizer.
    c_tokenizer_t *tkn_ctx;
    // Active if/ifdef/ifndef directives.
    size_t         ifdir_len, ifdir_cap;
    // Active if/ifdef/ifndef directives.
    c_ifdir_t     *ifdir;
};

// If-directive stack entry.
struct c_ifdir {
    // If/else directive position.
    pos_t pos;
    // Emit tokens here.
    bool  do_emit;
    // Disabled by earlier successful branch.
    bool  disabled;
    // Allow another else clause.
    bool  allow_else;
};

// A macro definition.
struct c_macro {
    // Number of non-variadic arguments.
    size_t   args_len;
    // Argument names.
    char   **args;
    // Number of token to expand.
    size_t   tokens_len;
    // Tokens to expand.
    token_t *tokens;
    // Variadic macros (with ...).
    bool     variadic;
};



// Create a preprocessor for a certain file.
c_preproc_t *c_preproc_create(srcfile_t *srcfile, int c_std);
// Get the next token from the preprocessor.
token_t      c_preproc_next(tokenizer_t *tkn_ctx);
// Add a pre-defined macro.
void         c_preproc_predef_macro(c_preproc_t *pre, char const *name, char const *expansion);

// Destroy a macro.
void c_macro_destroy(c_macro_t *macro);
// Perform macro-expansion.
void c_macro_expand(c_preproc_t *pre, c_macro_t const *macro);
