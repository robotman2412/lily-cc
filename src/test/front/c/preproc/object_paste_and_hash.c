
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "preproc.inc"

// `##` in an object-like macro pastes two identifiers, and a stray `#` in the
// body has no special meaning (no parameter to stringize).
int main() {
    // clang-format off
    char const data[] =
        "#define A0 foo##bar\n"
        "A0\n"
        "#define A1 #ok\n"
        "A1\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_object_paste_and_hash>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, &c_test_options, false, false);

    EXPECT_IDENT(pre, "foobar");
    EXPECT_PUNCT(pre, C_TKN_HASH);
    EXPECT_IDENT(pre, "ok");
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    TEST_OK();
}
