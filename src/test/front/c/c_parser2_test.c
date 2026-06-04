
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_parser2.h"
#include "c_std.h"
#include "c_tokenizer.h"
#include "testcase.h"



static c_options_t c_parse2_test_options = {
    .c_std          = C_STD_max,
    .gnu_ext_enable = true,
    .char_is_signed = true,
    .short16        = true,
    .int32          = true,
    .long64         = true,
    .big_endian     = false,
    .size_type      = C_PRIM_SLONG,
};



static char *test_c_expr_basic() {
    char const   source[] = "1 + 2 * 3 - 4 % 5 / 6";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_expr_basic>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = &c_tkn_create_impl(src, &c_parse2_test_options)->base;
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    c_ast_expr_t *expr = c_parse2_expr(&pctx);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_ast_expr_print(expr, stdout, 0);
        return TEST_FAIL;
    }

    c_ast_expr_delete(expr);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_expr_basic)


static char *test_c_expr_call() {
    char const   source[] = "foobar() + (1) - beer(2, 3)";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_expr_call>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = &c_tkn_create_impl(src, &c_parse2_test_options)->base;
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    c_ast_expr_t *expr = c_parse2_expr(&pctx);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_ast_expr_print(expr, stdout, 0);
        return TEST_FAIL;
    }

    c_ast_expr_delete(expr);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_expr_call)


static char *test_c_expr_deref() {
    char const   source[] = "*foo.bar->baz[1](2)";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_expr_deref>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = &c_tkn_create_impl(src, &c_parse2_test_options)->base;
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    c_ast_expr_t *expr = c_parse2_expr(&pctx);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_ast_expr_print(expr, stdout, 0);
        return TEST_FAIL;
    }

    c_ast_expr_delete(expr);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_expr_deref)


static char *test_c_expr_cast() {
    char const   source[] = "(ident0 *(*const volatile)[2]) (ident1)";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_expr_cast>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = &c_tkn_create_impl(src, &c_parse2_test_options)->base;
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    set_add(&pctx.type_names, "ident0");
    c_ast_expr_t *expr = c_parse2_expr(&pctx);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_ast_expr_print(expr, stdout, 0);
        return TEST_FAIL;
    }

    set_clear(&pctx.type_names);
    c_ast_expr_delete(expr);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_expr_cast)


static char *test_c_type_funcptr() {
    char const   source[] = "ident0 (*)(ident1)";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_type_funcptr>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = &c_tkn_create_impl(src, &c_parse2_test_options)->base;
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    set_add(&pctx.type_names, "ident0");
    set_add(&pctx.type_names, "ident1");
    c_ast_type_name_t *type = c_parse2_type_name(&pctx);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_ast_type_name_print(type, stdout, 0);
        return TEST_FAIL;
    }

    set_clear(&pctx.type_names);
    c_ast_type_name_delete(type);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_type_funcptr)


static char *test_c_type_struct() {
    // clang-format off
    char const   source[] =
    "struct thing {\n"
    "  int a;\n"
    "  int b;\n"
    "}\n"
    ;
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<c_type_struct>", source, sizeof(source) - 1);
    tokenizer_t *tctx = &c_tkn_create_impl(src, &c_parse2_test_options)->base;
    c_parser_t   pctx = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    c_ast_type_name_t *type = c_parse2_type_name(&pctx);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_ast_type_name_print(type, stdout, 0);
        return TEST_FAIL;
    }

    set_clear(&pctx.type_names);
    set_clear(&pctx.local_type_names);
    c_ast_type_name_delete(type);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_type_struct)


static char *test_c_type_enum() {
    // clang-format off
    char const   source[] =
    "enum thing {\n"
    "  thing_first,\n"
    "  thing_second\n"
    "}\n"
    ;
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<c_type_enum>", source, sizeof(source) - 1);
    tokenizer_t *tctx = &c_tkn_create_impl(src, &c_parse2_test_options)->base;
    c_parser_t   pctx = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    c_ast_type_name_t *type = c_parse2_type_name(&pctx);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_ast_type_name_print(type, stdout, 0);
        return TEST_FAIL;
    }

    set_clear(&pctx.type_names);
    set_clear(&pctx.local_type_names);
    c_ast_type_name_delete(type);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_type_enum)


static char *test_c_stmt_decl() {
    char const   source[] = "typename ident, *ident2[];";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_stmt_decl>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = &c_tkn_create_impl(src, &c_parse2_test_options)->base;
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    set_add(&pctx.type_names, "typename");
    c_ast_def_t *def = c_parse2_def(&pctx, false);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_ast_def_print(def, stdout, 0);
        return TEST_FAIL;
    }

    set_clear(&pctx.type_names);
    c_ast_def_delete(def);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_stmt_decl)


static char *test_c_stmt_ctrl() {
    // clang-format off
    char const   source[] =
    "if (1) {\n"
    "  c(a * b);\n"
    "}\n"
    "while (c(a, 2), d()) {\n"
    "  e();\n"
    "}\n"
    "for (int i = 0; i < 10; i++) {\n"
    "  printf(\"Hello, World!\\n\");\n"
    "}\n"
    "return 13;\n"
    "return;\n"
    ;
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<c_stmt_ctrl>", source, sizeof(source) - 1);
    tokenizer_t *tctx = &c_tkn_create_impl(src, &c_parse2_test_options)->base;
    c_parser_t   pctx = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    set_add(&pctx.type_names, "typename");
    c_ast_stmt_list_t *stmts = c_parse2_stmts(&pctx);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_ast_stmt_list_print(stmts, stdout, 0);
        return TEST_FAIL;
    }

    set_clear(&pctx.type_names);
    c_ast_stmt_list_delete(stmts);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_stmt_ctrl)


static char *test_c_function() {
    // clang-format off
    char const   source[] =
    "void funcname() {\n"
    "  foo();\n"
    "}\n"
    ;
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<c_function>", source, sizeof(source) - 1);
    tokenizer_t *tctx = &c_tkn_create_impl(src, &c_parse2_test_options)->base;
    c_parser_t   pctx = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    set_add(&pctx.type_names, "typename");
    c_ast_def_t *def = c_parse2_def(&pctx, true);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_ast_def_print(def, stdout, 0);
        return TEST_FAIL;
    }

    set_clear(&pctx.type_names);
    c_ast_def_delete(def);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_function)


static char *test_c_compliteral() {
    // clang-format off
    char const   source[] =
    "(struct a) {\n"
    "    [1] = 2,\n"
    "    .abc = 3,\n"
    "    4,\n"
    "    .def[5] = 6,\n"
    "    [7].fgh = 8,\n"
    "    {9, 10},\n"
    "    {{11}, 12},\n"
    "}\n"
    ;
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<c_compiteral>", source, sizeof(source) - 1);
    tokenizer_t *tctx = &c_tkn_create_impl(src, &c_parse2_test_options)->base;
    c_parser_t   pctx = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    c_ast_expr_t *expr = c_parse2_expr(&pctx);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_ast_expr_print(expr, stdout, 0);
        return TEST_FAIL;
    }

    c_ast_expr_delete(expr);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_compliteral)
