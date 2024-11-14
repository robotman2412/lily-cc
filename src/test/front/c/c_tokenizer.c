
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
    "++ --\n"
    "// Line comment things\n"
    "/*\n"
    "Block comment things\n"
    "*/\n"
    "an_identifier forauxiliary_identifier\n"
    "\"\\x00\\000\\x0\\x0000\\770\\377\\00\\0\"\n"
    "\'A\'\n"
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
    EXPECT_INT(tkn.pos.line, 0);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 3);
    EXPECT_INT(tkn.type, TOKENTYPE_KEYWORD);
    EXPECT_INT(tkn.subtype, C_KEYW_for);

    tkn = c_tkn_next(&tkn_ctx); // int
    EXPECT_INT(tkn.pos.line, 0);
    EXPECT_INT(tkn.pos.col, 4);
    EXPECT_INT(tkn.pos.len, 3);
    EXPECT_INT(tkn.type, TOKENTYPE_KEYWORD);
    EXPECT_INT(tkn.subtype, C_KEYW_int);

    tkn = c_tkn_next(&tkn_ctx); // 0x1000
    EXPECT_INT(tkn.pos.line, 1);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 6);
    EXPECT_INT(tkn.type, TOKENTYPE_ICONST);
    EXPECT_INT(tkn.ival, 0x1000);

    tkn = c_tkn_next(&tkn_ctx); // ++
    EXPECT_INT(tkn.pos.line, 2);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 2);
    EXPECT_INT(tkn.type, TOKENTYPE_OTHER);
    EXPECT_INT(tkn.subtype, C_TKN_INC);

    tkn = c_tkn_next(&tkn_ctx); // --
    EXPECT_INT(tkn.pos.line, 2);
    EXPECT_INT(tkn.pos.col, 3);
    EXPECT_INT(tkn.pos.len, 2);
    EXPECT_INT(tkn.type, TOKENTYPE_OTHER);
    EXPECT_INT(tkn.subtype, C_TKN_DEC);

    tkn = c_tkn_next(&tkn_ctx); // an_identifier
    EXPECT_INT(tkn.pos.line, 7);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 13);
    EXPECT_INT(tkn.type, TOKENTYPE_IDENT);
    EXPECT_STR(tkn.strval->data, "an_identifier");

    tkn = c_tkn_next(&tkn_ctx); // forauxiliary_identifier
    EXPECT_INT(tkn.pos.line, 7);
    EXPECT_INT(tkn.pos.col, 14);
    EXPECT_INT(tkn.pos.len, 23);
    EXPECT_INT(tkn.type, TOKENTYPE_IDENT);
    EXPECT_STR(tkn.strval->data, "forauxiliary_identifier");

    tkn = c_tkn_next(&tkn_ctx); // "\x00\000\x0\x0000\770\377\00\0"
    EXPECT_INT(tkn.pos.line, 8);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 32);
    EXPECT_INT(tkn.type, TOKENTYPE_SCONST);
    EXPECT_STR(tkn.strval->data, "\x00\000\x0\x0000\770\377\00\0");

    tkn = c_tkn_next(&tkn_ctx); // 'A'
    EXPECT_INT(tkn.pos.line, 9);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 3);
    EXPECT_INT(tkn.type, TOKENTYPE_CCONST);
    EXPECT_CHAR(tkn.ival, 'A');


    return TEST_OK;
}
LILY_TEST_CASE(tkn_basic)
