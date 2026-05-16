
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



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
static void    c_preproc_pragma(c_preproc_t *pre, pos_t pos, char const *pragma);
static void    c_pragma_once(c_preproc_t *pre, pos_t pos, char const *args);
static void    c_incfile_push(c_preproc_t *pre, pos_t pos, char const *path);
static void    c_incfile_pop(c_preproc_t *pre);
static void    c_incfile_eof(c_preproc_t *pre);
static int     c_preproc_op_precedence(c_tokentype_t type);
static bool    c_preproc_is_prefix_op(c_tokentype_t type);
static i128_t  c_preproc_eval_prefix(c_tokentype_t oper, i128_t value);
static i128_t  c_preproc_eval_infix(bool is_signed, i128_t lhs, c_tokentype_t oper, i128_t rhs);
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
static void    c_macro_expand(c_preproc_t *pre, pos_t pos, c_macro_t const *macro);



static c_expansion_t c_proc_macro_counter(c_preproc_t *pre, token_t const *args, size_t args_len, void *cookie) {
    (void)cookie;
    (void)args;
    (void)args_len;

    int cap = snprintf(NULL, 0, "%" PRIu64, pre->counter_macro);
    if (cap < 0) {
        perror("snprintf");
        abort();
    }
    cap       += 1;
    char *buf  = malloc(cap);
    int   len  = snprintf(buf, cap, "%" PRIu64, pre->counter_macro);
    pre->counter_macro++;

    token_t tkn = {
        .pos        = {0},
        .type       = TOKENTYPE_IDENT,
        .strval     = buf,
        .strval_len = len,
    };
    c_expansion_t expand = {
        .tokens_len = 1,
        .tokens     = strong_malloc(sizeof(token_t)),
    };
    *expand.tokens = tkn;

    return expand;
}

// Create an empty preprocessor.
// `cc` must be valid for at least as long as the resulting preprocessor.
c_preproc_t *c_preproc_create(srcfile_t *srcfile, int c_std, bool raw_mode, bool keep_comments) {
    c_preproc_t *pre = strong_calloc(1, sizeof(c_preproc_t));

    c_tokenizer_t *srctok = c_tkn_create(srcfile, c_std);
    if (!srctok) {
        free(pre);
        return NULL;
    }
    srctok->preproc_mode  = true;
    srctok->keep_comments = keep_comments;

    // Note: `base` has a `pos` and `file`, but we do not use either.
    pre->base.next          = c_preproc_next;
    pre->base.cleanup       = c_preproc_destroy;
    pre->cctx               = srcfile->ctx;
    pre->macros             = STR_MAP_EMPTY;
    pre->once_files         = PTR_SET_EMPTY;
    pre->stack_len          = 1;
    pre->stack_cap          = 1;
    pre->stack              = strong_malloc(sizeof(c_incfile_t));
    pre->stack[0].tkn_ctx   = srctok;
    pre->stack[0].ifdir     = NULL;
    pre->stack[0].ifdir_len = 0;
    pre->stack[0].ifdir_cap = 0;
    pre->c_std              = c_std;
    pre->blank_line         = true;
    pre->raw_mode           = raw_mode;
    pre->keep_comments      = keep_comments;

    c_macro_t *counter_macro = c_proc_macro_create(false, c_proc_macro_counter, NULL);
    c_preproc_predef_macro(pre, "__COUNTER__", counter_macro);

    return pre;
}

// Destroy a preprocessor.
static void c_preproc_destroy(tokenizer_t *tkn) {
    c_preproc_t *pre = (c_preproc_t *)tkn;

    map_foreach(ent, &pre->macros) {
        c_macro_destroy(ent->value);
    }
    map_clear(&pre->macros);

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

    // Note: The source files are kept alive by the associated cctx_t.
    free(pre->files);
}

// Pragma: once.
static void c_pragma_once(c_preproc_t *pre, pos_t pos, char const *args) {
    if (*args) {
        cctx_diagnostic(pre->cctx, pos, DIAG_WARN, "Extra tokens after #pragma once");
    }
    c_incfile_t *file = &pre->stack[pre->stack_len - 1];
    set_add(&pre->once_files, file->tkn_ctx->base.file);
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
        cctx_diagnostic(pre->cctx, pos, DIAG_WARN, "Unrecognized pragma: %.*s", (int)name_len, pragma);
    }
}

// Search for an include file and push it into the include stack.
static void c_incfile_push(c_preproc_t *pre, pos_t pos, char const *path) {
    // TODO: Replace with _popen when include search paths are implemented.
    srcfile_t *file = srcfile_open(pre->cctx, path);
    if (!file) {
        cctx_diagnostic(pre->cctx, pos, DIAG_ERR, "Cannot open include file: %s", path);
        return;
    }

    if (set_contains(&pre->once_files, file)) {
        // A `#pragma once` for this file already occurred.
        return;
    }

    // TODO: Add include stack info to the tokenizer's position.
    c_incfile_t incfile = {
        .tkn_ctx   = c_tkn_create(file, pre->c_std),
        .ifdir_cap = 0,
        .ifdir_len = 0,
        .ifdir     = 0,
    };
    incfile.tkn_ctx->preproc_mode = true;
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

    tkn_ctx_delete(&incfile->tkn_ctx->base);
    pre->stack_len--;
}

// Do end-of-file checks for top-most file of the include stack.
static void c_incfile_eof(c_preproc_t *pre) {
    assert(pre->stack_len >= 1);
    c_incfile_t *incfile = &pre->stack[pre->stack_len - 1];

    for (size_t i = 0; i < incfile->ifdir_len; i++) {
        cctx_diagnostic(
            pre->cctx,
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
        token_t tkn = c_preproc_get_tkn(pre, LINE_NOWS_EXPAND);                                                        \
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
                peek.value.value     = int128(0, pre->c_std >= C_STD_C23 && !strcmp(tkn.strval, "true"));              \
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
        cctx_diagnostic(pre->cctx, pos, DIAG_ERR, "Expected preprocessor expression");
        return 0;
    } else if (stack_len == 1 && stack[0].type == ENTRY_VALUE) {
        return cmp128u(stack[0].value.value, int128(0, 0)) != 0;
    } else {
        cctx_diagnostic(
            pre->cctx,
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
    c_incfile_t *file            = &pre->stack[pre->stack_len - 1];
    file->tkn_ctx->str_anglebrac = true;
    token_t token                = c_preproc_get_tkn(pre, LINE_NOWS_EXPAND);
    file->tkn_ctx->str_anglebrac = false;
    if (token.type != TOKENTYPE_SCONST) {
        cctx_diagnostic(pre->cctx, token.pos, DIAG_ERR, "Expected a path");
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
            cctx_diagnostic(pre->cctx, tkn.pos, DIAG_ERR, "Expected an identifier");
            return;
        }
        eval = map_get(&pre->macros, tkn.strval) != NULL;
        if (ifndef) {
            eval = !eval;
        }
    } else {
        eval = c_preproc_eval(pre, pos);
    }

    bool parent_emit = file->ifdir_len < 2 || file->ifdir[file->ifdir_len - 2].do_emit;
    if (elif) {
        if (!file->ifdir_len) {
            cctx_diagnostic(pre->cctx, pos, DIAG_ERR, "#elif without matching #if");
            return;
        }
        c_ifdir_t *ifdir = &file->ifdir[file->ifdir_len - 1];
        if (!ifdir->allow_else) {
            cctx_diagnostic(pre->cctx, pos, DIAG_ERR, "#elif without matching #if");
            cctx_diagnostic(pre->cctx, ifdir->pos, DIAG_INFO, "Most recent #else directive");
            cctx_diagnostic(pre->cctx, pos, DIAG_HINT, "Change the #elif into an #if");
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
        cctx_diagnostic(pre->cctx, pos, DIAG_ERR, "#else without matching #if");
        return;
    }
    c_ifdir_t *ifdir       = &file->ifdir[file->ifdir_len - 1];
    bool       parent_emit = file->ifdir_len < 2 || file->ifdir[file->ifdir_len - 2].do_emit;

    if (!ifdir->allow_else) {
        cctx_diagnostic(pre->cctx, pos, DIAG_ERR, "Dangling #else");
        cctx_diagnostic(pre->cctx, ifdir->pos, DIAG_INFO, "Earlier #else occurred here");
        cctx_diagnostic(pre->cctx, pos, DIAG_HINT, "Write #endif here");
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
        cctx_diagnostic(pre->cctx, pos, DIAG_ERR, "#endif without matching #if");
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
    cctx_diagnostic(pre->cctx, span, is_error ? DIAG_ERR : DIAG_WARN, "%s", buf);
    free(buf);
}

// Preprocessor directive: define.
static void c_directive_define(c_preproc_t *pre, pos_t pos) {
    (void)pos;
    token_t name = c_preproc_get_tkn(pre, LINE_NOWS_RAW);
    if (name.type == TOKENTYPE_EOL) {
        cctx_diagnostic(pre->cctx, name.pos, DIAG_ERR, "Expected macro name");
        return;
    } else if (name.type != TOKENTYPE_IDENT) {
        cctx_diagnostic(pre->cctx, name.pos, DIAG_ERR, "Macro name must be an identifier");
        return;
    }

    token_t *tokens     = NULL;
    size_t   tokens_len = 0;
    size_t   tokens_cap = 0;
    char   **params     = NULL;
    size_t   params_len = 0;
    size_t   params_cap = 0;
    bool     variadic   = false;

    token_t tkn = c_preproc_get_tkn(pre, LINE_RAW);
    if (tkn.type == TOKENTYPE_WHITESPACE) {
        // Simple macro.
        tkn_delete(tkn);
        tkn = c_preproc_get_tkn(pre, LINE_NOWS_RAW);
    } else if (tkn.type == TOKENTYPE_OTHER && tkn.subtype == C_TKN_LPAR) {
        // Collect parameter list.
        pos_t list_start = tkn.pos;
        tkn_delete(tkn);
        bool need_arg   = false;
        bool must_close = false;
        while (1) {
            tkn = c_preproc_get_tkn(pre, LINE_NOWS_RAW);
            if (tkn.type == TOKENTYPE_EOL) {
                // Abrupt end of line.
                cctx_diagnostic(pre->cctx, list_start, DIAG_ERR, "Missing `(` to match this `)`");
                tkn_delete(tkn);
                goto error;
            } else if (!need_arg && tkn.type == TOKENTYPE_OTHER && tkn.subtype == C_TKN_RPAR) {
                // Empty parameter list.
                tkn_delete(tkn);
                break;
            } else if (tkn.type == TOKENTYPE_OTHER && tkn.subtype == C_TKN_VARARG) {
                // Elipsis (`...`).
                variadic   = true;
                must_close = true;
                tkn_delete(tkn);
            } else if (tkn.type == TOKENTYPE_IDENT) {
                if (!strcmp(tkn.strval, "__VA_ARGS__") || !strcmp(tkn.strval, "__VA_OPT__")) {
                    cctx_diagnostic(
                        pre->cctx,
                        tkn.pos,
                        DIAG_WARN,
                        "Macro parameters should not be called %s",
                        tkn.strval
                    );
                }
                array_lencap_insert_strong(&params, sizeof(token_t), &params_len, &params_cap, &tkn, params_len);
            } else {
                cctx_diagnostic(pre->cctx, tkn.pos, DIAG_ERR, "Expected an identifier or `...`");
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
                        pre->cctx,
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

    // Collect all tokens after parameter list.
    while (1) {
        if (tokens_len == 0 && tkn.type == TOKENTYPE_OTHER && tkn.subtype == C_TKN_PASTE) {
            cctx_diagnostic(
                pre->cctx,
                tkn.pos,
                DIAG_ERR,
                "`##` is not allowed at the start/end of macro expansion lists"
            );
            goto error;
        }
        if (tkn.type == TOKENTYPE_EOL) {
            tkn_delete(tkn);
            break;
        }
        if (!variadic && tkn.type == TOKENTYPE_IDENT
            && (!strcmp(tkn.strval, "__VA_ARGS__") || !strcmp(tkn.strval, "__VA_OPT__"))) {
            cctx_diagnostic(pre->cctx, tkn.pos, DIAG_WARN, "%s in non-variadic macro", tkn.strval);
        }
        if (tkn.type == TOKENTYPE_IDENT) {
            for (size_t i = 0; i < params_len; i++) {
                if (!strcmp(tkn.strval, params[i])) {
                    tkn_delete(tkn);
                    tkn = (token_t){
                        .pos  = tkn.pos,
                        .type = TOKENTYPE_IDENT,
                        .ival = i,
                    };
                    break;
                }
            }
        }
        array_lencap_insert_strong(&tokens, sizeof(token_t), &tokens_len, &tokens_cap, &tkn, tokens_len);
        tkn = c_preproc_get_tkn(pre, LINE_NOWS_RAW);
    }

    if (tokens_len && tokens[tokens_len - 1].type == TOKENTYPE_OTHER && tokens[tokens_len - 1].subtype == C_TKN_PASTE) {
        cctx_diagnostic(
            pre->cctx,
            tokens[tokens_len - 1].pos,
            DIAG_ERR,
            "`##` is not allowed at the start/end of macro expansion lists"
        );
        goto error;
    }

    c_macro_t *existing = map_get(&pre->macros, name.strval);
    if (existing) {
        cctx_diagnostic(
            pre->cctx,
            name.pos,
            DIAG_WARN,
            "Redefinition of %smacro `%s`",
            existing->is_builtin ? "built-in " : "",
            name.strval
        );
        c_macro_destroy(existing);
    }

    c_macro_t *macro          = strong_calloc(1, sizeof(c_macro_t));
    macro->is_proc_macro      = false;
    macro->is_builtin         = false;
    macro->regular.variadic   = variadic;
    macro->regular.tokens     = tokens;
    macro->regular.tokens_len = tokens_len;
    map_set(&pre->macros, name.strval, macro);
    return;

error:
    free(tokens);
    free(params);
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
        cctx_diagnostic(pre->cctx, name.pos, DIAG_ERR, "Expected macro name");
        return;
    } else if (name.type != TOKENTYPE_IDENT) {
        cctx_diagnostic(pre->cctx, name.pos, DIAG_ERR, "Macro name must be an identifier");
        return;
    }

    c_macro_t *macro = map_get(&pre->macros, name.strval);
    if (!macro) {
        // Nothing to do.
        return;
    }

    if (macro->is_builtin) {
        cctx_diagnostic(pre->cctx, name.pos, DIAG_WARN, "Undefining a built-in macro");
    }
    map_remove(&pre->macros, name.strval);
    c_macro_destroy(macro);
}

// Handle a preprocessor directive.
static void c_preproc_directive(c_preproc_t *pre) {
    token_t name = c_preproc_get_tkn(pre, LINE_NOWS_RAW);
    if (name.type == TOKENTYPE_EOL || name.type == TOKENTYPE_EOF) {
        // Note: Counterintuitively, `#` followed by newline does nothing according to the spec.
        return;
    } else if (name.type != TOKENTYPE_IDENT) {
        cctx_diagnostic(pre->cctx, name.pos, DIAG_ERR, "Expected preprocessing directive name");
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
        token_t peek = tkn_peek(&file->tkn_ctx->base);
        if (peek.type == TOKENTYPE_EOL || peek.type == TOKENTYPE_EOF) {
            break;
        }
        token_t tkn = tkn_next(&file->tkn_ctx->base);
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
        cctx_diagnostic(pre->cctx, extra, DIAG_WARN, "Extra tokens ignored");
    }
}

// Whether the current if/else directive's code is active.
// Always true if no if/else directive currently exists.
static bool c_preproc_do_emit(c_preproc_t *pre) {
    c_incfile_t *file = &pre->stack[pre->stack_len - 1];
    return file->ifdir_len == 0 || file->ifdir[file->ifdir_len - 1].do_emit;
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
        token_t peek = tkn_peek(&pre->stack[i].tkn_ctx->base);
        if (peek.type != TOKENTYPE_EOF) {
            return peek;
        }
    }

    // Check source file.
    return tkn_peek(&pre->stack[0].tkn_ctx->base);
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
        tkn = tkn_next(&pre->stack[pre->stack_len - 1].tkn_ctx->base);
        if (tkn.type == TOKENTYPE_EOF) {
            tkn_delete(tkn);
        } else {
            goto emit;
        }
        c_incfile_eof(pre);
        c_incfile_pop(pre);
    }

    // Check source file; if it EOFs, just return it verbatim.
    tkn = tkn_next(&pre->stack[0].tkn_ctx->base);

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
    c_macro_t const *macro = map_get(&pre->macros, tkn.strval);
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
        peek = c_preproc_raw_peek(pre);
        if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_LPAR) {
            return tkn;
        }
    }
    c_macro_expand(pre, tkn.pos, macro);
    tkn_delete(tkn);
    goto again;
}

// Get the next preprocessed token.
token_t c_preproc_next(tokenizer_t *ctx) {
    c_preproc_t *pre = (c_preproc_t *)ctx;

again:
    if (!pre->blank_line) {
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
        return c_preproc_get_tkn(pre, NEXT_EXPAND);
    } else {
        do {
            tkn_delete(c_preproc_get_tkn(pre, NEXT_RAW));
        } while (!pre->blank_line);
        goto again;
    }
}

// Convert a preprocessor token to a C token.
token_t c_preproc_tkn_to_c_tkn(c_preproc_t *pre, token_t tkn);

// Add a pre-defined macro.
void c_preproc_predef_macro(c_preproc_t *pre, char const *name, c_macro_t *macro) {
    map_set(&pre->macros, name, macro);
}

// Create a regular macro.
c_macro_t *c_macro_create(char const *virt_file, char const *spec, char **name_out) {
    cctx_t    *cctx     = cctx_create();
    size_t     spec_len = strlen(spec);
    srcfile_t *src      = srcfile_create(cctx, virt_file, spec, spec_len);

    c_macro_t     *macro      = strong_calloc(1, sizeof(c_macro_t));
    char          *name       = NULL;
    c_tokenizer_t *tkn        = NULL;
    size_t         args_cap   = 0;
    size_t         tokens_cap = 0;

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
                i                       += 3;
                macro->regular.variadic  = true;
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
        while (1) {
            token_t t = tkn_next(&tkn->base);
            if (t.type == TOKENTYPE_WHITESPACE) {
                tkn_delete(t);
                continue;
            }
            if (t.type == TOKENTYPE_EOF) {
                tkn_delete(t);
                break;
            }
            if (t.type == TOKENTYPE_IDENT) {
                for (size_t i = 0; i < macro->regular.args_len; i++) {
                    if (!strcmp(t.strval, macro->regular.args[i])) {
                        tkn_delete(t);
                        // Ident but instead of strval it has an index, this will be replaced by the macro expander.
                        t = (token_t){
                            .pos  = t.pos,
                            .type = TOKENTYPE_IDENT,
                            .ival = i,
                        };
                        break;
                    }
                }
            }
            array_lencap_insert_strong(
                &macro->regular.tokens,
                sizeof(token_t),
                &macro->regular.tokens_len,
                &tokens_cap,
                &t,
                macro->regular.tokens_len
            );
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
    *name_out = name;
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
    c_macro_t *macro      = strong_malloc(sizeof(c_macro_t));
    macro->is_proc_macro  = true;
    macro->proc.uses_args = uses_args;
    macro->proc.callback  = callback;
    macro->proc.cookie    = cookie;
    return macro;
}

// Destroy a macro.
void c_macro_destroy(c_macro_t *macro) {
    if (!macro->is_proc_macro) {
        for (size_t i = 0; i < macro->regular.args_len; i++) {
            free(macro->regular.args[i]);
        }
        free(macro->regular.args);
        for (size_t i = 0; i < macro->regular.tokens_len; i++) {
            tkn_delete(macro->regular.tokens[i]);
        }
        free(macro->regular.tokens);
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
    bool lhs_marker = lhs->type == TOKENTYPE_IDENT && lhs->strval_len == 0;
    bool rhs_marker = rhs->type == TOKENTYPE_IDENT && rhs->strval_len == 0;
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
        pre->cctx,
        tkn.pos,
        DIAG_ERR,
        "Pasting would create `%s`, an invalid preprocessing token",
        tkn.strval
    );
    tkn_delete(tkn);
    return false;
}

// Perform macro-expansion.
static void c_macro_expand(c_preproc_t *pre, pos_t pos, c_macro_t const *macro) {
    size_t   args_len = 0;
    size_t   args_cap = 0;
    token_t *args     = NULL;

    if (macro->uses_args) {
        // TODO: Arguments, parsement.
    }

    // This is the actual expansion code.
    c_expansion_t expand;
    if (macro->is_proc_macro) {
        expand = macro->proc.callback(pre, args, args_len, macro->proc.cookie);
    } else {
        if (args_len != macro->regular.args_len) {
            cctx_diagnostic(
                pre->cctx,
                pos,
                DIAG_ERR,
                "Macro expects %zu argument%s, got %zu",
                macro->regular.args_len,
                macro->regular.args_len == 1 ? "" : "s",
                args_len
            );
            goto exit;
        }
        expand.tokens_len = macro->regular.tokens_len;
        expand.tokens     = strong_calloc(expand.tokens_len, sizeof(token_t));
        for (size_t i = 0; i < macro->regular.tokens_len; i++) {
            if (macro->regular.tokens[i].type == TOKENTYPE_IDENT && macro->regular.tokens[i].strval == NULL) {
                // This is a macro argument.
                expand.tokens[i] = tkn_clone(&args[macro->regular.tokens[i].ival]);
            } else {
                // Regular token to expand.
                expand.tokens[i] = tkn_clone(&macro->regular.tokens[i]);
            }
            // TODO: Add expansion info to the position.
            expand.tokens[i].pos = pos;
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
            if (expand.tokens[i].type == TOKENTYPE_IDENT && expand.tokens[i].strval_len == 0) {
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
        tkn_delete(args[i]);
    }
    free(args);
}
