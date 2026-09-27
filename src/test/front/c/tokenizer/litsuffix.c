// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "testcase.h"
#include "tokenizer.inc"

int main() {
    // clang-format off
    char const data[] =
    "0xc0de\n"          // sint
    "0xc0deu\n"         // uint
    "0xcafebabe\n"      // uint
    "0xcafebabellu\n"   // ullong
    "0xcafebabelu\n"    // ulong
    "0xcafebabeull\n"   // ullong
    "0xcafebabeul\n"    // ulong
    ;
    // clang-format on

    cctx_t    *cctx = cctx_create();
    srcfile_t *src  = srcfile_create(cctx, "<test_c_tkn_litsuffix>", data, sizeof(data) - 1);

    tokenizer_t *tkn_ctx = &c_tkn_create_impl(src, &c_test_options)->base;
    token_t      tkn;

    tkn = c_tkn_next(tkn_ctx); // 0xc0de (sint)
    TEST_EXPECT_INT(tkn.pos.line, 0);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    TEST_EXPECT_INT(tkn.pos.len, 6);
    TEST_ASSERT(tkn.type == TOKENTYPE_ICONST);
    TEST_EXPECT_INT(tkn.subtype, C_PRIM_SINT);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // 0xc0deu (uint)
    TEST_EXPECT_INT(tkn.pos.line, 1);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    TEST_EXPECT_INT(tkn.pos.len, 7);
    TEST_ASSERT(tkn.type == TOKENTYPE_ICONST);
    TEST_EXPECT_INT(tkn.subtype, C_PRIM_UINT);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // 0xcafebabe (uint)
    TEST_EXPECT_INT(tkn.pos.line, 2);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    TEST_EXPECT_INT(tkn.pos.len, 10);
    TEST_ASSERT(tkn.type == TOKENTYPE_ICONST);
    TEST_EXPECT_INT(tkn.subtype, C_PRIM_UINT);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // 0xcafebabellu (ullong)
    TEST_EXPECT_INT(tkn.pos.line, 3);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    TEST_EXPECT_INT(tkn.pos.len, 13);
    TEST_ASSERT(tkn.type == TOKENTYPE_ICONST);
    TEST_EXPECT_INT(tkn.subtype, C_PRIM_ULLONG);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // 0xcafebabelu (ulong)
    TEST_EXPECT_INT(tkn.pos.line, 4);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    TEST_EXPECT_INT(tkn.pos.len, 12);
    TEST_ASSERT(tkn.type == TOKENTYPE_ICONST);
    TEST_EXPECT_INT(tkn.subtype, C_PRIM_ULONG);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // 0xcafebabeull (ullong)
    TEST_EXPECT_INT(tkn.pos.line, 5);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    TEST_EXPECT_INT(tkn.pos.len, 13);
    TEST_ASSERT(tkn.type == TOKENTYPE_ICONST);
    TEST_EXPECT_INT(tkn.subtype, C_PRIM_ULLONG);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // 0xcafebabeul (ulong)
    TEST_EXPECT_INT(tkn.pos.line, 6);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    TEST_EXPECT_INT(tkn.pos.len, 12);
    TEST_ASSERT(tkn.type == TOKENTYPE_ICONST);
    TEST_EXPECT_INT(tkn.subtype, C_PRIM_ULONG);
    tkn_delete(tkn);

    tkn_ctx_delete(tkn_ctx);
    cctx_delete(cctx);

    TEST_OK();
}
