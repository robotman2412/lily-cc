// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "parser.inc"

int main() {
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
    tokenizer_t *tctx = &c_tkn_create_impl(src, &c_test_options)->base;
    c_parser_t   pctx = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

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
