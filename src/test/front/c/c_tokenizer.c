
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "token/c_tokenizer.h"

#include "testcase.h"



// Simple test of the various tokens.
static char const *tkn_basic() {
    // clang-format off
    char const data[] =
    "for int\n"
    "0x1000\n"
    "++ -- +/ += /=\n"
    "// Line comment things\n"
    "/*\n"
    "Block comment things\n"
    "*/\n"
    // "\"\\x00\\000\\x0\\x0000\\770\\377\\00\\0\"\n"
    // "\'A\'\n"
    "an_identifier forauxiliary_identifier"
    ;
    // clang-format on


    front_ctx_t *fe_ctx = front_create();
    srcfile_t   *src    = srcfile_create(fe_ctx, "<tkn_basic>", data, sizeof(data) - 1);

    tokenizer_t tkn_ctx = {
        .file = src,
        .pos = {
            .srcfile = src,
        },
    };
    token_t tkn;


    tkn = c_tkn_next(&tkn_ctx); // for
    EXPECT_INT(tkn.pos.len, 3);
    EXPECT_INT(tkn.pos.line, 0);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.type, TOKENTYPE_KEYWORD);
    EXPECT_INT(tkn.subtype, C_KEYW_for);

    tkn = c_tkn_next(&tkn_ctx); // int
    EXPECT_INT(tkn.pos.len, 3);
    EXPECT_INT(tkn.pos.line, 0);
    EXPECT_INT(tkn.pos.col, 4);
    EXPECT_INT(tkn.type, TOKENTYPE_KEYWORD);
    EXPECT_INT(tkn.subtype, C_KEYW_int);

    tkn = c_tkn_next(&tkn_ctx); // 0x1000
    EXPECT_INT(tkn.pos.len, 6);
    EXPECT_INT(tkn.pos.line, 1);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.type, TOKENTYPE_ICONST);
    EXPECT_INT(tkn.ival, 0x1000);


    return TEST_OK;
}
LILY_TEST_CASE(tkn_basic)
