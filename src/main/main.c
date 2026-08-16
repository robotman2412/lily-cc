
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "asm_print.h"
#include "backend.h"
#include "c_ast.h"
#include "c_compile.h"
#include "c_compiler.h"
#include "c_ir.h"
#include "c_lower.h"
#include "c_parser.h"
#include "codegen.h"
#include "ir.h"
#include "ir_optimizer.h"
#include "ir_serialization.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void compile2(char const *path) {
    // Create requisite contexts.
    cctx_t    *cctx = cctx_create();
    srcfile_t *src  = srcfile_open(cctx, path);
    if (!src) {
        perror("Cannot open source file");
        cctx_delete(cctx);
        return;
    }
    c_compiler_t *cc = c_compiler_create(
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
    tokenizer_t       *tctx    = c_tokenizer_create(cc, src, true);
    c_parser_t        *pctx    = c_parser_create(cc, tctx);
    backend_t const   *backend = backend_default();
    backend_profile_t *profile = backend->create_profile();
    backend->init_codegen(profile);

    printf("// Compiling %s\n", path);

    // While not EOF, keep parsing and compiling stuff.
    c_ast_def_list_t *ast = c_parse(pctx);
    cir_trans_unit_t *tu  = c_compile(cc, ast);

    for (size_t i = 0; i < tu->units.len; i++) {
        switch (tu->units.arr[i]->tag) {
            case CIR_UNIT_DECL: printf("decl %s\n", tu->units.arr[i]->decl->name); break;
            case CIR_UNIT_FUNC: printf("func %s\n", tu->units.arr[i]->func->name); break;
        }
    }
    if (cctx->n_err == 0) {
        for (size_t i = 0; i < tu->units.len; i++) {
            if (tu->units.arr[i]->tag != CIR_UNIT_FUNC) {
                continue;
            }
            ir_func_t *ir = c_lower_func(cc, tu->units.arr[i]->func);
            codegen(profile, ir);
            printf("\n");
            asm_print_func(ir, profile, stdout);
        }
    }

    c_ast_def_list_delete(ast);
    cir_trans_unit_delete(tu);

    // Print diagnostics.
    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        fflush(stdout);
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        fflush(stderr);
    }

    // Clean up.
    backend->delete_profile(profile);
    c_parser_delete(pctx);
    c_compiler_delete(cc);
    cctx_delete(cctx);
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        // compile(argv[i]);
        compile2(argv[i]);
    }
}
