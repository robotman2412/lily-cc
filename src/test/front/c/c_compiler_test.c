
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_parser.h"
#include "testcase.h"



static char *test_c_compile_type() {
    char const   source[] = "void (*foobarb)();";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_compile_type>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = c_tkn_create(src, C_STD_def);
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};
    c_compiler_t cc       = {
        .cctx = cctx,
        .options = {
            .c_std          = C_STD_def,
            .char_is_signed = true,
            .short16        = true,
            .int32          = true,
            .long64         = true,
            .size_type      = C_PRIM_ULONG,
        },
        .typedefs = STR_MAP_EMPTY,
    };

    token_t decl = c_parse_decls(&pctx, false);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_tkn_debug_print(decl);
        return TEST_FAIL;
    }

    rc_t inner = c_compile_spec_qual_list(&cc, &decl.params[0]);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag);
            diag = (diagnostic_t const *)diag->node.next;
        }
        if (inner) {
            c_type_explain(inner->data, stdout);
            printf("\n");
        }
        return TEST_FAIL;
    }

    rc_t full = c_compile_decl(&cc, &decl.params[1], inner, NULL);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag);
            diag = (diagnostic_t const *)diag->node.next;
        }
        if (inner) {
            c_type_explain(full->data, stdout);
            printf("\n");
        }
        return TEST_FAIL;
    }

    tkn_delete(decl);
    rc_delete(full);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_compile_type)


static char *test_c_compile_expr() {
    char const    source[] = "1 + 2 * 3";
    cctx_t       *cctx     = cctx_create();
    srcfile_t    *src      = srcfile_create(cctx, "<c_compile_expr>", source, sizeof(source) - 1);
    tokenizer_t  *tctx     = c_tkn_create(src, C_STD_def);
    c_parser_t    pctx     = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};
    c_compiler_t *cc       = c_compiler_create(
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
    c_prepass_t dummy_prepass = {
        .pointer_taken = PTR_SET_EMPTY,
    };

    token_t expr_tok = c_parse_expr(&pctx);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_tkn_debug_print(expr_tok);
        return TEST_FAIL;
    }

    ir_func_t       *func = ir_func_create("c_compile_expr", NULL, 0, NULL);
    c_compile_expr_t expr = c_compile_expr(cc, &dummy_prepass, (ir_code_t *)func->code_list.head, NULL, &expr_tok);
    c_value_destroy(expr.res);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag);
            diag = (diagnostic_t const *)diag->node.next;
        }
        ir_func_serialize(func, stdout);
        return TEST_FAIL;
    }


    ir_func_delete(func);
    tkn_delete(expr_tok);

    c_compiler_destroy(cc);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_compile_expr)


static char *test_c_compile_func() {
    // clang-format off
    char const source[] =
    "int foobar(int);\n"
    "void functest() {\n"
    "    int a;\n"
    "    a = 3;\n"
    "    if (a != 0) {\n"
    "        a = 1;\n"
    "    } else {\n"
    "        a = 2;\n"
    "    }\n"
    "    return a;\n"
    "}\n"
    ;
    // clang-format on
    cctx_t       *cctx = cctx_create();
    srcfile_t    *src  = srcfile_create(cctx, "<c_compile_func>", source, sizeof(source) - 1);
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

    token_t foobar_tok = c_parse_decls(&pctx, true);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_tkn_debug_print(foobar_tok);
        return TEST_FAIL;
    }

    token_t functest_tok = c_parse_decls(&pctx, true);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_tkn_debug_print(functest_tok);
        return TEST_FAIL;
    }

    c_prepass_t prepass = c_precompile_pass(&functest_tok);
    ir_func_t  *func    = c_compile_func_def(cc, &functest_tok, &prepass);
    c_prepass_destroy(prepass);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag);
            diag = (diagnostic_t const *)diag->node.next;
        }
        ir_func_serialize(func, stdout);
        return TEST_FAIL;
    }

    ir_func_delete(func);
    tkn_delete(functest_tok);
    tkn_delete(foobar_tok);

    c_compiler_destroy(cc);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_compile_func)
