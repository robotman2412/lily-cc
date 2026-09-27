// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "parser.inc"

int main() {
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
    tokenizer_t *tctx = &c_tkn_create_impl(src, &c_test_options)->base;
    c_parser_t   pctx = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    set_add(&pctx.type_names, "typename");
    c_ast_stmt_list_t *stmts = c_parse_stmts(&pctx);

    if (c_parser_test_diagnostics(cctx)) {
        c_ast_stmt_list_dbg(stmts, 0, stdout);
        TEST_FAIL();
    }

    set_clear(&pctx.type_names);
    c_ast_stmt_list_delete(stmts);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    TEST_OK();
}
