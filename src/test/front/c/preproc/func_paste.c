
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "preproc.inc"

// `##` between two identifier arguments fuses them into a single identifier.
int main() {
    // clang-format off
    char const data[] =
        "#define PASTE(A, B) A##B\n"
        "PASTE(foo, bar)\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_func_paste>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, &c_test_options, false, false);

    EXPECT_IDENT(pre, "foobar");
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    TEST_OK();
}
