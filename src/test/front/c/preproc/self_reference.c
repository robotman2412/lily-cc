
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "preproc.inc"

// A macro that names itself transitively does not recurse: once the
// expansion of `F0()` re-encounters `F0`, that occurrence is left as a
// plain identifier followed by its parentheses.
int main() {
    // clang-format off
    char const data[] =
        "#define F0() F1()\n"
        "#define F1   F0\n"
        "F0()\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_self_reference>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, &c_test_options, false, false);

    EXPECT_IDENT(pre, "F0");
    EXPECT_PUNCT(pre, C_TKN_LPAR);
    EXPECT_PUNCT(pre, C_TKN_RPAR);
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    TEST_OK();
}
