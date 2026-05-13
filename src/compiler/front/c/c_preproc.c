
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_preproc.h"

#include "arrays.h"
#include "c_tokenizer.h"
#include "compiler.h"
#include "map.h"
#include "set.h"
#include "strong_malloc.h"
#include "tokenizer.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>



static void    c_preproc_destroy(tokenizer_t *tkn);
static void    c_preproc_pragma(c_preproc_t *pre, pos_t pos, char const *pragma);
static void    c_pragma_once(c_preproc_t *pre, pos_t pos, char const *args);
static void    c_incfile_push(c_preproc_t *pre, pos_t pos, char const *path);
static void    c_incfile_pop(c_preproc_t *pre);
static void    c_incfile_eof(c_preproc_t *pre);
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
static token_t c_preproc_next_raw(c_preproc_t *pre, bool skip_whitespace, bool skip_eol, bool allow_next_line);
static token_t c_preproc_next_expanded(c_preproc_t *pre);
static void    c_preproc_directive(c_preproc_t *pre);



// Create an empty preprocessor.
// `cc` must be valid for at least as long as the resulting preprocessor.
c_preproc_t *c_preproc_create(srcfile_t *srcfile, int c_std) {
    c_preproc_t *pre = strong_calloc(1, sizeof(c_preproc_t));

    c_tokenizer_t *srctok = c_tkn_create(srcfile, c_std);
    if (!srctok) {
        free(pre);
        return NULL;
    }
    srctok->preproc_mode = true;

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

    return pre;
}

// Destroy a preprocessor.
static void c_preproc_destroy(tokenizer_t *tkn) {
    c_preproc_t *pre = (c_preproc_t *)tkn;

    map_foreach(ent, &pre->macros) {
        c_macro_destroy(ent->value);
    }
    map_clear(&pre->macros);

    for (size_t i = pre->expand_index; i < pre->expand_len; i++) {
        tkn_delete(pre->expand[i]);
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

// Preprocessor directive: include.
static void c_directive_include(c_preproc_t *pre, pos_t pos) {
    c_incfile_t *file            = &pre->stack[pre->stack_len - 1];
    file->tkn_ctx->str_anglebrac = true;
    token_t token                = c_preproc_next_raw(pre, true, false, false);
    file->tkn_ctx->str_anglebrac = false;
    if (token.type != TOKENTYPE_SCONST) {
        cctx_diagnostic(pre->cctx, token.pos, DIAG_ERR, "Expected a path");
        tkn_delete(token);
        return;
    }

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
        token_t tkn = c_preproc_next_raw(pre, true, false, false);
        if (tkn.type != TOKENTYPE_IDENT) {
            cctx_diagnostic(pre->cctx, tkn.pos, DIAG_ERR, "Expected an identifier");
            return;
        }
        eval = map_get(&pre->macros, tkn.strval) != NULL;
        if (ifndef) {
            eval = !eval;
        }
    } else {
        // TODO.
        eval = false;
    }

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
        ifdir->do_emit   = !ifdir->disabled && eval;
        ifdir->disabled |= ifdir->do_emit;
    } else {
        c_ifdir_t ifdir = {
            .pos        = pos,
            .allow_else = true,
            .disabled   = eval,
            .do_emit    = eval,
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
    c_ifdir_t *ifdir = &file->ifdir[file->ifdir_len - 1];

    if (!ifdir->allow_else) {
        cctx_diagnostic(pre->cctx, pos, DIAG_ERR, "Dangling #else");
        cctx_diagnostic(pre->cctx, ifdir->pos, DIAG_INFO, "Earlier #else occurred here");
        cctx_diagnostic(pre->cctx, pos, DIAG_HINT, "Write #endif here");
        return;
    }

    ifdir->pos        = pos;
    ifdir->allow_else = false;
    ifdir->do_emit    = !ifdir->disabled;
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
}

// Preprocessor directive: undef.
static void c_directive_undef(c_preproc_t *pre, pos_t pos) {
}

// Handle a preprocessor directive.
static void c_preproc_directive(c_preproc_t *pre) {
    token_t name = c_preproc_next_raw(pre, true, false, false);
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

    while (1) {
        token_t peek = tkn_peek(&file->tkn_ctx->base);
        if (peek.type == TOKENTYPE_EOL || peek.type == TOKENTYPE_EOF) {
            break;
        }
        token_t tkn = c_preproc_next_raw(pre, false, false, false);
        if (tkn.type != TOKENTYPE_WHITESPACE) {
            if (!has_token) {
                start_pos = tkn.pos;
                has_token = true;
            }
            end_pos = tkn.pos;
        }
        tkn_delete(tkn);
    }

    if (!has_token) {
        return NULL;
    }

    pos_t span = pos_including(start_pos, end_pos);
    char *buf  = strong_malloc(span.len + 1);
    for (off_t i = 0; i < span.len; i++) {
        int c = srcfile_readb(span.srcfile, span.off + i);
        if (c < 0) {
            buf[i] = '\0';
            break;
        }
        buf[i] = (char)c;
    }
    buf[span.len] = '\0';

    if (pos_out) {
        *pos_out = span;
    }
    return buf;
}

// Consume tokens up to and including the next newline.
// If `warn_extra_tok` is `true`, emit a warning if any non-whitespace tokens exist.
static void c_preproc_until_eol(c_preproc_t *pre, bool warn_extra_tok) {
    bool  has_extra = false;
    pos_t extra;

    while (1) {
        token_t tkn = c_preproc_next_raw(pre, false, false, false);
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

// Get the next token without preprocessing.
// If `allow_next_file` is `false` and at the end of an include file, returns EOF.
static token_t c_preproc_next_raw(c_preproc_t *pre, bool skip_whitespace, bool skip_eol, bool allow_next_line) {
    while (1) {
        assert(pre->stack_len >= 1);
        token_t tkn;
    again:
        tkn = tkn_peek(&pre->stack[pre->stack_len - 1].tkn_ctx->base);
        if (!allow_next_line && (tkn.type == TOKENTYPE_EOL || tkn.type == TOKENTYPE_EOF)) {
            tkn_delete(tkn);
            return (token_t){
                .pos  = tkn.pos,
                .type = TOKENTYPE_EOL,
            };
        }
        tkn_next(&pre->stack[pre->stack_len - 1].tkn_ctx->base);
        if (tkn.type == TOKENTYPE_EOF || tkn.type == TOKENTYPE_EOL) {
            pre->blank_line = true;
        } else {
            pre->blank_line &= tkn.type == TOKENTYPE_WHITESPACE;
        }
        if ((tkn.type == TOKENTYPE_WHITESPACE && skip_whitespace) || (tkn.type == TOKENTYPE_EOL && skip_eol)) {
            tkn_delete(tkn);
            goto again;
        }
        if (tkn.type != TOKENTYPE_EOF || pre->stack_len == 1) {
            return tkn;
        }

        // Pop include file.
        c_incfile_eof(pre);
        c_incfile_pop(pre);
        pre->blank_line = true;
    }
}

// Get the next token on the current line after macro expansion.
// Used by certain directives.
static token_t c_preproc_next_expanded(c_preproc_t *pre) {
    token_t tkn;

again:
    if (pre->expand_index < pre->expand_len) {
        // A macro was expanded; return its tokens first.
        tkn = pre->expand[pre->expand_index];
        pre->expand_index++;
        goto emit;
    }

    tkn = c_preproc_next_raw(pre, true, false, false);

    if (tkn.type == TOKENTYPE_IDENT) {
        c_macro_t const *macro = map_get(&pre->macros, tkn.strval);
        if (macro) {
            tkn_delete(tkn);
            c_macro_expand(pre, macro);
            goto again;
        }
    }

emit:
    return tkn;
}

// Get the next token from the preprocessor.
token_t c_preproc_next(tokenizer_t *ctx) {
    c_preproc_t *pre = (c_preproc_t *)ctx;
    token_t      tkn;

again:
    if (pre->expand_index < pre->expand_len) {
        // A macro was expanded; return its tokens first.
        tkn = pre->expand[pre->expand_index];
        pre->expand_index++;
        goto emit;
    }

    // Cache this before `c_preproc_next_raw` overwrites it.
    bool blank_line = pre->blank_line;
    tkn             = c_preproc_next_raw(pre, !pre->keep_whitespace, !pre->keep_whitespace, true);

    if (tkn.type == TOKENTYPE_OTHER && tkn.subtype == C_TKN_HASH && blank_line) {
        // Always check for directives.
        tkn_delete(tkn);
        c_preproc_directive(pre);
        goto again;
    } else if (!c_preproc_do_emit(pre)) {
        // Anything else while not emitting is ignored.
        tkn_delete(tkn);
        goto again;
    } else if (tkn.type == TOKENTYPE_IDENT) {
        c_macro_t const *macro = map_get(&pre->macros, tkn.strval);
        if (macro) {
            tkn_delete(tkn);
            c_macro_expand(pre, macro);
            goto again;
        }
    }

emit:
    if (tkn.type == TOKENTYPE_EOF || tkn.type == TOKENTYPE_WHITESPACE || tkn.type == TOKENTYPE_EOL) {
        return tkn;
    }

    if (tkn.type == TOKENTYPE_IDENT) {
        assert(tkn.strval_len >= 1);
        if ('0' <= tkn.strval[0] && tkn.strval[0] <= '9') {
            // TODO: Numeric.
        } else {
            c_keyw_t keyw = c_keyw_get(pre->c_std, tkn.strval);
            if (keyw < C_N_KEYWS) {
                free(tkn.strval);
                tkn.strval     = NULL;
                tkn.strval_len = 0;
                tkn.type       = TOKENTYPE_KEYWORD;
                tkn.subtype    = keyw;
            }
        }
    }

    return tkn;
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

// Perform macro-expansion.
void c_macro_expand(c_preproc_t *pre, c_macro_t const *macro) {
}
