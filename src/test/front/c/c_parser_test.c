
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_parser.h"
#include "testcase.h"



static char *test_c_expr_basic() {
    char const   source[] = "1 + 2 * 3 - 4 % 5 / 6";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_expr_basic>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = c_tkn_create(src, C_STD_def);
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    token_t expr = c_parse_expr(&pctx);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag);
            diag = (diagnostic_t const *)diag->node.next;
        }
        return TEST_FAIL;
    }

    EXPECT_INT(expr.type, TOKENTYPE_AST);
    EXPECT_INT(expr.subtype, C_AST_EXPR_INFIX);
    EXPECT_INT(expr.params_len, 3);
    {
        token_t *sub = &expr.params[0];
        EXPECT_INT(sub->type, TOKENTYPE_OTHER);
        EXPECT_INT(sub->subtype, C_TKN_SUB);

        token_t *add_expr = &expr.params[1];
        EXPECT_INT(add_expr->type, TOKENTYPE_AST);
        EXPECT_INT(add_expr->subtype, C_AST_EXPR_INFIX);
        EXPECT_INT(add_expr->params_len, 3);
        {
            token_t *add = &add_expr->params[0];
            EXPECT_INT(add->type, TOKENTYPE_OTHER);
            EXPECT_INT(add->subtype, C_TKN_ADD);

            token_t *t1 = &add_expr->params[1];
            EXPECT_INT(t1->type, TOKENTYPE_ICONST);
            EXPECT_INT(t1->ival, 1);

            token_t *mul_expr = &add_expr->params[2];
            EXPECT_INT(mul_expr->type, TOKENTYPE_AST);
            EXPECT_INT(mul_expr->subtype, C_AST_EXPR_INFIX);
            EXPECT_INT(mul_expr->params_len, 3);
            {
                token_t *mul = &mul_expr->params[0];
                EXPECT_INT(mul->type, TOKENTYPE_OTHER);
                EXPECT_INT(mul->subtype, C_TKN_MUL);

                token_t *t2 = &mul_expr->params[1];
                EXPECT_INT(t2->type, TOKENTYPE_ICONST);
                EXPECT_INT(t2->ival, 2);

                token_t *t3 = &mul_expr->params[2];
                EXPECT_INT(t3->type, TOKENTYPE_ICONST);
                EXPECT_INT(t3->ival, 3);
            }
        }

        token_t *div_expr = &expr.params[2];
        EXPECT_INT(div_expr->type, TOKENTYPE_AST);
        EXPECT_INT(div_expr->subtype, C_AST_EXPR_INFIX);
        EXPECT_INT(div_expr->params_len, 3);
        {
            token_t *div = &div_expr->params[0];
            EXPECT_INT(div->type, TOKENTYPE_OTHER);
            EXPECT_INT(div->subtype, C_TKN_DIV);

            token_t *mod_expr = &div_expr->params[1];
            EXPECT_INT(mod_expr->type, TOKENTYPE_AST);
            EXPECT_INT(mod_expr->subtype, C_AST_EXPR_INFIX);
            EXPECT_INT(mod_expr->params_len, 3);
            {
                token_t *mod = &mod_expr->params[0];
                EXPECT_INT(mod->type, TOKENTYPE_OTHER);
                EXPECT_INT(mod->subtype, C_TKN_MOD);

                token_t *t4 = &mod_expr->params[1];
                EXPECT_INT(t4->type, TOKENTYPE_ICONST);
                EXPECT_INT(t4->ival, 4);

                token_t *t5 = &mod_expr->params[2];
                EXPECT_INT(t5->type, TOKENTYPE_ICONST);
                EXPECT_INT(t5->ival, 5);
            }

            token_t *t6 = &div_expr->params[2];
            EXPECT_INT(t6->type, TOKENTYPE_ICONST);
            EXPECT_INT(t6->ival, 6);
        }
    }

    tkn_delete(expr);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_expr_basic)


static char *test_c_expr_call() {
    char const   source[] = "foobar() + (1) - beer(2, 3)";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_expr_call>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = c_tkn_create(src, C_STD_def);
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    token_t expr = c_parse_expr(&pctx);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_tkn_debug_print(expr);
        return TEST_FAIL;
    }

    tkn_delete(expr);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_expr_call)


static char *test_c_expr_deref() {
    char const   source[] = "*foo.bar->baz[1](2)";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_expr_deref>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = c_tkn_create(src, C_STD_def);
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    token_t expr = c_parse_expr(&pctx);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag);
            diag = (diagnostic_t const *)diag->node.next;
        }
        return TEST_FAIL;
    }

    tkn_delete(expr);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_expr_deref)


static char *test_c_expr_cast() {
    char const   source[] = "(ident0 *(*const volatile)[2]) (ident1)";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_expr_cast>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = c_tkn_create(src, C_STD_def);
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    set_add(&pctx.type_names, "ident0");
    token_t token = c_parse_expr(&pctx);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_tkn_debug_print(token);
        return TEST_FAIL;
    }

    set_clear(&pctx.type_names);
    tkn_delete(token);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_expr_cast)


static char *test_c_type_funcptr() {
    char const   source[] = "ident0 (*)(ident1)";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_type_funcptr>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = c_tkn_create(src, C_STD_def);
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    set_add(&pctx.type_names, "ident0");
    set_add(&pctx.type_names, "ident1");
    token_t token = c_parse_type_name(&pctx);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_tkn_debug_print(token);
        return TEST_FAIL;
    }

    set_clear(&pctx.type_names);
    tkn_delete(token);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_type_funcptr)


static char *test_c_type_struct() {
    // clang-format off
    char const   source[] =
    "typedef struct thing {\n"
    "  int a;\n"
    "  int b;\n"
    "} thing_t;\n"
    ;
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<c_type_struct>", source, sizeof(source) - 1);
    tokenizer_t *tctx = c_tkn_create(src, C_STD_def);
    c_parser_t   pctx = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    token_t token = c_parse_decls(&pctx, false);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_tkn_debug_print(token);
        return TEST_FAIL;
    }

    set_clear(&pctx.type_names);
    set_clear(&pctx.local_type_names);
    tkn_delete(token);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_type_struct)


static char *test_c_type_enum() {
    // clang-format off
    char const   source[] =
    "typedef enum thing {\n"
    "  thing_first,\n"
    "  thing_second\n"
    "} thing_t;\n"
    ;
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<c_type_enum>", source, sizeof(source) - 1);
    tokenizer_t *tctx = c_tkn_create(src, C_STD_def);
    c_parser_t   pctx = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    token_t token = c_parse_decls(&pctx, false);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_tkn_debug_print(token);
        return TEST_FAIL;
    }

    set_clear(&pctx.type_names);
    set_clear(&pctx.local_type_names);
    tkn_delete(token);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_type_enum)


static char *test_c_stmt_decl() {
    char const   source[] = "typename ident, *ident2[];";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_stmt_decl>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = c_tkn_create(src, C_STD_def);
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    set_add(&pctx.type_names, "typename");
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

    set_clear(&pctx.type_names);
    tkn_delete(decl);
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
    tokenizer_t *tctx = c_tkn_create(src, C_STD_def);
    c_parser_t   pctx = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    set_add(&pctx.type_names, "typename");
    token_t decl = c_parse_stmts(&pctx);

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

    set_clear(&pctx.type_names);
    tkn_delete(decl);
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
    tokenizer_t *tctx = c_tkn_create(src, C_STD_def);
    c_parser_t   pctx = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    set_add(&pctx.type_names, "typename");
    token_t decl = c_parse_decls(&pctx, true);

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

    set_clear(&pctx.type_names);
    tkn_delete(decl);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_function)
