
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once



#include "c_options.h"
#include "c_parser.h"
#include "c_tokenizer.h"
#include "compiler.h"
#include "tokenizer.h"



// C compiler context.
typedef struct c_compiler c_compiler_t;



// C compiler context.
struct c_compiler {
    // C compiler options.
    c_options_t options;
    // Generic compiler context.
    cctx_t     *cctx;
};



// Create a new C compiler context.
c_compiler_t *c_compiler_create(cctx_t *cctx, c_options_t options);
// Destroy a C compiler context.
void          c_compiler_delete(c_compiler_t *cc);

// Create a tokenizing context for this compiler.
tokenizer_t *c_tokenizer_create(c_compiler_t *cc, srcfile_t *src, bool do_preproc);

// Create a parsing context for this compiler.
c_parser_t *c_parser_create(c_compiler_t *cc, tokenizer_t *tkn_ctx);
// Delete a parsing context.
void        c_parser_delete(c_parser_t *parser);
