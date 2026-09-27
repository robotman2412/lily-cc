
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "preproc.inc"

// `__VA_OPT__` expansion.
int main() {
    // clang-format off
    char const data[] =
        "#define FOO(a, b, ...) __VA_OPT__((yes))\n"
        "FOO(no1, no2, yes1, yes2)\n"
        "#define yes YES\n"
        "FOO(no1, no2, yes1, yes2)\n"
        "#undef yes\n"
        "#define yes YYYEEESSS\n"
        "FOO(no1, no2, yes1, yes2)\n"
        "FOO(no3, no4)\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_va_opt>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, &c_test_options, false, false);

    EXPECT_PUNCT(pre, C_TKN_LPAR);
    EXPECT_IDENT(pre, "yes");
    EXPECT_PUNCT(pre, C_TKN_RPAR);
    EXPECT_PUNCT(pre, C_TKN_LPAR);
    EXPECT_IDENT(pre, "YES");
    EXPECT_PUNCT(pre, C_TKN_RPAR);
    EXPECT_PUNCT(pre, C_TKN_LPAR);
    EXPECT_IDENT(pre, "YYYEEESSS");
    EXPECT_PUNCT(pre, C_TKN_RPAR);
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    TEST_OK();
}
