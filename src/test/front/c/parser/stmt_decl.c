// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "parser.inc"

int main() {
    char const   source[] = "typename ident, *ident2[];";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_stmt_decl>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = &c_tkn_create_impl(src, &c_test_options)->base;
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    set_add(&pctx.type_names, "typename");
    c_ast_def_t *def = c_parse_def(&pctx, false);

    if (c_parser_test_diagnostics(cctx)) {
        c_ast_def_dbg(def, 0, stdout);
        TEST_FAIL();
    }

    set_clear(&pctx.type_names);
    c_ast_def_delete(def);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    TEST_OK();
}
