// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "parser.inc"

int main() {
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
    tokenizer_t *tctx = &c_tkn_create_impl(src, &c_test_options)->base;
    c_parser_t   pctx = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    c_ast_type_name_t *type = c_parse_type_name(&pctx);

    if (c_parser_test_diagnostics(cctx)) {
        c_ast_type_name_dbg(type, 0, stdout);
        TEST_FAIL();
    }

    set_clear(&pctx.type_names);
    set_clear(&pctx.local_type_names);
    c_ast_type_name_delete(type);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    TEST_OK();
}
