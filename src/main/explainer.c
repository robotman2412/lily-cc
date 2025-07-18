
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_compiler.h"
#include "c_parser.h"

#include <string.h>

static void compile_explain_type(char const *value) {
    // Create requisite contexts.
    cctx_t    *cctx = cctx_create();
    srcfile_t *src  = srcfile_create(cctx, "<argv>", value, strlen(value));
    if (!src) {
        return;
    }

    tokenizer_t  *tctx = c_tkn_create(src, C_STD_def);
    c_parser_t    pctx = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};
    c_compiler_t *cc   = c_compiler_create(
        cctx,
        (c_options_t){
              .c_std          = C_STD_def,
              .char_is_signed = true,
              .short16        = true,
              .int32          = true,
              .long64         = true,
              .size_type      = C_PRIM_ULONG,
        }
    );

    // While not EOF, keep parsing and compiling stuff.
    token_t ast     = c_parse_type_name(&pctx);
    rc_t    type_rc = NULL;
    if (!cctx->diagnostics.len) {
        if (ast.subtype == C_AST_TYPE_NAME) {
            rc_t spec_qual = c_compile_spec_qual_list(cc, &ast.params[0]);
            type_rc        = c_compile_decl(cc, &ast.params[1], spec_qual, NULL);
        } else if (ast.subtype == C_AST_SPEC_QUAL_LIST) {
            type_rc = c_compile_spec_qual_list(cc, &ast);
        }
    }

    // Print diagnostics.
    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        while (diag) {
            print_diagnostic(diag);
            diag = (diagnostic_t const *)diag->node.next;
        }
    }
    if (type_rc) {
        c_type_explain(type_rc->data, stdout);
        rc_delete(type_rc);
    }

    // Clean up.
    c_compiler_destroy(cc);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        compile_explain_type(argv[i]);
    }
}
