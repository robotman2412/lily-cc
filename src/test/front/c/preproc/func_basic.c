
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "preproc.inc"

// A function-like macro substitutes each parameter and emits the body's
// punctuation verbatim.
int main() {
    // clang-format off
    char const data[] =
        "#define A(X, Y, Z) X | Y | Z\n"
        "A(a, b, c)\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_func_basic>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, &c_test_options, false, false);

    EXPECT_IDENT(pre, "a");
    EXPECT_PUNCT(pre, C_TKN_OR);
    EXPECT_IDENT(pre, "b");
    EXPECT_PUNCT(pre, C_TKN_OR);
    EXPECT_IDENT(pre, "c");
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    TEST_OK();
}
