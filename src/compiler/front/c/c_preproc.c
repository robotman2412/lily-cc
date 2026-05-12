
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
#include <string.h>



static void    c_preproc_destroy(tokenizer_t *tkn);
static void    c_incfile_push(c_preproc_t *pre, pos_t pos, char const *path);
static void    c_incfile_pop(c_preproc_t *pre);
static void    c_incfile_eof(c_preproc_t *pre);
static void    c_directive_include(c_preproc_t *pre);
static void    c_directive_pragma(c_preproc_t *pre);
static void    c_directive_if(c_preproc_t *pre, bool elif, bool ifdef, bool ifndef);
static void    c_directive_else(c_preproc_t *pre);
static void    c_directive_endif(c_preproc_t *pre);
static void    c_directive_warning(c_preproc_t *pre, bool is_error);
static void    c_directive_define(c_preproc_t *pre);
static void    c_directive_undef(c_preproc_t *pre);
static void    c_preproc_until_eol(c_preproc_t *pre, bool warn_extra_tok);
static bool    c_preproc_do_emit(c_preproc_t *pre);
static token_t c_preproc_next_raw(c_preproc_t *pre, bool allow_next_file);
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
    pre->stack_len          = 1;
    pre->stack_cap          = 1;
    pre->stack              = strong_malloc(sizeof(void *));
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
    assert(pre->stack_len >= 2);
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
            "Unterminated %s directive",
            incfile->ifdir[i].allow_else ? "if" : "else"
        );
        cctx_diagnostic(pre->cctx, incfile->tkn_ctx->base.pos, DIAG_HINT, "Add an #endif here");
    }
    free(incfile->ifdir);
    incfile->ifdir_len = 0;
    incfile->ifdir_cap = 0;
}

// Preprocessor directive: include.
static void c_directive_include(c_preproc_t *pre) {
    c_incfile_t *file = &pre->stack[pre->stack_len - 1];
    token_t      token;

again:
    file->tkn_ctx->str_anglebrac = true;
    token                        = c_preproc_next_raw(pre, false);
    file->tkn_ctx->str_anglebrac = false;
    if (token.type == TOKENTYPE_WHITESPACE) {
        tkn_delete(token);
        goto again;
    } else if (token.type != TOKENTYPE_SCONST) {
        cctx_diagnostic(pre->cctx, token.pos, DIAG_ERR, "Expected a path");
        tkn_delete(token);
        return;
    }

    // TODO: Is there a difference between `<>` and `""` style strings for `#include`?
    c_incfile_push(pre, token.pos, token.strval);
    tkn_delete(token);
}

// Preprocessor directive: pragma.
static void c_directive_pragma(c_preproc_t *pre) {
}

// Preprocessor directive: if/elif.
static void c_directive_if(c_preproc_t *pre, bool elif, bool ifdef, bool ifndef) {
}

// Preprocessor directive: else.
static void c_directive_else(c_preproc_t *pre) {
}

// Preprocessor directive: endif.
static void c_directive_endif(c_preproc_t *pre) {
}

// Preprocessor directive: warning.
static void c_directive_warning(c_preproc_t *pre, bool is_error) {
}

// Preprocessor directive: define.
static void c_directive_define(c_preproc_t *pre) {
}

// Preprocessor directive: undef.
static void c_directive_undef(c_preproc_t *pre) {
}

// Handle a preprocessor directive.
static void c_preproc_directive(c_preproc_t *pre) {
    token_t name;
again:
    name = c_preproc_next_raw(pre, false);
    if (name.type == TOKENTYPE_WHITESPACE) {
        tkn_delete(name);
        goto again;
    } else if (name.type != TOKENTYPE_IDENT) {
        cctx_diagnostic(pre->cctx, name.pos, DIAG_ERR, "Expected preprocessing directive name");
        tkn_delete(name);
        return;
    }

    // The if directives are always processed.
    if (!strcmp(name.strval, "if")) {
        c_directive_if(pre, false, false, false);
    } else if (!strcmp(name.strval, "ifdef")) {
        c_directive_if(pre, false, true, false);
    } else if (!strcmp(name.strval, "ifndef")) {
        c_directive_if(pre, false, false, true);
    } else if (!strcmp(name.strval, "elif")) {
        c_directive_if(pre, true, false, false);
    } else if (!strcmp(name.strval, "elifdef")) {
        c_directive_if(pre, true, true, false);
    } else if (!strcmp(name.strval, "elifndef")) {
        c_directive_if(pre, true, false, true);
    } else if (!strcmp(name.strval, "else")) {
        c_directive_else(pre);
    } else if (!strcmp(name.strval, "endif")) {
        c_directive_endif(pre);
    } else if (!c_preproc_do_emit(pre)) {
        // Any remaining directives are only processed if the current if directive branch is active.
        c_preproc_until_eol(pre, false);
        return;
    } else if (!strcmp(name.strval, "include")) {
        c_directive_include(pre);
    } else if (!strcmp(name.strval, "pragma")) {
        c_directive_pragma(pre);
    } else if (!strcmp(name.strval, "warning")) {
        c_directive_warning(pre, false);
    } else if (!strcmp(name.strval, "error")) {
        c_directive_warning(pre, true);
    } else if (!strcmp(name.strval, "define")) {
        c_directive_define(pre);
    } else if (!strcmp(name.strval, "undef")) {
        c_directive_undef(pre);
    }

    c_preproc_until_eol(pre, true);
}

// Consume tokens up to and including the next newline.
// If `warn_extra_tok` is `true`, emit a warning if any non-whitespace tokens exist.
static void c_preproc_until_eol(c_preproc_t *pre, bool warn_extra_tok) {
    bool  has_extra = false;
    pos_t extra;

    while (1) {
        token_t tkn = c_preproc_next_raw(pre, false);
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
static token_t c_preproc_next_raw(c_preproc_t *pre, bool allow_next_file) {
    while (1) {
        assert(pre->stack_len >= 1);
        token_t tkn = tkn_next(&pre->stack[pre->stack_len - 1].tkn_ctx->base);
        if (tkn.type == TOKENTYPE_EOF) {
            c_incfile_eof(pre);
        }
        if (tkn.type == TOKENTYPE_EOF || tkn.type == TOKENTYPE_EOL) {
            pre->blank_line = true;
        }
        if (tkn.type != TOKENTYPE_EOF || pre->stack_len == 1 || !allow_next_file) {
            return tkn;
        }

        // Pop include file.
        c_incfile_pop(pre);
    }
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

    tkn = c_preproc_next_raw(pre, true);

    if (tkn.type == TOKENTYPE_OTHER && tkn.subtype == C_TKN_HASH && pre->blank_line) {
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
    if (tkn.type == TOKENTYPE_EOF) {
        return tkn;
    } else if (tkn.type == TOKENTYPE_WHITESPACE || tkn.type == TOKENTYPE_EOL) {
        tkn_delete(tkn);
        goto again;
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

    pre->blank_line = false;
    return tkn;
}

// Destroy a macro.
void c_macro_destroy(c_macro_t *macro) {
    for (size_t i = 0; i < macro->args_len; i++) {
        free(macro->args[i]);
    }
    free(macro->args);
    for (size_t i = 0; i < macro->tokens_len; i++) {
        tkn_delete(macro->tokens[i]);
    }
    free(macro->tokens);
    free(macro);
}

// Perform macro-expansion.
void c_macro_expand(c_preproc_t *pre, c_macro_t const *macro) {
}
