
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "preproc.inc"

// Forward references inside a macro body are resolved at expansion time, so
// redefining a referenced macro between two expansions changes the result.
int main() {
    // clang-format off
    char const data[] =
        "#define FOO BAR\n"
        "FOO\n"
        "#define BAR baz\n"
        "FOO\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_object_macros>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, &c_test_options, false, false);

    EXPECT_IDENT(pre, "BAR");
    EXPECT_IDENT(pre, "baz");
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    TEST_OK();
}
