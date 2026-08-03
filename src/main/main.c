
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "asm_print.h"
#include "backend.h"
#include "c_ast.h"
#include "c_compile.h"
#include "c_compiler.h"
#include "c_ir.h"
#include "c_parser.h"
#include "c_parser2.h"
#include "codegen.h"
#include "ir.h"
#include "ir/ir_optimizer.h"
#include "ir_serialization.h"

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
    while (tkn_peek(tctx).type != TOKENTYPE_EOF) {
        token_t decls = c_parse_decls(pctx, true);
        if (decls.subtype == C_AST_FUNC_DEF) {
            // Function definition.
            c_prepass_t prepass = c_precompile_pass(&decls);
            ir_func_t  *func    = c_compile_func_def(cc, &decls, &prepass);
            c_prepass_destroy(prepass);

            printf("\n// Compiled, unoptimized IR:\n");
            ir_func_serialize(func, profile, stdout);

            ir_func_to_ssa(func);
            ir_optimize(func);
            printf("\n// Optimized IR:\n");
            ir_func_serialize(func, profile, stdout);

            codegen(profile, func);
            printf("\n// IR lowering to RISC-V instructions:\n");
            ir_func_serialize(func, profile, stdout);

            printf("\n// Assembly printing:\n");
            asm_print_func(func, profile, stdout);

            ir_func_delete(func);
            printf("\n\n");
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
    c_ast_def_list_t *ast = c_parse2(pctx);
    c_ast_def_list_dbg(ast, 0, stdout);
    cir_trans_unit_t *tu = c_compile2(cc, ast);
    cir_trans_unit_dbg(tu, 0, stdout);
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
