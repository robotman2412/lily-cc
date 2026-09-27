
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "preproc.inc"

// `#` stringizes its argument verbatim (no pre-expansion of the argument).
// `STR2` defers through another macro layer so the argument *is* expanded
// before stringization.
int main() {
    // clang-format off
    char const data[] =
        "#define STR(x)  # x\n"
        "#define STR2(x) STR(x)\n"
        "#define PASTE(A, B) A##B\n"
        "STR(This is some text)\n"
        "STR(PASTE(foo, bar))\n"
        "STR2(PASTE(foo, bar))\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_stringize>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, &c_test_options, false, false);

    EXPECT_SCONST(pre, "This is some text");
    EXPECT_SCONST(pre, "PASTE(foo, bar)");
    EXPECT_SCONST(pre, "foobar");
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    TEST_OK();
}
