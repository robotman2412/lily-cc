
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "preproc.inc"

// Line-continuation backslashes are folded both inside a definition body and
// inside a reference to the macro.
int main() {
    // clang-format off
    char const data[] =
        "#define A2 foo\\\n"
        "bar\n"
        "A2\n"
        "A\\\n"
        "2\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_line_continuation>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, &c_test_options, false, false);

    EXPECT_IDENT(pre, "foobar");
    EXPECT_IDENT(pre, "foobar");
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    TEST_OK();
}
