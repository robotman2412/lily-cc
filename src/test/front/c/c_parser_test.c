
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "c_parser.h"
#include "testcase.h"



char const *c_expr_basic() {
    char const   source[] = "1 + 2 * 3 - 4 % 5 / 6";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_expr_basic>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = c_tkn_create(src, C_STD_def);

    token_t expr = c_parse_expr(tctx);

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


char const *c_expr_call() {
    char const   source[] = "foobar() + (1) - beer(2, 3)";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_expr_call>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = c_tkn_create(src, C_STD_def);

    token_t expr = c_parse_expr(tctx);

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


char const *c_expr_deref() {
    char const   source[] = "*foo.bar->baz[1](2)";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_expr_deref>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = c_tkn_create(src, C_STD_def);

    token_t expr = c_parse_expr(tctx);

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


char const *c_expr_cast() {
    char const   source[] = "(ident0 *(*)[2]) (ident1)";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_expr_cast>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = c_tkn_create(src, C_STD_def);

    token_t expr = c_parse_expr(tctx);

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

    printf("\n");
    c_tkn_debug_print(expr);

    return TEST_OK;
}
LILY_TEST_CASE(c_expr_cast)
