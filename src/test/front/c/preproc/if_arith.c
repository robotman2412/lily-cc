
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "preproc.inc"

// `#if` evaluates a constant expression with C operator precedence. Both
// expressions here are false, so neither inner `#error` fires and no
// diagnostics are produced.
int main() {
    // clang-format off
    char const data[] =
        "#if 2 + 2 * -1\n"
        "#error fail1\n"
        "#endif\n"
        "#if !0 - 1\n"
        "#error fail2\n"
        "#endif\n"
        "done\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_if_arith>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, &c_test_options, false, false);

    EXPECT_IDENT(pre, "done");
    EXPECT_EOF(pre);
    TEST_ASSERT(cctx->diagnostics.len == 0);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    TEST_OK();
}
