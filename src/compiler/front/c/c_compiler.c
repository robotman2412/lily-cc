
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_compiler.h"

#include "c_parser.h"
#include "c_preproc.h"
#include "c_tokenizer.h"
#include "compiler.h"
#include "lilycc_malloc.h"
#include "set.h"
#include "tokenizer.h"

#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>



// Create a new C compiler context.
c_compiler_t *c_compiler_create(cctx_t *cctx, c_options_t options) {
    c_compiler_t *cc = lilycc_calloc(1, sizeof(c_compiler_t));
    cc->cctx         = cctx;
    cc->options      = options;

    return cc;
}

// Destroy a C compiler context.
void c_compiler_delete(c_compiler_t *cc) {
    lilycc_free(cc);
}


// Create a tokenizing context for this compiler.
tokenizer_t *c_tokenizer_create(c_compiler_t *cc, srcfile_t *src, bool do_preproc) {
    if (do_preproc) {
        c_preproc_t *pre = c_preproc_create(src, &cc->options, false, false);
        return &pre->base;
    } else {
        c_tokenizer_t *ctx = c_tkn_create_impl(src, &cc->options);
        return &ctx->base;
    }
}


// Create a parsing context for this compiler.
c_parser_t *c_parser_create(c_compiler_t *cc, tokenizer_t *tkn_ctx) {
    c_parser_t *ctx       = lilycc_calloc(1, sizeof(c_parser_t));
    ctx->local_type_names = STR_SET_EMPTY;
    ctx->type_names       = STR_SET_EMPTY;
    ctx->tkn_ctx          = tkn_ctx;
    ctx->options          = &cc->options;
    return ctx;
}

// Delete a parsing context.
void c_parser_delete(c_parser_t *parser) {
    set_clear(&parser->local_type_names);
    set_clear(&parser->type_names);
    tkn_ctx_delete(parser->tkn_ctx);
    lilycc_free(parser);
}
