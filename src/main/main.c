
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_compiler.h"
#include "c_parser.h"
#include "ir/ir_optimizer.h"

#include <stdio.h>
#include <stdlib.h>

static void compile(char const *path) {
    // Create requisite contexts.
    cctx_t    *cctx = cctx_create();
    srcfile_t *src  = srcfile_open(cctx, path);
    if (!src) {
        perror("Cannot open source file");
        cctx_delete(cctx);
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

    printf("// Compiling %s\n", path);

    // While not EOF, keep parsing and compiling stuff.
    while (tkn_peek(tctx).type != TOKENTYPE_EOF) {
        token_t decls = c_parse_decls(&pctx, true);
        if (decls.subtype == C_AST_FUNC_DEF) {
            // Function definition.
            c_prepass_t prepass = c_precompile_pass(&decls);
            ir_func_t  *func    = c_compile_func_def(cc, &decls, &prepass);
            c_prepass_destroy(prepass);
            ir_func_to_ssa(func);
            ir_optimize(func);
            printf("\n");
            ir_func_serialize(func, stdout);
            ir_func_destroy(func);
        } else {
            // Declarations.
            c_compile_decls(cc, NULL, NULL, &cc->global_scope, &decls);
        }
        tkn_delete(decls);
    }

    // Print diagnostics.
    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag);
            diag = (diagnostic_t const *)diag->node.next;
        }
    }

    // Clean up.
    set_clear(&pctx.type_names);
    set_clear(&pctx.local_type_names);
    c_compiler_destroy(cc);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        compile(argv[i]);
    }
}
