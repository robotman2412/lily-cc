
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "preproc.inc"

// A function-like macro can supply its own opening parenthesis from another
// macro and match a closing parenthesis from the surrounding source. The
// chain Q2()()() collapses through three nullary macros to "fin".
int main() {
    // clang-format off
    char const data[] =
        "#define R2() fin\n"
        "#define R1() R2(\n"
        "#define R0 R1(\n"
        "R0))\n"
        "#define Q0() fin\n"
        "#define Q1() Q0\n"
        "#define Q2() Q1\n"
        "Q2()()()\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_nested_calls>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, &c_test_options, false, false);

    EXPECT_IDENT(pre, "fin");
    EXPECT_IDENT(pre, "fin");
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    TEST_OK();
}
