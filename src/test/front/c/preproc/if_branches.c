
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "preproc.inc"

// `#if` / `#elif` / `#else` selects exactly one branch. The first truthy
// condition wins, and `#else` only fires if every prior branch was false.
int main() {
    // clang-format off
    char const data[] =
        "#if 1\n"
        "first\n"
        "#elif 1\n"
        "first_elif\n"
        "#else\n"
        "first_else\n"
        "#endif\n"
        "#if 0\n"
        "second\n"
        "#elif 1\n"
        "second_elif\n"
        "#else\n"
        "second_else\n"
        "#endif\n"
        "#if 0\n"
        "third\n"
        "#elif 0\n"
        "third_elif\n"
        "#else\n"
        "third_else\n"
        "#endif\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_if_branches>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, &c_test_options, false, false);

    EXPECT_IDENT(pre, "first");
    EXPECT_IDENT(pre, "second_elif");
    EXPECT_IDENT(pre, "third_else");
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    TEST_OK();
}
