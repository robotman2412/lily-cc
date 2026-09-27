
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "preproc.inc"

// `__VA_ARGS__` expansion.
int main() {
    // clang-format off
    char const data[] =
        "#define FOO(a, b, ...) __VA_ARGS__\n"
        "FOO(no1, no2, yes1, yes2)\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_va_args>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, &c_test_options, false, false);

    EXPECT_IDENT(pre, "yes1");
    EXPECT_PUNCT(pre, C_TKN_COMMA);
    EXPECT_IDENT(pre, "yes2");
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    TEST_OK();
}
