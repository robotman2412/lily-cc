// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "testcase.h"
#include "tokenizer.inc"

int main() {
    // clang-format off
    char const data[] =
    "for int\n"
    "0x1000\n"
    "++ --\n"
    "// Line comment things\n"
    "/*\n"
    "Block comment things\n"
    "*/\n"
    "an_identifier forauxiliary_identifier\n"
    "\"\\x00\\000\\x0\\x0000\\770\\377\\00\\0\"\n"
    "\'A\'\n"
    "\"" "\\\'\\\"\\?\\\\\\a\\b\\f\\n\\r\\t\\v" "\"\n"
    ;
    // clang-format on

    cctx_t    *cctx = cctx_create();
    srcfile_t *src  = srcfile_create(cctx, "<c_tkn_basic>", data, sizeof(data) - 1);

    tokenizer_t *tkn_ctx = &c_tkn_create_impl(src, &c_test_options)->base;
    token_t      tkn;


    tkn = c_tkn_next(tkn_ctx); // for
    TEST_EXPECT_INT(tkn.pos.line, 0);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    TEST_EXPECT_INT(tkn.pos.len, 3);
    TEST_ASSERT(tkn.type == TOKENTYPE_KEYWORD);
    TEST_EXPECT_INT(tkn.subtype, C_KEYW_for);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // int
    TEST_EXPECT_INT(tkn.pos.line, 0);
    TEST_EXPECT_INT(tkn.pos.col, 4);
    TEST_EXPECT_INT(tkn.pos.len, 3);
    TEST_ASSERT(tkn.type == TOKENTYPE_KEYWORD);
    TEST_EXPECT_INT(tkn.subtype, C_KEYW_int);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // 0x1000
    TEST_EXPECT_INT(tkn.pos.line, 1);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    TEST_EXPECT_INT(tkn.pos.len, 6);
    TEST_ASSERT(tkn.type == TOKENTYPE_ICONST);
    TEST_EXPECT_INT(tkn.ival, 0x1000u);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // ++
    TEST_EXPECT_INT(tkn.pos.line, 2);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    TEST_EXPECT_INT(tkn.pos.len, 2);
    TEST_ASSERT(tkn.type == TOKENTYPE_OTHER);
    TEST_EXPECT_INT(tkn.subtype, C_TKN_INC);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // --
    TEST_EXPECT_INT(tkn.pos.line, 2);
    TEST_EXPECT_INT(tkn.pos.col, 3);
    TEST_EXPECT_INT(tkn.pos.len, 2);
    TEST_ASSERT(tkn.type == TOKENTYPE_OTHER);
    TEST_EXPECT_INT(tkn.subtype, C_TKN_DEC);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // an_identifier
    TEST_EXPECT_INT(tkn.pos.line, 7);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    TEST_EXPECT_INT(tkn.pos.len, 13);
    TEST_ASSERT(tkn.type == TOKENTYPE_IDENT);
    TEST_EXPECT_STR(tkn.strval, "an_identifier");
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // forauxiliary_identifier
    TEST_EXPECT_INT(tkn.pos.line, 7);
    TEST_EXPECT_INT(tkn.pos.col, 14);
    TEST_EXPECT_INT(tkn.pos.len, 23);
    TEST_ASSERT(tkn.type == TOKENTYPE_IDENT);
    TEST_EXPECT_STR(tkn.strval, "forauxiliary_identifier");
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // "\x00\000\x0\x0000\770\377\00\0"
    TEST_EXPECT_INT(tkn.pos.line, 8);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    TEST_EXPECT_INT(tkn.pos.len, 32);
    TEST_ASSERT(tkn.type == TOKENTYPE_SCONST);
    TEST_EXPECT_STR_L(tkn.strval, tkn.strval_len, "\x00\000\x0\x0000\0770\377\00\0", 9u);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // 'A'
    TEST_EXPECT_INT(tkn.pos.line, 9);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    TEST_EXPECT_INT(tkn.pos.len, 3);
    TEST_ASSERT(tkn.type == TOKENTYPE_CCONST);
    TEST_EXPECT_INT((int)tkn.ival, 'A');
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // "\'\"\?\\\a\b\f\n\r\t\v"
    TEST_EXPECT_INT(tkn.pos.line, 10);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    TEST_EXPECT_INT(tkn.pos.len, 24);
    TEST_ASSERT(tkn.type == TOKENTYPE_SCONST);
    TEST_EXPECT_STR(tkn.strval, "\'\"\?\\\a\b\f\n\r\t\v");
    tkn_delete(tkn);


    tkn_ctx_delete(tkn_ctx);
    cctx_delete(cctx);
    TEST_OK();
}
