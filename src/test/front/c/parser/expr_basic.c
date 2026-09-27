// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "parser.inc"

int main() {
    char const   source[] = "1 + 2 * 3 - 4 % 5 / 6";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_expr_basic>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = &c_tkn_create_impl(src, &c_test_options)->base;
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    c_ast_expr_t *expr = c_parse_expr(&pctx);

    if (c_parser_test_diagnostics(cctx)) {
        c_ast_expr_dbg(expr, 0, stdout);
        TEST_FAIL();
    }

    c_ast_expr_delete(expr);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    TEST_OK();
}
