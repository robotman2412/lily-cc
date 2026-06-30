
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "c_options.h"
#include "c_tokenizer.h"
#include "compiler.h"
#include "map.h"
#include "set.h"
#include "tokenizer.h"



// Type of C macro substitution.
typedef enum __attribute__((packed)) {
    // Plain token.
    C_SUBST_TOKEN,
    // Function-like macro argument.
    C_SUBST_ARG,
    // The value of `__VA_ARGS__`.
    C_SUBST_VA_ARGS,
    // The tokens of `__VA_OPT__`.
    C_SUBST_VA_OPT,
} c_subst_type_t;

// C compiler context.
typedef struct c_compiler       c_compiler_t;
// State shared between a root preprocessor and any nested expansion contexts.
typedef struct c_preproc_shared c_preproc_shared_t;
// C preprocessor state.
typedef struct c_preproc        c_preproc_t;
// Include-file stack entry.
typedef struct c_incfile        c_incfile_t;
// If-directive stack entry.
typedef struct c_ifdir          c_ifdir_t;
// A macro definition.
typedef struct c_macro          c_macro_t;
// A single substitution position within a regular macro's body.
typedef struct c_macro_subst    c_macro_subst_t;
// A single collected argument to a function-like macro.
typedef struct c_macro_arg      c_macro_arg_t;
// Expanded macro value.
typedef struct c_expansion      c_expansion_t;

VEC_TYPE_DEF(vec_incfile_t, c_incfile_t)
VEC_TYPE_DEF(vec_ifdir_t, c_ifdir_t)
VEC_TYPE_DEF(vec_macro_subst_t, c_macro_subst_t)
VEC_TYPE_DEF(vec_macro_arg_t, c_macro_arg_t)
VEC_TYPE_DEF(vec_expansion_t, c_expansion_t)

// Procedural macro callback.
typedef c_expansion_t (*c_proc_macro_cb_t)(c_preproc_t *pre, vec_macro_arg_t const *args, void *cookie);

// State shared between a root preprocessor and any nested expansion contexts.
// Nested contexts (used for recursive argument expansion) hold a pointer to
// the same `c_preproc_shared_t` as the root, so macro definitions, pragma
// state, and the diagnostics sink stay consistent across all expansions.
struct c_preproc_shared {
    // Parent compiler context.
    cctx_t            *cctx;
    // Pointer to active C options.
    c_options_t const *options;
    // Macro definitions by name.
    // Map of `char *` -> `c_macro_t *`.
    map_t              macros;
    // Set of files which have already executed a `#pragma once`.
    set_t              once_files;
    // Next value for `__COUNTER__`.
    uint64_t           counter_macro;
};

// C preprocessor state.
struct c_preproc {
    // Base tokenizer.
    tokenizer_t         base;
    // Pointer to the root preprocessor (may be pointer to self).
    c_preproc_t        *root;
    // State shared with the root preprocessor.
    c_preproc_shared_t *shared;
    // Whether this preprocessor owns `shared` and should free it on destroy.
    // True for the root preprocessor; false for nested expansion contexts.
    bool                owns_shared;
    // Queue of tokens to emit from macro expansions.
    vec_expansion_t     expand;
    // Include-file tokenizer stack, bottom is the original file.
    vec_incfile_t       stack;
    // Whether the current line has non-whitespace tokens on it.
    bool                blank_line;
    // Do not convert tokens to C tokens before emitting them, and do not remove whitespace tokens.
    bool                raw_mode;
    // Keep comments instead of replacing them with a single space each.
    bool                keep_comments;
    // Disable processing of directives.
    bool                no_directives;
};

// Include-file stack entry.
struct c_incfile {
    // Associated tokenizer.
    tokenizer_t *tkn_ctx;
    // Active if/ifdef/ifndef directives.
    vec_ifdir_t  ifdir;
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
    // Uses a callback instead of subsitution tokens and args.
    bool is_proc_macro;
    // Is a built-in macro (that shouldn't be undefined).
    bool is_builtin;
    // Is a function-like macro.
    bool uses_args;
    union {
        struct {
            // Variadic macros (with ...).
            bool              is_variadic;
            // Argument names.
            vec_cstr_t        args;
            // Substitution positions in the body.
            vec_macro_subst_t subst;
        } regular;
        struct {
            // Callback to run on invocation.
            c_proc_macro_cb_t callback;
            // Cookie provided to the callback.
            void             *cookie;
        } proc;
    };
};

// A single collected argument to a function-like macro.
struct c_macro_arg {
    // Position of this argument (may be 0-length).
    pos_t       pos;
    // Tokens making up this argument.
    vec_token_t tokens;
    // Per-token: was there whitespace (or newlines) before this token in the
    // original argument source? The first token's entry is always false.
    vec_bool_t  ws_before;
    // Lazily-computed stringized version.
    char       *stringized;
    // Lazily-computed macro-expanded version.
    vec_token_t expanded;
};

// A single substitution position within a regular macro's body.
struct c_macro_subst {
    // Type of substitution.
    c_subst_type_t type;
    // For argument substitutions: stringize the argument with `#`.
    bool           stringize;
    // For argument substitutions: is surrounded by `##` on one or both sides.
    bool           pasting;
    union {
        // Literal token to emit (`C_SUBST_TOKEN`).
        token_t       token;
        // Index into the macro argument list (`C_SUBST_ARG` and `C_SUBST_STRINGIZE`).
        size_t        arg_index;
        // Synthetic arg for `__VA_OPT__` value.
        c_macro_arg_t va_opt;
    };
};

// Expanded macro value.
struct c_expansion {
    // Source macro; as the return value of a procedural macro, this field is ignored.
    c_macro_t const *macro;
    // Number of tokens already expanded.
    size_t           index;
    // Tokens to expand.
    vec_token_t      tokens;
};



// Create a preprocessor for a certain file.
// See `c_preproc_t` for details about `raw_mode` and `keep_comments`.
// Applying either flag after creation of the preprocessor will create incorrect output.
c_preproc_t *c_preproc_create(srcfile_t *srcfile, c_options_t const *options, bool raw_mode, bool keep_comments);
// Create a nested preprocessor that shares macro/file/pragma state with `parent`.
// Used for recursively expanding a function-like macro's arguments before
// they are substituted. The caller is responsible for feeding input tokens
// into the returned context (via its `expand` queue or include stack) and
// for destroying it with `tkn_ctx_delete(&nested->base)` once done.
c_preproc_t *c_preproc_create_nested(c_preproc_t *parent);
// Get the next token from the preprocessor.
token_t      c_preproc_next(tokenizer_t *tkn_ctx);
// Convert a preprocessor token to a C token.
token_t      c_preproc_tkn_to_c_tkn(c_preproc_t *pre, token_t tkn);
// Add a command-line or predefined macro.
void         c_preproc_add_macro(c_preproc_t *pre, char const *name, c_macro_t *macro);
// Add an include/embed search path.
void         c_preproc_add_path(c_preproc_t *pre, char const *path, bool is_sysinc);

// Create a regular macro by parsing it from a string.
// On success, `*name_out` is set to a heap-allocated copy of the parsed macro
// name (caller takes ownership). On failure, prints diagnostics to stdout,
// returns NULL, and leaves `*name_out` unchanged.
c_macro_t *c_macro_create(c_options_t const *options, char const *virt_file, char const *spec, char **name_out);
// Create a procedural macro.
c_macro_t *c_proc_macro_create(bool uses_args, c_proc_macro_cb_t callback, void *cookie);
// Destroy a macro.
void       c_macro_destroy(c_macro_t *macro);
