
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "preproc.inc"

// __COUNTER__ produces a fresh integer each time.
int main() {
    char const   data[] = "__COUNTER__\n__COUNTER__\n";
    cctx_t      *cctx   = cctx_create();
    srcfile_t   *src    = srcfile_create(cctx, "<test_preproc_counter>", data, sizeof(data) - 1);
    c_preproc_t *pre    = c_preproc_create(src, &c_test_options, false, false);

    EXPECT_ICONST(pre, 0u);
    EXPECT_ICONST(pre, 1u);
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    TEST_OK();
}
