
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_preproc.h"

#include "arith128.h"
#include "arrays.h"
#include "c_tokenizer.h"
#include "compiler.h"
#include "map.h"
#include "set.h"
#include "strong_malloc.h"
#include "tokenizer.h"

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>



// Mode that `c_preproc_get_tkn` uses.
// No-expand mode only causes no *new* expansions; it still reads out expanded tokens first.
typedef enum {
    // Expand macros, allow newline.
    NEXT_EXPAND,
    // Only file tokens, allow newline.
    NEXT_RAW,
    // Expand macros, current line.
    LINE_EXPAND,
    // Only file tokens, current line.
    LINE_RAW,
    // Expand macros, skip whitespace (even on peek), current line.
    LINE_NOWS_EXPAND,
    // Expand macros, skip whitespace (even on peek), current line.
    LINE_NOWS_RAW,
} next_mode_t;

static void    c_preproc_destroy(tokenizer_t *tkn);
static void    c_preproc_builtin_macros(c_preproc_t *preproc);
static void    c_preproc_pragma(c_preproc_t *pre, pos_t pos, char const *pragma);
static void    c_pragma_once(c_preproc_t *pre, pos_t pos, char const *args);
static void    c_incfile_push(c_preproc_t *pre, pos_t pos, char const *path);
static void    c_incfile_pop(c_preproc_t *pre);
static void    c_incfile_eof(c_preproc_t *pre);
static int     c_preproc_op_precedence(c_tokentype_t type);
static bool    c_preproc_is_prefix_op(c_tokentype_t type);
static i128_t  c_preproc_eval_prefix(c_tokentype_t oper, i128_t value);
static i128_t  c_preproc_eval_infix(bool is_signed, i128_t lhs, c_tokentype_t oper, i128_t rhs);
static token_t c_preproc_eval_get_helper(c_preproc_t *pre);
static bool    c_preproc_eval(c_preproc_t *pre, pos_t pos);
static void    c_directive_include(c_preproc_t *pre, pos_t pos);
static void    c_directive_pragma(c_preproc_t *pre, pos_t pos);
static void    c_directive_if(c_preproc_t *pre, pos_t pos, bool elif, bool ifdef, bool ifndef);
static void    c_directive_else(c_preproc_t *pre, pos_t pos);
static void    c_directive_endif(c_preproc_t *pre, pos_t pos);
static void    c_directive_warning(c_preproc_t *pre, pos_t pos, bool is_error);
static void    c_directive_define(c_preproc_t *pre, pos_t pos);
static void    c_directive_undef(c_preproc_t *pre, pos_t pos);
static char   *c_preproc_read_bytes(c_preproc_t *pre, pos_t *pos_out);
static void    c_preproc_until_eol(c_preproc_t *pre, bool warn_extra_tok);
static bool    c_preproc_do_emit(c_preproc_t *pre);
static token_t c_preproc_get_tkn(c_preproc_t *pre, next_mode_t mode);
static void    c_preproc_directive(c_preproc_t *pre);
static bool    c_preproc_tkn_paste(c_preproc_t *pre, token_t const *lhs, token_t const *rhs, token_t *out);
static char   *c_preproc_esc_str(char const *raw);
static token_t c_preproc_raw_peek(c_preproc_t *pre);
static token_t c_preproc_raw_next(c_preproc_t *pre);
static bool    c_macro_parse_body(c_macro_t *macro, tokenizer_t *tkn_ctx);
static void    c_macro_mark_pasting(c_macro_subst_t *tokens, size_t tokens_len);
static token_t c_macro_arg_stringize(c_macro_arg_t *arg, pos_t pos);
static void    c_macro_arg_preexpand(c_preproc_t *pre, c_macro_arg_t *arg);
static void    c_macro_expand(c_preproc_t *pre, pos_t pos, c_macro_t const *macro);



#pragma region builtins

// Implementation of `__COUNTER__`.
static c_expansion_t c_proc_macro_counter(c_preproc_t *pre, c_macro_arg_t const *args, size_t args_len, void *cookie) {
    (void)cookie;
    (void)args;
    (void)args_len;

    int cap = snprintf(NULL, 0, "%" PRIu64, pre->shared->counter_macro);
    if (cap < 0) {
        perror("snprintf");
        abort();
    }
    cap       += 1;
    char *buf  = malloc(cap);
    int   len  = snprintf(buf, cap, "%" PRIu64, pre->shared->counter_macro);
    pre->shared->counter_macro++;

    token_t tkn = {
        .pos        = {0},
        .type       = TOKENTYPE_IDENT,
        .subtype    = C_PPNUMBER,
        .strval     = buf,
        .strval_len = len,
    };
    c_expansion_t expand = {
        .tokens_len = 1,
        .tokens_cap = 1,
        .tokens     = strong_malloc(sizeof(token_t)),
    };
    *expand.tokens = tkn;

    return expand;
}

// Implementation of `__FILE__`.
static c_expansion_t c_proc_macro_file(c_preproc_t *pre, c_macro_arg_t const *args, size_t args_len, void *cookie) {
    (void)args;
    (void)args_len;
    (void)cookie;

    token_t peek = c_preproc_raw_peek(pre->root);
    char   *path;
    if (peek.pos.srcfile && peek.pos.srcfile->path) {
        path = c_preproc_esc_str(peek.pos.srcfile->path);
    } else {
        path = strong_strdup("<unknown>");
    }

    token_t tkn = {
        .pos        = {0},
        .type       = TOKENTYPE_SCONST,
        .subtype    = C_STR_RAW_DQUOT,
        .strval     = path,
        .strval_len = strlen(path),
    };
    c_expansion_t expand = {
        .tokens_len = 1,
        .tokens_cap = 1,
        .tokens     = strong_malloc(sizeof(token_t)),
    };
    *expand.tokens = tkn;

    return expand;
}

// Implementation of `__LINE__`.
static c_expansion_t c_proc_macro_line(c_preproc_t *pre, c_macro_arg_t const *args, size_t args_len, void *cookie) {
    (void)cookie;
    (void)args;
    (void)args_len;

    token_t peek = c_preproc_raw_peek(pre->root);

    int cap = snprintf(NULL, 0, "%d", peek.pos.line);
    if (cap < 0) {
        perror("snprintf");
        abort();
    }
    cap       += 1;
    char *buf  = malloc(cap);
    int   len  = snprintf(buf, cap, "%d", peek.pos.line);

    token_t tkn = {
        .pos        = {0},
        .type       = TOKENTYPE_IDENT,
        .subtype    = C_PPNUMBER,
        .strval     = buf,
        .strval_len = len,
    };
    c_expansion_t expand = {
        .tokens_len = 1,
        .tokens_cap = 1,
        .tokens     = strong_malloc(sizeof(token_t)),
    };
    *expand.tokens = tkn;

    return expand;
}

// Format-print a macro definition for `c_preproc_predef_macros`.
__attribute__((format(printf, 2, 3))) static void c_preproc_fmt_builtin_macro(c_preproc_t *pre, char const *fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    int cap = vsnprintf(NULL, 0, fmt, vl);
    va_end(vl);

    char *spec = malloc(cap + 1);
    va_start(vl, fmt);
    vsnprintf(spec, cap + 1, fmt, vl);
    va_end(vl);

    char      *name;
    c_macro_t *macro  = c_macro_create("<built-in>", spec, &name);
    macro->is_builtin = true;
    c_preproc_add_macro(pre, name, macro);
    free(name);
    free(spec);
}

// Create all (standard and extension) pre-defined macros for a given C frontend.
static void c_preproc_builtin_macros(c_preproc_t *pre) {
    time_t    ts       = time(NULL);
    struct tm datetime = *localtime(&ts);

    // __DATE__
    char const *month[] = {
        "Jan",
        "Feb",
        "Mar",
        "Apr",
        "May",
        "Jun",
        "Jul",
        "Aug",
        "Sep",
        "Oct",
        "Nov",
        "Dec",
    };
    c_preproc_fmt_builtin_macro(
        pre,
        "__DATE__=\"%s %02d %04d\"",
        month[datetime.tm_mon],
        datetime.tm_mday,
        datetime.tm_year + 1900
    );

    // __TIME__
    c_preproc_fmt_builtin_macro(pre, "__TIME__=\"%02d:%02d:%02d\"", datetime.tm_hour, datetime.tm_min, datetime.tm_sec);

    // __STDC__
    c_preproc_fmt_builtin_macro(pre, "__STDC__=1");

    // __STDC_VERSION__
    c_preproc_fmt_builtin_macro(pre, "__STDC_VERSION__=%dL", pre->shared->c_std);

    // __COUNTER__
    c_macro_t *counter_macro  = c_proc_macro_create(false, c_proc_macro_counter, NULL);
    counter_macro->is_builtin = true;
    c_preproc_add_macro(pre, "__COUNTER__", counter_macro);

    // __FILE__
    c_macro_t *file_macro  = c_proc_macro_create(false, c_proc_macro_file, NULL);
    file_macro->is_builtin = true;
    c_preproc_add_macro(pre, "__FILE__", file_macro);

    // __LINE__
    c_macro_t *line_macro  = c_proc_macro_create(false, c_proc_macro_line, NULL);
    line_macro->is_builtin = true;
    c_preproc_add_macro(pre, "__LINE__", line_macro);
}

#pragma endregion builtins

// Create a preprocessor for a certain file.
// See `c_preproc_t` for details about `raw_mode` and `keep_comments`.
// Applying either flag after creation of the preprocessor will create incorrect output.
c_preproc_t *c_preproc_create(srcfile_t *srcfile, int c_std, bool raw_mode, bool keep_comments) {
    c_preproc_t *pre = strong_calloc(1, sizeof(c_preproc_t));

    c_tokenizer_t *srctok = c_tkn_create(srcfile, c_std);
    if (!srctok) {
        free(pre);
        return NULL;
    }
    srctok->preproc_mode  = true;
    srctok->keep_comments = keep_comments;

    c_preproc_shared_t *shared = strong_calloc(1, sizeof(c_preproc_shared_t));
    shared->cctx               = srcfile->ctx;
    shared->macros             = STR_MAP_EMPTY;
    shared->once_files         = PTR_SET_EMPTY;
    shared->c_std              = c_std;

    // Note: `base` has a `pos` and `file`, but we do not use either.
    pre->base.next          = c_preproc_next;
    pre->base.cleanup       = c_preproc_destroy;
    pre->root               = pre;
    pre->shared             = shared;
    pre->owns_shared        = true;
    pre->stack_len          = 1;
    pre->stack_cap          = 1;
    pre->stack              = strong_malloc(sizeof(c_incfile_t));
    pre->stack[0].tkn_ctx   = &srctok->base;
    pre->stack[0].ifdir     = NULL;
    pre->stack[0].ifdir_len = 0;
    pre->stack[0].ifdir_cap = 0;
    pre->blank_line         = true;
    pre->raw_mode           = raw_mode;
    pre->keep_comments      = keep_comments;

    c_preproc_builtin_macros(pre);

    return pre;
}

// Create a nested preprocessor that shares macro/file/pragma state with `parent`.
c_preproc_t *c_preproc_create_nested(c_preproc_t *parent) {
    c_preproc_t *pre = strong_calloc(1, sizeof(c_preproc_t));

    pre->base.next     = c_preproc_next;
    pre->base.cleanup  = c_preproc_destroy;
    pre->shared        = parent->shared;
    pre->root          = parent->root;
    pre->owns_shared   = false;
    pre->blank_line    = true;
    pre->raw_mode      = true;
    pre->keep_comments = false;

    return pre;
}

// Destroy a preprocessor.
static void c_preproc_destroy(tokenizer_t *tkn) {
    c_preproc_t *pre = (c_preproc_t *)tkn;

    for (size_t i = 0; i < pre->expand_len; i++) {
        c_expansion_t *expand = &pre->expand[i];
        for (size_t x = expand->index; x < expand->tokens_len; x++) {
            tkn_delete(expand->tokens[x]);
        }
        free(expand->tokens);
    }
    free(pre->expand);

    while (pre->stack_len) {
        c_incfile_pop(pre);
    }
    free(pre->stack);

    if (pre->owns_shared) {
        map_foreach(ent, &pre->shared->macros) {
            c_macro_destroy(ent->value);
        }
        map_clear(&pre->shared->macros);
        // Note: The source files are kept alive by the associated cctx_t.
        free(pre->shared->files);
        free(pre->shared);
    }
}

#pragma region pragmas

// Pragma: once.
static void c_pragma_once(c_preproc_t *pre, pos_t pos, char const *args) {
    if (*args) {
        cctx_diagnostic(pre->shared->cctx, pos, DIAG_WARN, "Extra tokens after #pragma once");
    }
    c_incfile_t *file = &pre->stack[pre->stack_len - 1];
    set_add(&pre->shared->once_files, file->tkn_ctx->file);
}

// Handle a #pragma directive or _Pragma operator.
static void c_preproc_pragma(c_preproc_t *pre, pos_t pos, char const *pragma) {
    while (isspace((unsigned char)*pragma)) {
        pragma++;
    }
    if (!*pragma) {
        return;
    }
    size_t name_len = 0;
    while (pragma[name_len] && !isspace((unsigned char)pragma[name_len])) {
        name_len++;
    }
    char const *args = pragma + name_len;
    while (isspace((unsigned char)*args)) {
        args++;
    }

    if (name_len == 4 && !memcmp(pragma, "once", 4)) {
        c_pragma_once(pre, pos, args);
    } else if ((name_len == 6 && !memcmp(pragma, "region", 4)) || (name_len == 9 && !memcmp(pragma, "endregion", 4))) {
        // These pragmas are recognised but ignored.
    } else {
        cctx_diagnostic(pre->shared->cctx, pos, DIAG_WARN, "Unrecognized pragma: %.*s", (int)name_len, pragma);
    }
}

#pragma endregion pragmas

#pragma region directives

// Search for an include file and push it into the include stack.
static void c_incfile_push(c_preproc_t *pre, pos_t pos, char const *path) {
    // TODO: Replace with _popen when include search paths are implemented.
    srcfile_t *file = srcfile_open(pre->shared->cctx, path);
    if (!file) {
        cctx_diagnostic(pre->shared->cctx, pos, DIAG_ERR, "Cannot open include file: %s", path);
        return;
    }

    if (set_contains(&pre->shared->once_files, file)) {
        // A `#pragma once` for this file already occurred.
        return;
    }

    // TODO: Add include stack info to the tokenizer's position.
    c_tokenizer_t *tkn_ctx = c_tkn_create(file, pre->shared->c_std);
    tkn_ctx->preproc_mode  = true;
    c_incfile_t incfile    = {
        .tkn_ctx   = &tkn_ctx->base,
        .ifdir_cap = 0,
        .ifdir_len = 0,
        .ifdir     = 0,
    };
    array_lencap_insert_strong(
        &pre->stack,
        sizeof(c_incfile_t),
        &pre->stack_len,
        &pre->stack_cap,
        &incfile,
        pre->stack_len
    );
}

// Pop the top-most file off of the include stack.
static void c_incfile_pop(c_preproc_t *pre) {
    assert(pre->stack_len >= 1);
    c_incfile_t *incfile = &pre->stack[pre->stack_len - 1];

    c_incfile_eof(pre);

    tkn_ctx_delete(incfile->tkn_ctx);
    pre->stack_len--;
}

// Do end-of-file checks for top-most file of the include stack.
static void c_incfile_eof(c_preproc_t *pre) {
    assert(pre->stack_len >= 1);
    c_incfile_t *incfile = &pre->stack[pre->stack_len - 1];

    for (size_t i = 0; i < incfile->ifdir_len; i++) {
        cctx_diagnostic(
            pre->shared->cctx,
            incfile->ifdir[i].pos,
            DIAG_ERR,
            "Unterminated #%s directive",
            incfile->ifdir[i].allow_else ? "if" : "else"
        );
    }
    free(incfile->ifdir);
    incfile->ifdir     = NULL;
    incfile->ifdir_len = 0;
    incfile->ifdir_cap = 0;
}

// Get operator precedence.
// Returns -1 if not an operator token.
// Note: Unlike C proper, there are no suffix operators.
// In addition, prefix operators have higher precedence than all infix operators.
static int c_preproc_op_precedence(c_tokentype_t type) {
    switch (type) {
        case C_TKN_MUL:
        case C_TKN_DIV:
        case C_TKN_MOD: return 10;

        case C_TKN_ADD:
        case C_TKN_SUB: return 9;

        case C_TKN_SHL:
        case C_TKN_SHR: return 8;

        case C_TKN_LT:
        case C_TKN_LE:
        case C_TKN_GT:
        case C_TKN_GE: return 7;

        case C_TKN_NE:
        case C_TKN_EQ: return 6;

        case C_TKN_AND: return 5;

        case C_TKN_XOR: return 4;

        case C_TKN_OR: return 3;

        case C_TKN_LAND: return 2;

        case C_TKN_LOR: return 1;

        default: return -1;
    }
}

// Is this a valid prefix operator token?
static bool c_preproc_is_prefix_op(c_tokentype_t type) {
    switch (type) {
        case C_TKN_ADD:
        case C_TKN_SUB:
        case C_TKN_NOT:
        case C_TKN_LNOT: return true;
        default: return false;
    }
}

// Evaluate a prefix expression.
static i128_t c_preproc_eval_prefix(c_tokentype_t oper, i128_t value) {
    switch (oper) {
        case C_TKN_ADD: return value;
        case C_TKN_SUB: return neg128(value);
        case C_TKN_NOT: return bneg128(value);
        case C_TKN_LNOT: return int128(0, cmp128u(value, int128(0, 0)) == 0);
        default: abort();
    }
}

// Evaluate an infix expression.
static i128_t c_preproc_eval_infix(bool is_signed, i128_t lhs, c_tokentype_t oper, i128_t rhs) {
    switch (oper) {
        case C_TKN_MUL: return mul128(lhs, rhs);
        case C_TKN_DIV:
            if (is_signed) {
                return div128s(lhs, rhs);
            } else {
                return div128u(lhs, rhs);
            }
        case C_TKN_MOD:
            if (is_signed) {
                return rem128s(lhs, rhs);
            } else {
                return rem128u(lhs, rhs);
            }

        case C_TKN_ADD: return add128(lhs, rhs);
        case C_TKN_SUB: return add128(lhs, neg128(rhs));

        case C_TKN_SHL: return shl128(lhs, lo64(rhs));
        case C_TKN_SHR:
            if (is_signed) {
                return shr128s(lhs, lo64(rhs));
            } else {
                return shr128u(lhs, lo64(rhs));
            }

        case C_TKN_LT:
            if (is_signed) {
                return int128(0, cmp128s(lhs, rhs) < 0);
            } else {
                return int128(0, cmp128u(lhs, rhs) < 0);
            }
        case C_TKN_LE:
            if (is_signed) {
                return int128(0, cmp128s(lhs, rhs) <= 0);
            } else {
                return int128(0, cmp128u(lhs, rhs) <= 0);
            }
        case C_TKN_GT:
            if (is_signed) {
                return int128(0, cmp128s(lhs, rhs) > 0);
            } else {
                return int128(0, cmp128u(lhs, rhs) > 0);
            }
        case C_TKN_GE:
            if (is_signed) {
                return int128(0, cmp128s(lhs, rhs) >= 0);
            } else {
                return int128(0, cmp128u(lhs, rhs) >= 0);
            }

        case C_TKN_NE: return int128(0, cmp128u(lhs, rhs) != 0);
        case C_TKN_EQ: return int128(0, cmp128u(lhs, rhs) == 0);

        case C_TKN_AND: return and128(lhs, rhs);

        case C_TKN_XOR: return xor128(lhs, rhs);

        case C_TKN_OR: return or128(lhs, rhs);

        case C_TKN_LAND: return int128(0, cmp128u(lhs, int128(0, 0)) && cmp128u(rhs, int128(0, 0)));

        case C_TKN_LOR: return int128(0, cmp128u(lhs, int128(0, 0)) || cmp128u(rhs, int128(0, 0)));

        default: abort();
    }
}

// Helper for `c_preproc_eval` that parses and evaluates `defined ...` and `defined (...)` and converts constants.
static token_t c_preproc_eval_get_helper(c_preproc_t *pre) {
    token_t oper = c_preproc_get_tkn(pre, LINE_NOWS_EXPAND);
    if (oper.type == TOKENTYPE_SCONST) {
        token_t res = c_tkn_conv_str(pre->shared->cctx, pre->shared->c_std, &oper);
        tkn_delete(oper);
        return res;
    } else if (oper.type == TOKENTYPE_IDENT && strcmp(oper.strval, "defined") != 0) {
        if (oper.subtype == C_PPNUMBER) {
            token_t res = c_tkn_conv_number(pre->shared->cctx, pre->shared->c_std, &oper);
            tkn_delete(oper);
            return res;
        } else {
            return oper;
        }
    } else if (oper.type != TOKENTYPE_IDENT) {
        return oper;
    }
    pos_t pos = oper.pos;

    bool    eval = false;
    token_t lpar = c_preproc_get_tkn(pre, LINE_NOWS_RAW);
    if (lpar.type == TOKENTYPE_IDENT && lpar.subtype == C_IDENT) {
        eval = map_get(&pre->shared->macros, lpar.strval) != NULL;
    } else if (lpar.type == TOKENTYPE_OTHER && lpar.subtype == C_TKN_LPAR) {
        token_t name = c_preproc_get_tkn(pre, LINE_NOWS_RAW);
        token_t rpar = c_preproc_get_tkn(pre, LINE_NOWS_RAW);
        if (name.type != TOKENTYPE_IDENT || name.subtype != C_IDENT) {
            cctx_diagnostic(pre->shared->cctx, name.pos, DIAG_ERR, "Expected identifier");
        } else if (rpar.type != TOKENTYPE_OTHER || rpar.subtype != C_TKN_RPAR) {
            cctx_diagnostic(pre->shared->cctx, name.pos, DIAG_ERR, "Expected )");
        } else {
            eval = map_get(&pre->shared->macros, name.strval) != NULL;
        }
    } else {
        cctx_diagnostic(pre->shared->cctx, lpar.pos, DIAG_ERR, "Expected identifier or (");
    }

    return (token_t){
        .pos  = pos,
        .type = TOKENTYPE_ICONST,
        .ival = eval,
    };
}

// Evaluate the condition for an `#if` or `#elif` directive.
static bool c_preproc_eval(c_preproc_t *pre, pos_t pos) {
    enum entry_type {
        ENTRY_TOKEN,
        ENTRY_VALUE,
        ENTRY_GARBAGE,
    };
    struct entry {
        enum entry_type type;
        pos_t           pos;
        union {
            c_tokentype_t token;
            struct {
                i128_t value;
                bool   is_signed;
            } value;
        };
    };

    struct entry *stack     = NULL;
    size_t        stack_len = 0;
    size_t        stack_cap = 0;
    bool          has_peek;
    struct entry  peek;

#pragma region
    // Push a node/token to the stack.
#define push(thing)                                                                                                    \
    do {                                                                                                               \
        struct entry push_temporary_value = thing;                                                                     \
        array_lencap_insert_strong(                                                                                    \
            &stack,                                                                                                    \
            sizeof(struct entry),                                                                                      \
            &stack_len,                                                                                                \
            &stack_cap,                                                                                                \
            &push_temporary_value,                                                                                     \
            stack_len                                                                                                  \
        );                                                                                                             \
    } while (0)
    // Pop a node/token from the stack.
#define pop()                                                                                                          \
    ({                                                                                                                 \
        struct entry pop_temporary_value = stack[stack_len - 1];                                                       \
        stack_len--;                                                                                                   \
        pop_temporary_value;                                                                                           \
    })
    // Index by depth.
#define by_depth(depth) stack[stack_len - (depth) - 1]
    // Is this a specific kind of token?
#define is_token2(depth, subtype)                                                                                      \
    (stack_len > (depth) && by_depth(depth).type == ENTRY_TOKEN && by_depth(depth).token == (subtype))
    // Is this a token?
#define is_token(depth) (stack_len > (depth) && by_depth(depth).type == ENTRY_TOKEN)
    // Is this a value?
#define is_value(depth) (stack_len > (depth) && by_depth(depth).type == ENTRY_VALUE)
    // Read the next token into peek.
#define peek()                                                                                                         \
    do {                                                                                                               \
        token_t tkn = c_preproc_eval_get_helper(pre);                                                                  \
        has_peek    = tkn.type != TOKENTYPE_EOL;                                                                       \
        if (has_peek) {                                                                                                \
            peek.pos = tkn.pos;                                                                                        \
            if (tkn.type == TOKENTYPE_ICONST || tkn.type == TOKENTYPE_CCONST) {                                        \
                peek.type            = ENTRY_VALUE;                                                                    \
                peek.value.value     = int128(tkn.ivalh, tkn.ival);                                                    \
                peek.value.is_signed = (tkn.subtype & 1) == 0;                                                         \
            } else if (                                                                                                \
                tkn.type == TOKENTYPE_OTHER                                                                            \
                && (tkn.subtype == C_TKN_LPAR || tkn.subtype == C_TKN_RPAR                                             \
                    || c_preproc_op_precedence(tkn.subtype) != -1 || c_preproc_is_prefix_op(tkn.subtype))              \
            ) {                                                                                                        \
                peek.type  = ENTRY_TOKEN;                                                                              \
                peek.token = tkn.subtype;                                                                              \
            } else if (tkn.type == TOKENTYPE_IDENT) {                                                                  \
                peek.type            = ENTRY_VALUE;                                                                    \
                peek.value.is_signed = true;                                                                           \
                peek.value.value     = int128(0, pre->shared->c_std >= C_STD_C23 && !strcmp(tkn.strval, "true"));      \
            } else {                                                                                                   \
                peek.type = ENTRY_GARBAGE;                                                                             \
            }                                                                                                          \
        }                                                                                                              \
        tkn_delete(tkn);                                                                                               \
    } while (0)
#pragma endregion

    peek();
    while (1) {
        if (is_token2(2, C_TKN_LPAR) && is_value(1) && is_token2(0, C_TKN_RPAR)) {
            // Reduce parentheses.
            struct entry rpar = pop();
            struct entry tmp  = pop();
            struct entry lpar = pop();
            tmp.pos           = pos_including(lpar.pos, rpar.pos);
            push(tmp);
        } else if (!is_value(2) && is_token(1) && c_preproc_is_prefix_op(by_depth(1).token) && is_value(0)) {
            // Reduce prefix.
            struct entry value = pop();
            struct entry oper  = pop();
            value.value.value  = c_preproc_eval_prefix(oper.token, value.value.value);
            value.pos          = pos_including(oper.pos, value.pos);
            push(value);
        } else if (
            is_value(2) && is_token(1) && is_value(0)
            && (!has_peek || peek.type != ENTRY_TOKEN
                || c_preproc_op_precedence(by_depth(1).token) >= c_preproc_op_precedence(peek.token))
        ) {
            // Reduce infix.
            struct entry rhs       = pop();
            struct entry oper      = pop();
            struct entry lhs       = pop();
            bool         is_signed = lhs.value.is_signed && rhs.value.is_signed;
            struct entry res       = {
                .type  = ENTRY_VALUE,
                .value = {
                    .value     = c_preproc_eval_infix(is_signed, lhs.value.value, oper.token, rhs.value.value),
                    .is_signed = is_signed,
                },
            };
            push(res);
        } else if (has_peek) {
            // Push a token.
            push(peek);
            peek();
        } else {
            // Nothing to push, can't reduce anymore.
            break;
        }
    }

    if (stack_len == 0) {
        cctx_diagnostic(pre->shared->cctx, pos, DIAG_ERR, "Expected preprocessor expression");
        return 0;
    } else if (stack_len == 1 && stack[0].type == ENTRY_VALUE) {
        return cmp128u(stack[0].value.value, int128(0, 0)) != 0;
    } else {
        cctx_diagnostic(
            pre->shared->cctx,
            pos_including(stack[0].pos, stack[stack_len - 1].pos),
            DIAG_ERR,
            "Invalid preprocessor expression"
        );
        return 0;
    }
}

// Preprocessor directive: include.
static void c_directive_include(c_preproc_t *pre, pos_t pos) {
    (void)pos;
    c_incfile_t *file = &pre->stack[pre->stack_len - 1];
    assert(file->tkn_ctx->next == &c_tkn_next);
    c_tokenizer_t *c_tkn = (c_tokenizer_t *)file->tkn_ctx;
    c_tkn->str_anglebrac = true;
    token_t token        = c_preproc_get_tkn(pre, LINE_NOWS_EXPAND);
    c_tkn->str_anglebrac = false;
    if (token.type != TOKENTYPE_SCONST) {
        cctx_diagnostic(pre->shared->cctx, token.pos, DIAG_ERR, "Expected a path");
        tkn_delete(token);
        return;
    }
    // This needs to happen *before* the include file is pushed.
    c_preproc_until_eol(pre, true);

    // TODO: Is there a difference between `<>` and `""` style strings for `#include`?
    c_incfile_push(pre, token.pos, token.strval);
    tkn_delete(token);
}

// Preprocessor directive: pragma.
static void c_directive_pragma(c_preproc_t *pre, pos_t pos) {
    (void)pos;
    pos_t span;
    char *buf = c_preproc_read_bytes(pre, &span);
    if (!buf) {
        return;
    }
    c_preproc_pragma(pre, span, buf);
    free(buf);
}

// Preprocessor directive: if/elif.
static void c_directive_if(c_preproc_t *pre, pos_t pos, bool elif, bool ifdef, bool ifndef) {
    c_incfile_t *file = &pre->stack[pre->stack_len - 1];

    bool eval;
    if (ifdef || ifndef) {
        token_t tkn = c_preproc_get_tkn(pre, LINE_NOWS_RAW);
        if (tkn.type != TOKENTYPE_IDENT) {
            cctx_diagnostic(pre->shared->cctx, tkn.pos, DIAG_ERR, "Expected an identifier");
            return;
        }
        eval = map_get(&pre->shared->macros, tkn.strval) != NULL;
        if (ifndef) {
            eval = !eval;
        }
    } else {
        eval = c_preproc_eval(pre, pos);
    }

    bool parent_emit = file->ifdir_len < 2 || file->ifdir[file->ifdir_len - 2].do_emit;
    if (elif) {
        if (!file->ifdir_len) {
            cctx_diagnostic(pre->shared->cctx, pos, DIAG_ERR, "#elif without matching #if");
            return;
        }
        c_ifdir_t *ifdir = &file->ifdir[file->ifdir_len - 1];
        if (!ifdir->allow_else) {
            cctx_diagnostic(pre->shared->cctx, pos, DIAG_ERR, "#elif without matching #if");
            cctx_diagnostic(pre->shared->cctx, ifdir->pos, DIAG_INFO, "Most recent #else directive");
            cctx_diagnostic(pre->shared->cctx, pos, DIAG_HINT, "Change the #elif into an #if");
            return;
        }
        ifdir->pos       = pos;
        ifdir->do_emit   = !ifdir->disabled && eval && parent_emit;
        ifdir->disabled |= ifdir->do_emit;
    } else {
        c_ifdir_t ifdir = {
            .pos        = pos,
            .allow_else = true,
            .disabled   = eval,
            .do_emit    = eval && parent_emit,
        };
        array_lencap_insert_strong(
            &file->ifdir,
            sizeof(c_ifdir_t),
            &file->ifdir_len,
            &file->ifdir_cap,
            &ifdir,
            file->ifdir_len
        );
    }
}

// Preprocessor directive: else.
static void c_directive_else(c_preproc_t *pre, pos_t pos) {
    c_incfile_t *file = &pre->stack[pre->stack_len - 1];

    if (!file->ifdir_len) {
        cctx_diagnostic(pre->shared->cctx, pos, DIAG_ERR, "#else without matching #if");
        return;
    }
    c_ifdir_t *ifdir       = &file->ifdir[file->ifdir_len - 1];
    bool       parent_emit = file->ifdir_len < 2 || file->ifdir[file->ifdir_len - 2].do_emit;

    if (!ifdir->allow_else) {
        cctx_diagnostic(pre->shared->cctx, pos, DIAG_ERR, "Dangling #else");
        cctx_diagnostic(pre->shared->cctx, ifdir->pos, DIAG_INFO, "Earlier #else occurred here");
        cctx_diagnostic(pre->shared->cctx, pos, DIAG_HINT, "Write #endif here");
        return;
    }

    ifdir->pos        = pos;
    ifdir->allow_else = false;
    ifdir->do_emit    = parent_emit && !ifdir->disabled;
}

// Preprocessor directive: endif.
static void c_directive_endif(c_preproc_t *pre, pos_t pos) {
    c_incfile_t *file = &pre->stack[pre->stack_len - 1];

    if (!file->ifdir_len) {
        cctx_diagnostic(pre->shared->cctx, pos, DIAG_ERR, "#endif without matching #if");
        return;
    }
    file->ifdir_len--;
}

// Preprocessor directive: warning.
static void c_directive_warning(c_preproc_t *pre, pos_t pos, bool is_error) {
    (void)pos;
    pos_t span;
    char *buf = c_preproc_read_bytes(pre, &span);
    if (!buf) {
        return;
    }
    cctx_diagnostic(pre->shared->cctx, span, is_error ? DIAG_ERR : DIAG_WARN, "%s", buf);
    free(buf);
}

// Preprocessor directive: define.
static void c_directive_define(c_preproc_t *pre, pos_t pos) {
    (void)pos;
    token_t name = c_preproc_get_tkn(pre, LINE_NOWS_RAW);
    if (name.type == TOKENTYPE_EOL) {
        cctx_diagnostic(pre->shared->cctx, name.pos, DIAG_ERR, "Expected macro name");
        return;
    } else if (name.type != TOKENTYPE_IDENT) {
        cctx_diagnostic(pre->shared->cctx, name.pos, DIAG_ERR, "Macro name must be an identifier");
        return;
    }

    c_macro_t *macro      = strong_calloc(1, sizeof(c_macro_t));
    size_t     params_cap = 0;

    // A `(` immediately after the name (no intervening whitespace) makes this a
    // function-like macro; otherwise the rest of the line is the body.
    token_t peek = c_preproc_raw_peek(pre);
    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_LPAR) {
        macro->uses_args   = true;
        token_t lpar       = c_preproc_raw_next(pre);
        pos_t   list_start = lpar.pos;
        tkn_delete(lpar);
        bool need_arg   = false;
        bool must_close = false;
        while (1) {
            token_t tkn = c_preproc_get_tkn(pre, LINE_NOWS_RAW);
            if (tkn.type == TOKENTYPE_EOL) {
                // Abrupt end of line.
                cctx_diagnostic(pre->shared->cctx, list_start, DIAG_ERR, "Missing `(` to match this `)`");
                tkn_delete(tkn);
                goto error;
            } else if (!need_arg && tkn.type == TOKENTYPE_OTHER && tkn.subtype == C_TKN_RPAR) {
                // Empty parameter list.
                tkn_delete(tkn);
                break;
            } else if (tkn.type == TOKENTYPE_OTHER && tkn.subtype == C_TKN_VARARG) {
                // Elipsis (`...`).
                macro->regular.is_variadic = true;
                must_close                 = true;
                tkn_delete(tkn);
            } else if (tkn.type == TOKENTYPE_IDENT) {
                if (!strcmp(tkn.strval, "__VA_ARGS__") || !strcmp(tkn.strval, "__VA_OPT__")) {
                    cctx_diagnostic(
                        pre->shared->cctx,
                        tkn.pos,
                        DIAG_WARN,
                        "Macro parameters should not be called %s",
                        tkn.strval
                    );
                }
                char *str = strong_strdup(tkn.strval);
                array_lencap_insert_strong(
                    &macro->regular.args,
                    sizeof(char *),
                    &macro->regular.args_len,
                    &params_cap,
                    &str,
                    macro->regular.args_len
                );
                tkn_delete(tkn);
            } else {
                cctx_diagnostic(pre->shared->cctx, tkn.pos, DIAG_ERR, "Expected an identifier or `...`");
                tkn_delete(tkn);
                goto error;
            }

            tkn = c_preproc_get_tkn(pre, LINE_NOWS_RAW);
            if (tkn.type == TOKENTYPE_OTHER && tkn.subtype == C_TKN_RPAR) {
                tkn_delete(tkn);
                break;
            } else if (tkn.type == TOKENTYPE_OTHER && tkn.subtype == C_TKN_COMMA) {
                if (must_close) {
                    cctx_diagnostic(
                        pre->shared->cctx,
                        tkn.pos,
                        DIAG_ERR,
                        "`...` must appear last in the argument list of a variadic macro"
                    );
                    tkn_delete(tkn);
                    goto error;
                }
                tkn_delete(tkn);
            }
        }
    }

    if (!c_macro_parse_body(macro, pre->stack[pre->stack_len - 1].tkn_ctx)) {
        goto error;
    }

    c_macro_t *existing = map_get(&pre->shared->macros, name.strval);
    if (existing) {
        cctx_diagnostic(
            pre->shared->cctx,
            name.pos,
            DIAG_WARN,
            "Redefinition of %smacro `%s`",
            existing->is_builtin ? "built-in " : "",
            name.strval
        );
        c_macro_destroy(existing);
    }
    map_set(&pre->shared->macros, name.strval, macro);
    return;

error:
    c_macro_destroy(macro);
    token_t del = c_preproc_get_tkn(pre, LINE_RAW);
    while (del.type != TOKENTYPE_EOL) {
        tkn_delete(del);
        del = c_preproc_get_tkn(pre, LINE_RAW);
    }
    tkn_delete(del);
}

// Preprocessor directive: undef.
static void c_directive_undef(c_preproc_t *pre, pos_t pos) {
    (void)pos;
    token_t name = c_preproc_get_tkn(pre, LINE_NOWS_RAW);
    if (name.type == TOKENTYPE_EOL) {
        cctx_diagnostic(pre->shared->cctx, name.pos, DIAG_ERR, "Expected macro name");
        return;
    } else if (name.type != TOKENTYPE_IDENT) {
        cctx_diagnostic(pre->shared->cctx, name.pos, DIAG_ERR, "Macro name must be an identifier");
        return;
    }

    c_macro_t *macro = map_get(&pre->shared->macros, name.strval);
    if (!macro) {
        // Nothing to do.
        return;
    }

    if (macro->is_builtin) {
        cctx_diagnostic(pre->shared->cctx, name.pos, DIAG_WARN, "Undefining a built-in macro");
    }
    map_remove(&pre->shared->macros, name.strval);
    c_macro_destroy(macro);
}

// Handle a preprocessor directive.
static void c_preproc_directive(c_preproc_t *pre) {
    token_t name = c_preproc_get_tkn(pre, LINE_NOWS_RAW);
    if (name.type == TOKENTYPE_EOL || name.type == TOKENTYPE_EOF) {
        // Note: Counterintuitively, `#` followed by newline does nothing according to the spec.
        return;
    } else if (name.type != TOKENTYPE_IDENT) {
        cctx_diagnostic(pre->shared->cctx, name.pos, DIAG_ERR, "Expected preprocessing directive name");
        tkn_delete(name);
        return;
    }

    // The if directives are always processed.
    if (!strcmp(name.strval, "if")) {
        c_directive_if(pre, name.pos, false, false, false);
    } else if (!strcmp(name.strval, "ifdef")) {
        c_directive_if(pre, name.pos, false, true, false);
    } else if (!strcmp(name.strval, "ifndef")) {
        c_directive_if(pre, name.pos, false, false, true);
    } else if (!strcmp(name.strval, "elif")) {
        c_directive_if(pre, name.pos, true, false, false);
    } else if (!strcmp(name.strval, "elifdef")) {
        c_directive_if(pre, name.pos, true, true, false);
    } else if (!strcmp(name.strval, "elifndef")) {
        c_directive_if(pre, name.pos, true, false, true);
    } else if (!strcmp(name.strval, "else")) {
        c_directive_else(pre, name.pos);
    } else if (!strcmp(name.strval, "endif")) {
        c_directive_endif(pre, name.pos);
    } else if (!c_preproc_do_emit(pre)) {
        // Any remaining directives are only processed if the current if directive branch is active.
        c_preproc_until_eol(pre, false);
        return;
    } else if (!strcmp(name.strval, "include")) {
        c_directive_include(pre, name.pos);
        return; // `#include` pushes files to the stack, so it consumes until EOL itself.
    } else if (!strcmp(name.strval, "pragma")) {
        c_directive_pragma(pre, name.pos);
    } else if (!strcmp(name.strval, "warning")) {
        c_directive_warning(pre, name.pos, false);
    } else if (!strcmp(name.strval, "error")) {
        c_directive_warning(pre, name.pos, true);
    } else if (!strcmp(name.strval, "define")) {
        c_directive_define(pre, name.pos);
    } else if (!strcmp(name.strval, "undef")) {
        c_directive_undef(pre, name.pos);
    }

    c_preproc_until_eol(pre, true);
}

#pragma endregion directives

#pragma region tokenizing

// Escape a string so that it can be made into a preprocessing string.
static char *c_preproc_esc_str(char const *raw) {
    size_t n_esc = 0;
    for (size_t i = 0; raw[i]; i++) {
        if (raw[i] == '\\' || raw[i] == '\"') {
            n_esc++;
        }
    }
    char  *res = malloc(strlen(raw) + n_esc + 1);
    size_t o   = 0;
    for (size_t i = 0; raw[i]; i++) {
        if (raw[i] == '\\' || raw[i] == '\"') {
            res[o++] = '\\';
        }
        res[o++] = raw[i];
    }
    res[o++] = 0;
    return res;
}

// Read bytes up to but excluding the next newline.
static char *c_preproc_read_bytes(c_preproc_t *pre, pos_t *pos_out) {
    c_incfile_t *file = &pre->stack[pre->stack_len - 1];
    pos_t        start_pos;
    pos_t        end_pos;
    bool         has_token = false;

    size_t len = 0;
    size_t cap = 64;
    char  *buf = strong_malloc(cap);

    while (1) {
        token_t peek = tkn_peek(file->tkn_ctx);
        if (peek.type == TOKENTYPE_EOL || peek.type == TOKENTYPE_EOF) {
            break;
        }
        token_t tkn = tkn_next(file->tkn_ctx);
        if (tkn.type != TOKENTYPE_WHITESPACE && !has_token) {
            start_pos = tkn.pos;
        }
        if (has_token) {
            c_tkn_append_src(&tkn, &buf, &len, &cap);
            end_pos = tkn.pos;
        }
        tkn_delete(tkn);
    }

    if (!len) {
        free(buf);
        return NULL;
    }

    for (size_t i = 0; i < len; i++) {
        // Since NUL is treated as whitespace but the output needs to be a C-string,
        // silently replace NUL with something printable instead.
        if (buf[i] == 0) {
            buf[i] = ' ';
        }
    }
    array_lencap_insert_strong(&buf, 1, &len, &cap, "", len);
    *pos_out = pos_including(start_pos, end_pos);
    return buf;
}

// Consume tokens up to and including the next newline.
// If `warn_extra_tok` is `true`, emit a warning if any non-whitespace tokens exist.
static void c_preproc_until_eol(c_preproc_t *pre, bool warn_extra_tok) {
    bool  has_extra = false;
    pos_t extra;

    while (1) {
        token_t tkn = c_preproc_get_tkn(pre, NEXT_RAW);
        if (tkn.type == TOKENTYPE_EOL || tkn.type == TOKENTYPE_EOF) {
            tkn_delete(tkn);
            break;
        } else if (tkn.type != TOKENTYPE_WHITESPACE) {
            if (has_extra) {
                extra = pos_including(extra, tkn.pos);
            } else {
                extra = tkn.pos;
            }
            tkn_delete(tkn);
        }
    }

    if (warn_extra_tok && has_extra) {
        cctx_diagnostic(pre->shared->cctx, extra, DIAG_WARN, "Extra tokens ignored");
    }
}

// Whether the current if/else directive's code is active.
// Always true if no if/else directive currently exists.
static bool c_preproc_do_emit(c_preproc_t *pre) {
    c_incfile_t *file = &pre->stack[pre->stack_len - 1];
    return file->ifdir_len == 0 || file->ifdir[file->ifdir_len - 1].do_emit;
}

// Look past any whitespace tokens in the current raw stream without consuming them.
// Returns the first non-whitespace token (or EOF), and writes the number of
// skipped whitespace tokens to `*ws_out`. The caller can commit by calling
// `c_preproc_raw_next` that many times.
static token_t c_preproc_raw_peek_nonws(c_preproc_t *pre, size_t *ws_out) {
    size_t ws = 0;

    // Walk the macro expansion stack from top to bottom.
    for (size_t i = pre->expand_len; i-- > 0;) {
        c_expansion_t *expand = &pre->expand[i];
        for (size_t j = expand->index; j < expand->tokens_len; j++) {
            if (expand->tokens[j].type == TOKENTYPE_WHITESPACE) {
                ws++;
            } else {
                if (ws_out) {
                    *ws_out = ws;
                }
                return expand->tokens[j];
            }
        }
    }

    // Walk the include-file stack from top to bottom.
    for (size_t i = pre->stack_len; i-- > 0;) {
        tokenizer_t *tknz  = pre->stack[i].tkn_ctx;
        size_t       depth = 0;
        while (1) {
            token_t p = tkn_peek_n(tknz, depth);
            if (p.type == TOKENTYPE_EOF) {
                break;
            }
            if (p.type == TOKENTYPE_WHITESPACE) {
                ws++;
                depth++;
                continue;
            }
            if (ws_out) {
                *ws_out = ws;
            }
            return p;
        }
    }

    if (ws_out) {
        *ws_out = ws;
    }
    return (token_t){.type = TOKENTYPE_EOF};
}

// Helper function for `c_preproc_get_tkn` that peeks raw tokens, first from macros, then from the srcfiles.
static token_t c_preproc_raw_peek(c_preproc_t *pre) {
    // First check the macro stack.
    for (size_t i = pre->expand_len - 1; i != SIZE_MAX; i--) {
        c_expansion_t *expand = &pre->expand[i];
        if (expand->index < expand->tokens_len) {
            return expand->tokens[expand->index];
        }
    }

    // Not to be found in macros; check include files.
    for (size_t i = pre->stack_len - 1; i >= 1; i--) {
        token_t peek = tkn_peek(pre->stack[i].tkn_ctx);
        if (peek.type != TOKENTYPE_EOF) {
            return peek;
        }
    }

    // Check source file.
    return tkn_peek(pre->stack[0].tkn_ctx);
}

// Helper function for `c_preproc_get_tkn` that gets raw tokens, first from macros, then from the srcfiles.
static token_t c_preproc_raw_next(c_preproc_t *pre) {
    token_t tkn;

    // First check the macro stack.
    for (size_t i = pre->expand_len - 1; i != SIZE_MAX; i--) {
        c_expansion_t *expand = &pre->expand[i];
        if (expand->index < expand->tokens_len) {
            tkn = expand->tokens[expand->index++];
            goto emit;
        }
        free(expand->tokens);
        pre->expand_len--;
    }

    // Not to be found in macros; check include files.
    while (pre->stack_len >= 2) {
        tkn = tkn_next(pre->stack[pre->stack_len - 1].tkn_ctx);
        if (tkn.type == TOKENTYPE_EOF) {
            tkn_delete(tkn);
        } else {
            goto emit;
        }
        c_incfile_eof(pre);
        c_incfile_pop(pre);
    }

    // Check source file; if it EOFs, just return it verbatim.
    tkn = tkn_next(pre->stack[0].tkn_ctx);

emit:
    if (tkn.type == TOKENTYPE_EOL) {
        pre->blank_line = true;
    } else if (tkn.type != TOKENTYPE_WHITESPACE) {
        pre->blank_line = false;
    }

    return tkn;
}

// Get the next token for preprocessing.
static token_t c_preproc_get_tkn(c_preproc_t *pre, next_mode_t mode) {
    bool do_expand;
    bool skip_whitespace;
    bool allow_next_line;
    switch (mode) {
        case NEXT_EXPAND:
            do_expand       = true;
            skip_whitespace = false;
            allow_next_line = true;
            break;
        case NEXT_RAW:
            do_expand       = false;
            skip_whitespace = false;
            allow_next_line = true;
            break;
        case LINE_EXPAND:
            do_expand       = true;
            skip_whitespace = false;
            allow_next_line = false;
            break;
        case LINE_RAW:
            do_expand       = false;
            skip_whitespace = false;
            allow_next_line = false;
            break;
        case LINE_NOWS_EXPAND:
            do_expand       = true;
            skip_whitespace = true;
            allow_next_line = false;
            break;
        case LINE_NOWS_RAW:
            do_expand       = false;
            skip_whitespace = true;
            allow_next_line = false;
            break;
    }

    token_t peek;
again:
    peek = c_preproc_raw_peek(pre);
    if (skip_whitespace) {
        while (peek.type == TOKENTYPE_WHITESPACE) {
            c_preproc_raw_next(pre);
            peek = c_preproc_raw_peek(pre);
        }
    }

    if (peek.type == TOKENTYPE_EOL && !allow_next_line) {
        return (token_t){
            .pos  = peek.pos,
            .type = TOKENTYPE_EOL,
        };
    }

    // Is this a macro?
    token_t tkn = c_preproc_raw_next(pre);
    if (tkn.type != TOKENTYPE_IDENT || !do_expand) {
        return tkn;
    }
    c_macro_t const *macro = map_get(&pre->shared->macros, tkn.strval);
    if (!macro) {
        return tkn;
    }
    // Check that this macro wasn't already expanded.
    for (size_t i = 0; i < pre->expand_len; i++) {
        if (pre->expand[i].macro == macro) {
            return tkn;
        }
    }

    if (macro->uses_args) {
        size_t  ws_count;
        token_t lookahead = c_preproc_raw_peek_nonws(pre, &ws_count);
        if (lookahead.type != TOKENTYPE_OTHER || lookahead.subtype != C_TKN_LPAR) {
            return tkn;
        }
        // Commit: discard the whitespace tokens that preceded the `(`.
        for (size_t i = 0; i < ws_count; i++) {
            tkn_delete(c_preproc_raw_next(pre));
        }
    }
    c_macro_expand(pre, tkn.pos, macro);
    tkn_delete(tkn);
    goto again;
}

#pragma endregion tokenizing

// Get the next preprocessed token.
token_t c_preproc_next(tokenizer_t *ctx) {
    c_preproc_t *pre = (c_preproc_t *)ctx;

again:
    if (pre->no_directives || !pre->blank_line) {
        goto emit;
    }
    token_t peek = c_preproc_raw_peek(pre);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_HASH) {
        goto emit;
    }

    // Blank line, verbatim `#` in the source -> process directives.
    tkn_delete(c_preproc_raw_next(pre)); // Consumes the `#` peeked earlier.
    c_preproc_directive(pre);
    goto again;

emit:
    if (c_preproc_do_emit(pre)) {
        token_t tkn = c_preproc_get_tkn(pre, NEXT_EXPAND);
        if (!pre->raw_mode) {
            tkn = c_preproc_tkn_to_c_tkn(pre, tkn);
        }
        return tkn;
    } else {
        do {
            tkn_delete(c_preproc_get_tkn(pre, NEXT_RAW));
        } while (!pre->blank_line);
        goto again;
    }
}

// Convert a preprocessor token to a C token.
// Identifiers whose spelling is a keyword become keyword tokens; preprocessing
// numbers go through `c_tkn_conv_number`; preprocessing string/char tokens go
// through `c_tkn_conv_str`. Other token types pass through unchanged.
token_t c_preproc_tkn_to_c_tkn(c_preproc_t *pre, token_t tkn) {
    if (tkn.type == TOKENTYPE_IDENT && tkn.subtype == C_PPNUMBER) {
        token_t res = c_tkn_conv_number(pre->shared->cctx, pre->shared->c_std, &tkn);
        tkn_delete(tkn);
        return res;
    } else if (tkn.type == TOKENTYPE_IDENT) {
        c_keyw_t keyw = c_keyw_get(pre->shared->c_std, tkn.strval);
        if (keyw >= C_N_KEYWS) {
            return tkn;
        }
        // Replace alternate spellings with the main spelling, matching the
        // mapping the C tokenizer applies when not in preprocessor mode.
#define C_ALT_KEYW_DEF(main_spelling, alt_spelling)                                                                    \
    if (keyw == C_KEYW_##main_spelling) {                                                                              \
        keyw = C_KEYW_##alt_spelling;                                                                                  \
    }
#include "c_keywords.inc"
        pos_t pos = tkn.pos;
        tkn_delete(tkn);
        return (token_t){
            .pos     = pos,
            .type    = TOKENTYPE_KEYWORD,
            .subtype = keyw,
        };
    } else if (tkn.type == TOKENTYPE_SCONST && tkn.subtype != C_STR_ANGLEBRAC) {
        token_t res = c_tkn_conv_str(pre->shared->cctx, pre->shared->c_std, &tkn);
        tkn_delete(tkn);
        return res;
    }
    return tkn;
}

#pragma region macros

// Add a command-line or predefined macro.
void c_preproc_add_macro(c_preproc_t *pre, char const *name, c_macro_t *macro) {
    map_set(&pre->shared->macros, name, macro);
}

// Mark argument substitutions adjacent to a `##` token with the `pasting` flag.
static void c_macro_mark_pasting(c_macro_subst_t *tokens, size_t tokens_len) {
    for (size_t i = 0; i < tokens_len; i++) {
        if (i > 0) {
            c_macro_subst_t const *prev = &tokens[i - 1];
            if (prev->token.type == TOKENTYPE_OTHER && prev->token.subtype == C_TKN_PASTE) {
                tokens[i].pasting = true;
                continue;
            }
        }
        if (i + 1 < tokens_len) {
            c_macro_subst_t const *next = &tokens[i + 1];
            if (next->token.type == TOKENTYPE_OTHER && next->token.subtype == C_TKN_PASTE) {
                tokens[i].pasting = true;
            }
        }
    }
}

// Common macro body parsing code for `c_macro_create` and `c_directive_define`.
// Reads body tokens (skipping whitespace) until EOL/EOF and appends substitutions
// to `macro->regular.subst`. The terminating EOL is left in the stream.
// Returns whether parsing succeeded.
static bool c_macro_parse_body(c_macro_t *macro, tokenizer_t *tkn_ctx) {
    cctx_t *cctx       = tkn_ctx->cctx;
    size_t  tokens_cap = macro->regular.subst_len;

    while (1) {
        // Skip whitespace; stop at EOL/EOF without consuming it.
        token_t peek = tkn_peek(tkn_ctx);
        while (peek.type == TOKENTYPE_WHITESPACE) {
            tkn_delete(tkn_next(tkn_ctx));
            peek = tkn_peek(tkn_ctx);
        }
        if (peek.type == TOKENTYPE_EOL || peek.type == TOKENTYPE_EOF) {
            break;
        }

        token_t tkn = tkn_next(tkn_ctx);

        if (macro->regular.subst_len == 0 && tkn.type == TOKENTYPE_OTHER && tkn.subtype == C_TKN_PASTE) {
            cctx_diagnostic(cctx, tkn.pos, DIAG_ERR, "`##` is not allowed at the start/end of macro expansion lists");
            tkn_delete(tkn);
            return false;
        }
        if (!macro->regular.is_variadic && tkn.type == TOKENTYPE_IDENT
            && (!strcmp(tkn.strval, "__VA_ARGS__") || !strcmp(tkn.strval, "__VA_OPT__"))) {
            cctx_diagnostic(cctx, tkn.pos, DIAG_WARN, "%s in non-variadic macro", tkn.strval);
        }

        c_macro_subst_t subst   = {0};
        bool            matched = false;
        if (macro->uses_args && tkn.type == TOKENTYPE_OTHER && tkn.subtype == C_TKN_HASH) {
            pos_t   hash_pos = tkn.pos;
            token_t next     = tkn_next(tkn_ctx);
            while (next.type == TOKENTYPE_WHITESPACE) {
                tkn_delete(next);
                next = tkn_next(tkn_ctx);
            }
            if (next.type != TOKENTYPE_IDENT) {
                cctx_diagnostic(cctx, hash_pos, DIAG_ERR, "`#` must be followed by a macro parameter name");
                tkn_delete(tkn);
                tkn_delete(next);
                return false;
            }
            bool found = false;
            for (size_t i = 0; i < macro->regular.args_len; i++) {
                if (!strcmp(next.strval, macro->regular.args[i])) {
                    found           = true;
                    subst.type      = C_SUBST_ARG;
                    subst.stringize = true;
                    subst.arg_index = i;
                    break;
                }
            }
            if (!found) {
                cctx_diagnostic(cctx, hash_pos, DIAG_ERR, "`#` must be followed by a macro parameter name");
                tkn_delete(tkn);
                tkn_delete(next);
                return false;
            }
            tkn_delete(tkn);
            tkn_delete(next);
            matched = true;
        } else if (tkn.type == TOKENTYPE_IDENT && !strcmp(tkn.strval, "__VA_OPT__")) {
            pos_t opt_pos = tkn.pos;
            tkn_delete(tkn);

            // Skip whitespace before `(`.
            token_t pp = tkn_peek(tkn_ctx);
            while (pp.type == TOKENTYPE_WHITESPACE) {
                tkn_delete(tkn_next(tkn_ctx));
                pp = tkn_peek(tkn_ctx);
            }
            if (pp.type != TOKENTYPE_OTHER || pp.subtype != C_TKN_LPAR) {
                cctx_diagnostic(cctx, opt_pos, DIAG_ERR, "`__VA_OPT__` must be followed by `(`");
                return false;
            }
            token_t lpar    = tkn_next(tkn_ctx);
            pos_t   end_pos = lpar.pos;
            tkn_delete(lpar);

            // Read tokens until the matching `)`; `(` and `)` nest. Whitespace
            // does not appear in stored tokens but `ws_before[i]` records whether
            // whitespace preceded the i-th token in the original input.
            token_t *opt_tokens     = NULL;
            size_t   opt_tokens_len = 0;
            size_t   opt_tokens_cap = 0;
            bool    *opt_ws         = NULL;
            size_t   opt_ws_cap     = 0;
            bool     saw_ws         = false;
            int      depth          = 0;
            while (1) {
                token_t p = tkn_peek(tkn_ctx);
                if (p.type == TOKENTYPE_EOL || p.type == TOKENTYPE_EOF) {
                    cctx_diagnostic(cctx, opt_pos, DIAG_ERR, "Unterminated `__VA_OPT__(`");
                    for (size_t x = 0; x < opt_tokens_len; x++) {
                        tkn_delete(opt_tokens[x]);
                    }
                    free(opt_tokens);
                    free(opt_ws);
                    return false;
                }
                if (p.type == TOKENTYPE_WHITESPACE) {
                    tkn_delete(tkn_next(tkn_ctx));
                    saw_ws = true;
                    continue;
                }
                if (p.type == TOKENTYPE_OTHER && p.subtype == C_TKN_RPAR && depth == 0) {
                    token_t rpar = tkn_next(tkn_ctx);
                    end_pos      = rpar.pos;
                    tkn_delete(rpar);
                    break;
                }
                token_t t = tkn_next(tkn_ctx);
                if (t.type == TOKENTYPE_OTHER && t.subtype == C_TKN_LPAR) {
                    depth++;
                } else if (t.type == TOKENTYPE_OTHER && t.subtype == C_TKN_RPAR) {
                    depth--;
                }
                bool   ws     = opt_tokens_len > 0 && saw_ws;
                size_t ws_len = opt_tokens_len;
                saw_ws        = false;
                array_lencap_insert_strong(&opt_ws, sizeof(bool), &ws_len, &opt_ws_cap, &ws, ws_len);
                array_lencap_insert_strong(
                    &opt_tokens,
                    sizeof(token_t),
                    &opt_tokens_len,
                    &opt_tokens_cap,
                    &t,
                    opt_tokens_len
                );
            }

            subst.type              = C_SUBST_VA_OPT;
            subst.va_opt.pos        = pos_including(opt_pos, end_pos);
            subst.va_opt.tokens     = opt_tokens;
            subst.va_opt.tokens_len = opt_tokens_len;
            subst.va_opt.ws_before  = opt_ws;
            matched                 = true;
        } else if (tkn.type == TOKENTYPE_IDENT && !strcmp(tkn.strval, "__VA_ARGS__")) {
            subst.type = C_SUBST_VA_ARGS;
            matched    = true;
        } else if (tkn.type == TOKENTYPE_IDENT) {
            for (size_t i = 0; i < macro->regular.args_len; i++) {
                if (!strcmp(tkn.strval, macro->regular.args[i])) {
                    tkn_delete(tkn);
                    subst.type      = C_SUBST_ARG;
                    subst.stringize = false;
                    subst.arg_index = i;
                    matched         = true;
                    break;
                }
            }
        }
        if (!matched) {
            subst.type  = C_SUBST_TOKEN;
            subst.token = tkn;
        }
        array_lencap_insert_strong(
            &macro->regular.subst,
            sizeof(c_macro_subst_t),
            &macro->regular.subst_len,
            &tokens_cap,
            &subst,
            macro->regular.subst_len
        );
    }

    if (macro->regular.subst_len) {
        c_macro_subst_t const *last = &macro->regular.subst[macro->regular.subst_len - 1];
        if (last->type == C_SUBST_TOKEN && last->token.type == TOKENTYPE_OTHER && last->token.subtype == C_TKN_PASTE) {
            cctx_diagnostic(
                cctx,
                last->token.pos,
                DIAG_ERR,
                "`##` is not allowed at the start/end of macro expansion lists"
            );
            return false;
        }
    }

    c_macro_mark_pasting(macro->regular.subst, macro->regular.subst_len);
    return true;
}

// Create a regular macro.
c_macro_t *c_macro_create(char const *virt_file, char const *spec, char **name_out) {
    cctx_t    *cctx     = cctx_create();
    size_t     spec_len = strlen(spec);
    srcfile_t *src      = srcfile_create(cctx, virt_file, spec, spec_len);

    c_macro_t     *macro    = strong_calloc(1, sizeof(c_macro_t));
    char          *name     = NULL;
    c_tokenizer_t *tkn      = NULL;
    size_t         args_cap = 0;

    size_t i = 0;

    // Skip leading whitespace.
    while (i < spec_len && isspace((unsigned char)spec[i])) {
        i++;
    }

    // Parse the macro name.
    if (i >= spec_len || !c_is_first_sym_char((unsigned char)spec[i])) {
        printf("%s:%zu: Expected macro name\n", virt_file, i);
        goto error;
    }
    size_t name_start = i;
    while (i < spec_len && c_is_sym_char((unsigned char)spec[i])) {
        i++;
    }
    size_t name_len = i - name_start;
    name            = strong_malloc(name_len + 1);
    memcpy(name, spec + name_start, name_len);
    name[name_len] = '\0';

    // Optional parameter list.
    if (i < spec_len && spec[i] == '(') {
        macro->uses_args = true;
        i++;
        bool need_arg = false;
        while (1) {
            while (i < spec_len && isspace((unsigned char)spec[i])) {
                i++;
            }
            if (i >= spec_len) {
                printf("%s:%zu: Unterminated parameter list\n", virt_file, i);
                goto error;
            }
            if (spec[i] == ')') {
                if (need_arg) {
                    printf("%s:%zu: Expected parameter name after ','\n", virt_file, i);
                    goto error;
                }
                i++;
                break;
            }
            if (i + 2 < spec_len && spec[i] == '.' && spec[i + 1] == '.' && spec[i + 2] == '.') {
                i                          += 3;
                macro->regular.is_variadic  = true;
                while (i < spec_len && isspace((unsigned char)spec[i])) {
                    i++;
                }
                if (i >= spec_len || spec[i] != ')') {
                    printf("%s:%zu: Expected ')' after '...'\n", virt_file, i);
                    goto error;
                }
                i++;
                break;
            }
            if (!c_is_first_sym_char((unsigned char)spec[i])) {
                printf("%s:%zu: Expected parameter name\n", virt_file, i);
                goto error;
            }
            size_t arg_start = i;
            while (i < spec_len && c_is_sym_char((unsigned char)spec[i])) {
                i++;
            }
            size_t arg_len = i - arg_start;
            char  *arg     = strong_malloc(arg_len + 1);
            memcpy(arg, spec + arg_start, arg_len);
            arg[arg_len] = '\0';
            array_lencap_insert_strong(
                &macro->regular.args,
                sizeof(char *),
                &macro->regular.args_len,
                &args_cap,
                &arg,
                macro->regular.args_len
            );
            while (i < spec_len && isspace((unsigned char)spec[i])) {
                i++;
            }
            if (i >= spec_len) {
                printf("%s:%zu: Unterminated parameter list\n", virt_file, i);
                goto error;
            }
            if (spec[i] == ',') {
                i++;
                need_arg = true;
                continue;
            }
            if (spec[i] == ')') {
                i++;
                break;
            }
            printf("%s:%zu: Expected ',' or ')' in parameter list\n", virt_file, i);
            goto error;
        }
    }

    // Skip whitespace before optional '='.
    while (i < spec_len && isspace((unsigned char)spec[i])) {
        i++;
    }

    if (i < spec_len) {
        if (spec[i] != '=') {
            printf("%s:%zu: Expected '=' or end of spec\n", virt_file, i);
            goto error;
        }
        i++;

        // Tokenize the body with a C tokenizer in preprocessor mode, starting
        // just past the '='.
        tkn               = c_tkn_create(src, C_STD_def);
        tkn->preproc_mode = true;
        tkn->base.pos.off = (off_t)i;
        tkn->base.pos.col = (int)i;
        if (!c_macro_parse_body(macro, &tkn->base)) {
            goto error;
        }
    }

    if (cctx->diagnostics.len) {
        dlist_foreach_node(diagnostic_t, d, &cctx->diagnostics) {
            print_diagnostic(d, stdout);
        }
        goto error;
    }

    if (tkn) {
        tkn_ctx_delete(&tkn->base);
    }
    cctx_delete(cctx);
    if (name_out) {
        *name_out = name;
    } else {
        free(name);
    }
    return macro;

error:
    if (cctx->diagnostics.len) {
        dlist_foreach_node(diagnostic_t, d, &cctx->diagnostics) {
            print_diagnostic(d, stdout);
        }
    }
    free(name);
    if (tkn) {
        tkn_ctx_delete(&tkn->base);
    }
    c_macro_destroy(macro);
    cctx_delete(cctx);
    return NULL;
}

// Create a procedural macro.
c_macro_t *c_proc_macro_create(bool uses_args, c_proc_macro_cb_t callback, void *cookie) {
    c_macro_t *macro     = strong_malloc(sizeof(c_macro_t));
    macro->is_proc_macro = true;
    macro->uses_args     = uses_args;
    macro->proc.callback = callback;
    macro->proc.cookie   = cookie;
    return macro;
}

// Destroy a macro.
void c_macro_destroy(c_macro_t *macro) {
    if (!macro->is_proc_macro) {
        for (size_t i = 0; i < macro->regular.args_len; i++) {
            free(macro->regular.args[i]);
        }
        free(macro->regular.args);
        for (size_t i = 0; i < macro->regular.subst_len; i++) {
            c_macro_subst_t *subst = &macro->regular.subst[i];
            if (subst->type == C_SUBST_TOKEN) {
                tkn_delete(subst->token);
            } else if (subst->type == C_SUBST_VA_OPT) {
                for (size_t x = 0; x < subst->va_opt.tokens_len; x++) {
                    tkn_delete(subst->va_opt.tokens[x]);
                }
                free(subst->va_opt.tokens);
            } else if (subst->type == C_SUBST_VA_OPT) {
                for (size_t j = 0; j < subst->va_opt.tokens_len; j++) {
                    tkn_delete(subst->va_opt.tokens[j]);
                }
                for (size_t j = 0; j < subst->va_opt.expanded_len; j++) {
                    tkn_delete(subst->va_opt.expanded[j]);
                }
                free(subst->va_opt.tokens);
                free(subst->va_opt.expanded);
                free(subst->va_opt.stringized);
                free(subst->va_opt.ws_before);
            }
        }
        free(macro->regular.subst);
    }
    free(macro);
}

// Paste two preprocessing tokens together.
static bool c_preproc_tkn_paste(c_preproc_t *pre, token_t const *lhs, token_t const *rhs, token_t *out) {
    size_t len = 0;
    size_t cap = 0;
    char  *buf = NULL;

    c_tkn_append_src(lhs, &buf, &len, &cap);
    c_tkn_append_src(rhs, &buf, &len, &cap);
    array_lencap_insert_strong(&buf, 1, &len, &cap, "", len);
    token_t tkn = (token_t){
        .pos        = pos_including(lhs->pos, rhs->pos),
        .type       = TOKENTYPE_IDENT,
        .strval     = buf,
        .strval_len = len - 1,
    };

    // In macro expansion, *placemarker* tokens may be created temporarily.
    // Lily-CC encodes these as empty identifiers.
    // Concatenations of two placemarkers returns another,
    // concatenations of a placemarker and another token return the non-placemarker.
    bool lhs_marker = lhs->type == TOKENTYPE_OTHER && lhs->subtype == C_TKN_MARKER;
    bool rhs_marker = rhs->type == TOKENTYPE_OTHER && rhs->subtype == C_TKN_MARKER;
    if (lhs_marker) {
        *out = tkn_clone(rhs);
        return true;
    } else if (rhs_marker) {
        *out = tkn_clone(lhs);
        return true;
    }

    // Check for symbolic tokens.
    for (int i = 0; i < C_N_TKNS; i++) {
        if (!strcmp(tkn.strval, c_token_name[i])) {
            tkn.type    = TOKENTYPE_OTHER;
            tkn.subtype = i;
            free(tkn.strval);
            tkn.strval     = NULL;
            tkn.strval_len = 0;
            *out           = tkn;
            return true;
        }
    }

    // On matching arms, the relevant lexical syntax specification is shown.
    // The following are checks in case `lhs` is a pp-number (C23 §6.4.9).
    bool lhs_numeric = lhs->type == TOKENTYPE_IDENT && lhs->strval[0] >= '0' && lhs->strval[0] <= '9';
    char lhs_last    = lhs->type == TOKENTYPE_IDENT ? lhs->strval[lhs->strval_len - 1] : 0;
    if (lhs_numeric && rhs->type == TOKENTYPE_IDENT) {
        // pp-number identifier-continue
        // pp-number digit
        // pp-number `.` digit
        *out = tkn;
        return true;
    }
    if (lhs_numeric && rhs->type == TOKENTYPE_OTHER && rhs->subtype == C_TKN_DOT) {
        // pp-number `.`
        *out = tkn;
        return true;
    }
    if (lhs_numeric && (lhs_last == 'e' || lhs_last == 'E' || lhs_last == 'p' || lhs_last == 'P')
        && rhs->type == TOKENTYPE_OTHER && (rhs->subtype == C_TKN_ADD || rhs->subtype == C_TKN_SUB)) {
        // pp-number `e` sign
        // pp-number `E` sign
        // pp-number `p` sign
        // pp-number `P` sign
        *out = tkn;
        return true;
    }

    // The following are checks in case `lhs` is an identifier (C23 §6.4.3.1).
    if (lhs->type == TOKENTYPE_IDENT && rhs->type == TOKENTYPE_IDENT) {
        // identifier identifier-continue
        *out = tkn;
        return true;
    }

    // If we get here, the resulting token is invalid.
    cctx_diagnostic(
        pre->shared->cctx,
        tkn.pos,
        DIAG_ERR,
        "Pasting would create `%s`, an invalid preprocessing token",
        tkn.strval
    );
    tkn_delete(tkn);
    return false;
}

// Stringize a function-like macro argument (`#` operator).
// Builds the source-form of the argument's tokens with a single space between
// tokens that were separated by whitespace in the original input, then escapes
// `\` and `"` for use inside a double-quoted preprocessor string token.
static token_t c_macro_arg_stringize(c_macro_arg_t *arg, pos_t pos) {
    if (arg->stringized == NULL) {
        // Build the unescaped source-form.
        size_t src_len = 0;
        size_t src_cap = 0;
        char  *src     = NULL;
        for (size_t i = 0; i < arg->tokens_len; i++) {
            if (i > 0 && arg->ws_before[i]) {
                char sp = ' ';
                array_lencap_insert_strong(&src, 1, &src_len, &src_cap, &sp, src_len);
            }
            c_tkn_append_src(&arg->tokens[i], &src, &src_len, &src_cap);
        }

        // Escape `\` and `"` and NUL-terminate. Preprocessor strings have C-string
        // syntax with unprocessed escape sequences, so any literal `\` or `"` in
        // the source-form must be escaped.
        size_t out_len = 0;
        size_t out_cap = 0;
        char  *out     = NULL;
        for (size_t i = 0; i < src_len; i++) {
            if (src[i] == '\\' || src[i] == '"') {
                char bs = '\\';
                array_lencap_insert_strong(&out, 1, &out_len, &out_cap, &bs, out_len);
            }
            array_lencap_insert_strong(&out, 1, &out_len, &out_cap, &src[i], out_len);
        }
        char nul = '\0';
        array_lencap_insert_strong(&out, 1, &out_len, &out_cap, &nul, out_len);
        free(src);

        // Cache the stringized value.
        arg->stringized = out;
    }

    return (token_t){
        .pos        = pos,
        .type       = TOKENTYPE_SCONST,
        .subtype    = C_STR_RAW_DQUOT,
        .strval     = strong_strdup(arg->stringized),
        .strval_len = strlen(arg->stringized),
    };
}

// Pre-expand a function-like macro argument.
// If it would expand to an empty sequence (or already is), then `expanded` is set to a single placemarker token.
static void c_macro_arg_preexpand(c_preproc_t *pre, c_macro_arg_t *arg) {
    if (arg->expanded_len) {
        // Already computed.
        return;
    }

    size_t       expanded_cap = 0;
    c_preproc_t *recurse      = c_preproc_create_nested(pre);
    recurse->keep_comments    = false;
    recurse->raw_mode         = true;
    recurse->no_directives    = true;

    // Note: EOF pos not important, just needs to be valid.
    tkn_array_t *arr_tkn = tkn_array_create(arg->tokens, arg->tokens_len, arg->pos);
    c_incfile_t  incfile = {
        .tkn_ctx   = &arr_tkn->base,
        .ifdir     = NULL,
        .ifdir_len = 0,
        .ifdir_cap = 0,
    };
    array_lencap_insert_strong(
        &recurse->stack,
        sizeof(c_incfile_t),
        &recurse->stack_len,
        &recurse->stack_cap,
        &incfile,
        0
    );

    while (1) {
        token_t tkn = tkn_next(&recurse->base);
        if (tkn.type == TOKENTYPE_EOF) {
            tkn_delete(tkn);
            break;
        } else if (tkn.type == TOKENTYPE_EOL || tkn.type == TOKENTYPE_WHITESPACE) {
            // Whitespace ignored here, added later in the macro pipeline.
            tkn_delete(tkn);
        } else {
            array_lencap_insert_strong(
                &arg->expanded,
                sizeof(token_t),
                &arg->expanded_len,
                &expanded_cap,
                &tkn,
                arg->expanded_len
            );
        }
    }

    tkn_ctx_delete(&recurse->base);

    // Empty result, replace with one placemarker.
    if (arg->expanded_len == 0) {
        token_t marker = {
            .pos     = arg->pos,
            .type    = TOKENTYPE_OTHER,
            .subtype = C_TKN_MARKER,
        };
        array_lencap_insert_strong(
            &arg->expanded,
            sizeof(token_t),
            &arg->expanded_len,
            &expanded_cap,
            &marker,
            arg->expanded_len
        );
    }
}

// Perform macro-expansion.
static void c_macro_expand(c_preproc_t *pre, pos_t pos, c_macro_t const *macro) {
    size_t         args_len = 0;
    size_t         args_cap = 0;
    c_macro_arg_t *args     = NULL;
    // TODO: Add expansion info to the position.

    if (macro->uses_args) {
        // Consume the opening `(` (already peeked by the caller).
        tkn_delete(c_preproc_raw_next(pre));

        // Skip whitespace and newlines before checking for an empty argument list.
        while (1) {
            token_t p = c_preproc_raw_peek(pre);
            if (p.type != TOKENTYPE_WHITESPACE && p.type != TOKENTYPE_EOL) {
                break;
            }
            tkn_delete(c_preproc_raw_next(pre));
        }

        token_t peek = c_preproc_raw_peek(pre);
        if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_RPAR) {
            // `()` is treated as zero arguments.
            tkn_delete(c_preproc_raw_next(pre));
        } else {
            // Collect tokens for each argument. Parentheses nest, and at depth > 0
            // commas are part of the argument rather than separators.
            int      depth      = 0;
            size_t   cur_len    = 0;
            size_t   cur_cap    = 0;
            token_t *cur        = NULL;
            size_t   cur_ws_cap = 0;
            bool    *cur_ws     = NULL;
            bool     saw_ws     = false;
            bool     has_pos    = false;
            size_t   lim        = SIZE_MAX;
            pos_t    pos; // Start position of the argument.
            if (!macro->is_proc_macro && macro->regular.is_variadic) {
                lim = macro->regular.args_len;
            }

            while (1) {
                token_t p = c_preproc_raw_peek(pre);
                if (!has_pos) {
                    pos     = p.pos;
                    has_pos = true;
                }
                if (p.type == TOKENTYPE_EOF) {
                    cctx_diagnostic(
                        pre->shared->cctx,
                        p.pos,
                        DIAG_ERR,
                        "Unexpected end of file in macro argument list"
                    );
                    for (size_t i = 0; i < cur_len; i++) {
                        tkn_delete(cur[i]);
                    }
                    free(cur);
                    free(cur_ws);
                    goto exit;
                }

                // Whitespace and newlines do not appear in stored argument tokens,
                // but we record that some appeared before the next token.
                if (p.type == TOKENTYPE_WHITESPACE || p.type == TOKENTYPE_EOL) {
                    tkn_delete(c_preproc_raw_next(pre));
                    saw_ws = true;
                    continue;
                }

                // At depth 0, `)` ends the argument list and `,` separates arguments.
                // Once we have collected enough regular args, commas inside the
                // variadic tail are kept verbatim, but `)` still ends the list.
                if (depth == 0 && p.type == TOKENTYPE_OTHER
                    && (p.subtype == C_TKN_RPAR || (p.subtype == C_TKN_COMMA && args_len < lim))) {
                    bool is_end = p.subtype == C_TKN_RPAR;
                    tkn_delete(c_preproc_raw_next(pre));
                    c_macro_arg_t arg = {
                        .pos        = pos_between(pos, p.pos),
                        .tokens_len = cur_len,
                        .tokens     = cur,
                        .ws_before  = cur_ws,
                    };
                    array_lencap_insert_strong(&args, sizeof(c_macro_arg_t), &args_len, &args_cap, &arg, args_len);
                    if (is_end) {
                        break;
                    }
                    cur        = NULL;
                    cur_len    = 0;
                    cur_cap    = 0;
                    cur_ws     = NULL;
                    cur_ws_cap = 0;
                    saw_ws     = false;
                    has_pos    = false;
                    continue;
                }

                token_t t = c_preproc_raw_next(pre);
                if (t.type == TOKENTYPE_OTHER) {
                    if (t.subtype == C_TKN_LPAR) {
                        depth++;
                    } else if (t.subtype == C_TKN_RPAR) {
                        depth--;
                    }
                }
                bool   ws     = cur_len > 0 && saw_ws;
                size_t ws_len = cur_len;
                saw_ws        = false;
                array_lencap_insert_strong(&cur_ws, sizeof(bool), &ws_len, &cur_ws_cap, &ws, ws_len);
                array_lencap_insert_strong(&cur, sizeof(token_t), &cur_len, &cur_cap, &t, cur_len);
            }

            if (!macro->is_proc_macro && macro->regular.is_variadic && args_len == macro->regular.args_len) {
                // Insert empty dummy arg for `__VA_ARGS__`.
                c_macro_arg_t dummy = {
                    .pos = pos,
                };
                array_lencap_insert_strong(&args, sizeof(c_macro_arg_t), &args_len, &args_cap, &dummy, args_len);
            }
        }
    }

    // This is the actual expansion code.
    c_expansion_t expand = {0};
    if (macro->is_proc_macro) {
        expand = macro->proc.callback(pre, args, args_len, macro->proc.cookie);
    } else {
        if (macro->regular.is_variadic) {
            if (args_len < macro->regular.args_len) {
                cctx_diagnostic(
                    pre->shared->cctx,
                    pos,
                    DIAG_ERR,
                    "Macro expects at least %zu argument%s, got %zu",
                    macro->regular.args_len,
                    macro->regular.args_len == 1 ? "" : "s",
                    args_len
                );
                goto exit;
            }
        } else {
            if (args_len != macro->regular.args_len) {
                cctx_diagnostic(
                    pre->shared->cctx,
                    pos,
                    DIAG_ERR,
                    "Macro expects exactly %zu argument%s, got %zu",
                    macro->regular.args_len,
                    macro->regular.args_len == 1 ? "" : "s",
                    args_len
                );
                goto exit;
            }
        }
        for (size_t i = 0; i < macro->regular.subst_len; i++) {
            c_macro_subst_t *subst = &macro->regular.subst[i];
            // Fast path for literal tokens.
            if (subst->type == C_SUBST_TOKEN) {
                token_t tkn = tkn_clone(&subst->token);
                tkn.pos     = pos;
                array_lencap_insert_strong(
                    &expand.tokens,
                    sizeof(token_t),
                    &expand.tokens_len,
                    &expand.tokens_cap,
                    &tkn,
                    expand.tokens_len
                );
                continue;
            }

            // Get the matching argument ptr.
            c_macro_arg_t *arg;
            if (subst->type == C_SUBST_ARG) {
                arg = &args[subst->arg_index];
            } else if (subst->type == C_SUBST_VA_ARGS) {
                arg = &args[args_len - 1];
            } else if (subst->type == C_SUBST_VA_OPT) {
                arg = &subst->va_opt;
            } else {
                abort();
            }

            if (subst->stringize) {
                // Stringization: turn the argument's tokens into a single string literal.
                token_t tkn = c_macro_arg_stringize(arg, pos);
                array_lencap_insert_strong(
                    &expand.tokens,
                    sizeof(token_t),
                    &expand.tokens_len,
                    &expand.tokens_cap,
                    &tkn,
                    expand.tokens_len
                );
            } else {
                // This is a macro argument.
                token_t const *tokens;
                size_t         tokens_len;
                if (subst->pasting) {
                    tokens     = arg->tokens;
                    tokens_len = arg->tokens_len;
                } else {
                    c_macro_arg_preexpand(pre, arg);
                    tokens     = arg->expanded;
                    tokens_len = arg->expanded_len;
                }

                for (size_t x = 0; x < tokens_len; x++) {
                    token_t tkn = tkn_clone(&tokens[x]);
                    if (tkn.type == TOKENTYPE_OTHER && tkn.subtype == C_TKN_PASTE) {
                        // Same value but doesn't cause pasting.
                        tkn.subtype = C_TKN_ESCPASTE;
                    }
                    tkn.pos = pos;
                    array_lencap_insert_strong(
                        &expand.tokens,
                        sizeof(token_t),
                        &expand.tokens_len,
                        &expand.tokens_cap,
                        &tkn,
                        expand.tokens_len
                    );
                }
            }
        }
    }
    expand.index = 0;
    expand.macro = macro;

    // Handle pasting.
    for (size_t i = 1; i + 1 < expand.tokens_len;) {
        if (expand.tokens[i].type != TOKENTYPE_OTHER || expand.tokens[i].subtype != C_TKN_PASTE) {
            i++;
            continue;
        }

        token_t res;
        if (!c_preproc_tkn_paste(pre, &expand.tokens[i - 1], &expand.tokens[i + 1], &res)) {
            // On fail, delete just the offending `##`.
            tkn_delete(expand.tokens[i]);
            array_copy(expand.tokens, expand.tokens, sizeof(token_t), i, i + 1, expand.tokens_len - (i + 1));
            expand.tokens_len--;
            i++;
            continue;
        }
        tkn_delete(expand.tokens[i - 1]);
        tkn_delete(expand.tokens[i]);
        tkn_delete(expand.tokens[i + 1]);
        expand.tokens[i - 1] = res;
        array_copy(expand.tokens, expand.tokens, sizeof(token_t), i, i + 2, expand.tokens_len - (i + 2));
        expand.tokens_len -= 2;

        // Don't increment `i`; if another `##` occurs, it will now be at `expand.tokens[i]`.
    }

    // Remove placemarker tokens.
    if (macro->uses_args) {
        for (size_t i = 0; i < expand.tokens_len;) {
            if (expand.tokens[i].type == TOKENTYPE_OTHER && expand.tokens[i].subtype == C_TKN_MARKER) {
                tkn_delete(expand.tokens[i]);
                expand.tokens_len--;
            } else {
                i++;
            }
        }
    }

    // Insert whitespace between tokens.
    if (expand.tokens_len >= 2) {
        size_t   extra = expand.tokens_len - 1;
        token_t *out   = strong_calloc(expand.tokens_len + extra, sizeof(token_t));
        for (size_t i = 0; i < expand.tokens_len; i++) {
            out[i * 2] = expand.tokens[i];
        }
        for (size_t i = 0; i < extra; i++) {
            out[i * 2 + 1] = (token_t){
                .pos        = pos,
                .type       = TOKENTYPE_WHITESPACE,
                .strval     = strong_strdup(" "),
                .strval_len = 1,
            };
        }
        free(expand.tokens);
        expand.tokens_len += extra;
        expand.tokens      = out;
    }

    // If it succeeded, the expanded tokens are put on the stack.
    array_lencap_insert_strong(
        &pre->expand,
        sizeof(c_expansion_t),
        &pre->expand_len,
        &pre->expand_cap,
        &expand,
        pre->expand_len
    );

exit:
    for (size_t i = 0; i < args_len; i++) {
        for (size_t j = 0; j < args[i].tokens_len; j++) {
            tkn_delete(args[i].tokens[j]);
        }
        for (size_t j = 0; j < args[i].expanded_len; j++) {
            tkn_delete(args[i].expanded[j]);
        }
        free(args[i].tokens);
        free(args[i].expanded);
        free(args[i].stringized);
        free(args[i].ws_before);
    }
    free(args);
}

#pragma endregion macros
