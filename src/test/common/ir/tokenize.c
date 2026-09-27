
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "compiler.h"
#include "ir_tokenizer.h"
#include "ir_types.h"
#include "testcase.h"
#include "tokenizer.h"



int main() {
    // clang-format off
    char const src[] =
    "function var\n"
    "s32'0xcafebabe\n"
    "// Line comment things\n"
    "/*\n"
    "Block comment things\n"
    "*/\n"
    "%localname\n"
    "varnota_keyword\n"
    "<globalname>\n"
    ",\n"
    "(\n"
    ")\n"
    "bool'?\n"
    ;
    // clang-format on

    // Create tokenizer context.
    cctx_t      *cctx    = cctx_create();
    srcfile_t   *srcfile = srcfile_create(cctx, "<test_ir_deserialize>.lily_ir", src, sizeof(src) - 1);
    tokenizer_t *tctx    = ir_tkn_create(srcfile);
    token_t      tkn;

#define ASSERT_TYPE(tokentype) TEST_ASSERT(tkn.type == tokentype, "%s", tokentype_name[tkn.type]) // NOLINT

#define EXPECT_EOL_TKN()                                                                                               \
    {                                                                                                                  \
        tkn = tkn_next(tctx);                                                                                          \
        ASSERT_TYPE(TOKENTYPE_EOL);                                                                                    \
        tkn_delete(tkn);                                                                                               \
        while (tkn_peek(tctx).type == TOKENTYPE_EOL) {                                                                 \
            tkn_delete(tkn_next(tctx));                                                                                \
        }                                                                                                              \
    }


    tkn = tkn_next(tctx); // function
    TEST_EXPECT_INT(tkn.pos.line, 0);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    ASSERT_TYPE(TOKENTYPE_KEYWORD);
    TEST_EXPECT_INT(tkn.pos.len, 8);
    TEST_EXPECT_INT(tkn.subtype, IR_KEYW_function);
    tkn_delete(tkn);

    tkn = tkn_next(tctx); // var
    TEST_EXPECT_INT(tkn.pos.line, 0);
    TEST_EXPECT_INT(tkn.pos.col, 9);
    ASSERT_TYPE(TOKENTYPE_KEYWORD);
    TEST_EXPECT_INT(tkn.pos.len, 3);
    TEST_EXPECT_INT(tkn.subtype, IR_KEYW_var);
    tkn_delete(tkn);
    EXPECT_EOL_TKN();

    tkn = tkn_next(tctx); // s32'0xcafebabe
    TEST_EXPECT_INT(tkn.pos.line, 1);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    ASSERT_TYPE(TOKENTYPE_ICONST);
    TEST_EXPECT_INT(tkn.pos.len, 14);
    TEST_EXPECT_INT(tkn.subtype, IR_PRIM_s32);
    TEST_EXPECT_INT((int)tkn.ival, (int)0xcafebabe);
    tkn_delete(tkn);
    EXPECT_EOL_TKN();

    tkn = tkn_next(tctx); // %localname
    TEST_EXPECT_INT(tkn.pos.line, 6);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    ASSERT_TYPE(TOKENTYPE_IDENT);
    TEST_EXPECT_INT(tkn.pos.len, 10);
    TEST_EXPECT_INT(tkn.subtype, IR_IDENT_LOCAL);
    TEST_EXPECT_STR_L(tkn.strval, tkn.strval_len, "localname", 9u);
    tkn_delete(tkn);
    EXPECT_EOL_TKN();

    tkn = tkn_next(tctx); // varnota_keyword
    TEST_EXPECT_INT(tkn.pos.line, 7);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    ASSERT_TYPE(TOKENTYPE_IDENT);
    TEST_EXPECT_INT(tkn.pos.len, 15);
    TEST_EXPECT_INT(tkn.subtype, IR_IDENT_BARE);
    TEST_EXPECT_STR_L(tkn.strval, tkn.strval_len, "varnota_keyword", 15u);
    tkn_delete(tkn);
    EXPECT_EOL_TKN();

    tkn = tkn_next(tctx); // <globalname>
    TEST_EXPECT_INT(tkn.pos.line, 8);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    ASSERT_TYPE(TOKENTYPE_IDENT);
    TEST_EXPECT_INT(tkn.pos.len, 12);
    TEST_EXPECT_INT(tkn.subtype, IR_IDENT_GLOBAL);
    TEST_EXPECT_STR_L(tkn.strval, tkn.strval_len, "globalname", 10u);
    tkn_delete(tkn);
    EXPECT_EOL_TKN();

    tkn = tkn_next(tctx); // ,
    TEST_EXPECT_INT(tkn.pos.line, 9);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    ASSERT_TYPE(TOKENTYPE_OTHER);
    TEST_EXPECT_INT(tkn.pos.len, 1);
    TEST_EXPECT_INT(tkn.subtype, IR_TKN_COMMA);
    tkn_delete(tkn);
    EXPECT_EOL_TKN();

    tkn = tkn_next(tctx); // (
    TEST_EXPECT_INT(tkn.pos.line, 10);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    ASSERT_TYPE(TOKENTYPE_OTHER);
    TEST_EXPECT_INT(tkn.pos.len, 1);
    TEST_EXPECT_INT(tkn.subtype, IR_TKN_LPAR);
    tkn_delete(tkn);
    EXPECT_EOL_TKN();

    tkn = tkn_next(tctx); // )
    TEST_EXPECT_INT(tkn.pos.line, 11);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    ASSERT_TYPE(TOKENTYPE_OTHER);
    TEST_EXPECT_INT(tkn.pos.len, 1);
    TEST_EXPECT_INT(tkn.subtype, IR_TKN_RPAR);
    tkn_delete(tkn);
    EXPECT_EOL_TKN();

    tkn = tkn_next(tctx); // bool'?
    TEST_EXPECT_INT(tkn.pos.line, 12);
    TEST_EXPECT_INT(tkn.pos.col, 0);
    ASSERT_TYPE(TOKENTYPE_OTHER);
    TEST_EXPECT_INT(tkn.pos.len, 6);
    TEST_EXPECT_INT(tkn.subtype, IR_TKN_UNDEF);
    TEST_ASSERT(tkn.ival == IR_PRIM_bool, "%s", ir_prim_name[tkn.ival]);
    tkn_delete(tkn);
    EXPECT_EOL_TKN();


    // Clean up.
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);

    TEST_OK();
}
