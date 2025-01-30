
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_parser.h"
#include "testcase.h"



static char *c_expr_basic() {
    char const   source[] = "1 + 2 * 3 - 4 % 5 / 6";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_expr_basic>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = c_tkn_create(src, C_STD_def);
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = SET_EMPTY};

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

    return TEST_OK;
}
LILY_TEST_CASE(c_expr_basic)


static char *c_expr_call() {
    char const   source[] = "foobar() + (1) - beer(2, 3)";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_expr_call>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = c_tkn_create(src, C_STD_def);
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = SET_EMPTY};

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

    return TEST_OK;
}
LILY_TEST_CASE(c_expr_call)


static char *c_expr_deref() {
    char const   source[] = "*foo.bar->baz[1](2)";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_expr_deref>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = c_tkn_create(src, C_STD_def);
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = SET_EMPTY};

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

    // printf("\n");
    // c_tkn_debug_print(expr);

    return TEST_OK;
}
LILY_TEST_CASE(c_expr_deref)


static char *c_expr_cast() {
    char const   source[] = "(ident0 *(*const volatile)[2]) (ident1)";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_expr_cast>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = c_tkn_create(src, C_STD_def);
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = SET_EMPTY};

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

#pragma region generated_test
    EXPECT_INT(token.pos.col, 1);
    EXPECT_INT(token.pos.len, 37);
    EXPECT_INT(token.type, TOKENTYPE_AST);
    EXPECT_INT(token.subtype, C_AST_EXPR_CALL);
    EXPECT_INT(token.params_len, 2);
    {
        token_t token_0 = token.params[0];
        EXPECT_INT(token_0.pos.line, 0);
        EXPECT_INT(token_0.pos.col, 1);
        EXPECT_INT(token_0.pos.len, 27);
        EXPECT_INT(token_0.type, TOKENTYPE_AST);
        EXPECT_INT(token_0.subtype, C_AST_TYPE_NAME);
        EXPECT_INT(token_0.params_len, 2);
        {
            token_t token_0_0 = token_0.params[0];
            EXPECT_INT(token_0_0.pos.line, 0);
            EXPECT_INT(token_0_0.pos.col, 1);
            EXPECT_INT(token_0_0.pos.len, 6);
            EXPECT_INT(token_0_0.type, TOKENTYPE_IDENT);
            EXPECT_STR_L(token_0_0.strval, token_0_0.strval_len, "ident0", 6);

            token_t token_0_1 = token_0.params[1];
            EXPECT_INT(token_0_1.pos.line, 0);
            EXPECT_INT(token_0_1.pos.col, 8);
            EXPECT_INT(token_0_1.pos.len, 20);
            EXPECT_INT(token_0_1.type, TOKENTYPE_AST);
            EXPECT_INT(token_0_1.subtype, C_AST_TYPE_PTR_TO);
            EXPECT_INT(token_0_1.params_len, 2);
            {
                token_t token_0_1_0 = token_0_1.params[0];
                EXPECT_INT(token_0_1_0.pos.line, 0);
                EXPECT_INT(token_0_1_0.pos.col, 8);
                EXPECT_INT(token_0_1_0.pos.len, 1);
                EXPECT_INT(token_0_1_0.type, TOKENTYPE_OTHER);
                EXPECT_INT(token_0_1_0.subtype, C_KEYW_bool);

                token_t token_0_1_1 = token_0_1.params[1];
                EXPECT_INT(token_0_1_1.pos.line, 0);
                EXPECT_INT(token_0_1_1.pos.col, 10);
                EXPECT_INT(token_0_1_1.pos.len, 18);
                EXPECT_INT(token_0_1_1.type, TOKENTYPE_AST);
                EXPECT_INT(token_0_1_1.subtype, C_AST_EXPR_INDEX);
                EXPECT_INT(token_0_1_1.params_len, 2);
                {
                    token_t token_0_1_1_0 = token_0_1_1.params[0];
                    EXPECT_INT(token_0_1_1_0.pos.line, 0);
                    EXPECT_INT(token_0_1_1_0.pos.col, 10);
                    EXPECT_INT(token_0_1_1_0.pos.len, 1);
                    EXPECT_INT(token_0_1_1_0.type, TOKENTYPE_AST);
                    EXPECT_INT(token_0_1_1_0.subtype, C_AST_TYPE_PTR_QUAL);
                    EXPECT_INT(token_0_1_1_0.params_len, 2);
                    {
                        token_t token_0_1_1_0_0 = token_0_1_1_0.params[0];
                        EXPECT_INT(token_0_1_1_0_0.pos.line, 0);
                        EXPECT_INT(token_0_1_1_0_0.pos.col, 11);
                        EXPECT_INT(token_0_1_1_0_0.pos.len, 5);
                        EXPECT_INT(token_0_1_1_0_0.type, TOKENTYPE_KEYWORD);
                        EXPECT_INT(token_0_1_1_0_0.subtype, C_KEYW_const);

                        token_t token_0_1_1_0_1 = token_0_1_1_0.params[1];
                        EXPECT_INT(token_0_1_1_0_1.pos.line, 0);
                        EXPECT_INT(token_0_1_1_0_1.pos.col, 17);
                        EXPECT_INT(token_0_1_1_0_1.pos.len, 8);
                        EXPECT_INT(token_0_1_1_0_1.type, TOKENTYPE_KEYWORD);
                        EXPECT_INT(token_0_1_1_0_1.subtype, C_KEYW_volatile);
                    }

                    token_t token_0_1_1_1 = token_0_1_1.params[1];
                    EXPECT_INT(token_0_1_1_1.pos.line, 0);
                    EXPECT_INT(token_0_1_1_1.pos.col, 27);
                    EXPECT_INT(token_0_1_1_1.pos.len, 1);
                    EXPECT_INT(token_0_1_1_1.type, TOKENTYPE_ICONST);
                    EXPECT_INT(token_0_1_1_1.ival, 2);
                }
            }
        }

        token_t token_1 = token.params[1];
        EXPECT_INT(token_1.pos.line, 0);
        EXPECT_INT(token_1.pos.col, 32);
        EXPECT_INT(token_1.pos.len, 6);
        EXPECT_INT(token_1.type, TOKENTYPE_AST);
        EXPECT_INT(token_1.subtype, C_AST_EXPRS);
        EXPECT_INT(token_1.params_len, 1);
        {
            token_t token_1_0 = token_1.params[0];
            EXPECT_INT(token_1_0.pos.line, 0);
            EXPECT_INT(token_1_0.pos.col, 32);
            EXPECT_INT(token_1_0.pos.len, 6);
            EXPECT_INT(token_1_0.type, TOKENTYPE_IDENT);
            EXPECT_STR_L(token_1_0.strval, token_1_0.strval_len, "ident1", 6);
        }
    }
#pragma endregion generated_test

    return TEST_OK;
}
LILY_TEST_CASE(c_expr_cast)


static char *c_type_funcptr() {
    char const   source[] = "ident0 (*)(ident1)";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_type_funcptr>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = c_tkn_create(src, C_STD_def);
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = SET_EMPTY};

    set_add(&pctx.type_names, "ident0");
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

    return TEST_OK;
}
LILY_TEST_CASE(c_type_funcptr)


// static char *c_stmt_decl() {
//     char const   source[] = "typename ident, *ident2[];";
//     cctx_t      *cctx     = cctx_create();
//     srcfile_t   *src      = srcfile_create(cctx, "<c_stmt_decl>", source, sizeof(source) - 1);
//     tokenizer_t *tctx     = c_tkn_create(src, C_STD_def);
//     c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = SET_EMPTY};

//     token_t decl = c_parse_decls(&pctx);

//     if (cctx->diagnostics.len) {
//         diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
//         printf("\n");
//         while (diag) {
//             print_diagnostic(diag);
//             diag = (diagnostic_t const *)diag->node.next;
//         }
//         c_tkn_debug_print(decl);
//         return TEST_FAIL;
//     }

//     printf("\n");
//     c_tkn_debug_print(decl);

//     return TEST_OK;
// }
// LILY_TEST_CASE(c_stmt_decl)
