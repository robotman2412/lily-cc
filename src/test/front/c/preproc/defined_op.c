
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "preproc.inc"

// The `defined` operator works with and without parentheses, and reports
// whether a macro is currently defined.
int main() {
    // clang-format off
    char const data[] =
        "#define YES\n"
        "#if defined NO\n"
        "absent_no\n"
        "#endif\n"
        "#if defined ( NO )\n"
        "absent_no_paren\n"
        "#endif\n"
        "#if defined YES\n"
        "present_yes\n"
        "#endif\n"
        "#if defined ( YES )\n"
        "present_yes_paren\n"
        "#endif\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_defined_op>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, &c_test_options, false, false);

    EXPECT_IDENT(pre, "present_yes");
    EXPECT_IDENT(pre, "present_yes_paren");
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    TEST_OK();
}
