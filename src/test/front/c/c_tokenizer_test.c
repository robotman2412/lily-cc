
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_compiler.h"
#include "c_std.h"
#include "c_tokenizer.h"
#include "testcase.h"



// Simple test of the various tokens.
static char *test_c_tkn_basic() {
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

    tokenizer_t *tkn_ctx = c_tkn_create(src, C_STD_max);
    token_t      tkn;


    tkn = c_tkn_next(tkn_ctx); // for
    EXPECT_INT(tkn.pos.line, 0);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 3);
    EXPECT_INT(tkn.type, TOKENTYPE_KEYWORD);
    EXPECT_INT(tkn.subtype, C_KEYW_for);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // int
    EXPECT_INT(tkn.pos.line, 0);
    EXPECT_INT(tkn.pos.col, 4);
    EXPECT_INT(tkn.pos.len, 3);
    EXPECT_INT(tkn.type, TOKENTYPE_KEYWORD);
    EXPECT_INT(tkn.subtype, C_KEYW_int);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // 0x1000
    EXPECT_INT(tkn.pos.line, 1);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 6);
    EXPECT_INT(tkn.type, TOKENTYPE_ICONST);
    EXPECT_INT(tkn.ival, 0x1000);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // ++
    EXPECT_INT(tkn.pos.line, 2);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 2);
    EXPECT_INT(tkn.type, TOKENTYPE_OTHER);
    EXPECT_INT(tkn.subtype, C_TKN_INC);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // --
    EXPECT_INT(tkn.pos.line, 2);
    EXPECT_INT(tkn.pos.col, 3);
    EXPECT_INT(tkn.pos.len, 2);
    EXPECT_INT(tkn.type, TOKENTYPE_OTHER);
    EXPECT_INT(tkn.subtype, C_TKN_DEC);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // an_identifier
    EXPECT_INT(tkn.pos.line, 7);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 13);
    EXPECT_INT(tkn.type, TOKENTYPE_IDENT);
    EXPECT_STR(tkn.strval, "an_identifier");
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // forauxiliary_identifier
    EXPECT_INT(tkn.pos.line, 7);
    EXPECT_INT(tkn.pos.col, 14);
    EXPECT_INT(tkn.pos.len, 23);
    EXPECT_INT(tkn.type, TOKENTYPE_IDENT);
    EXPECT_STR(tkn.strval, "forauxiliary_identifier");
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // "\x00\000\x0\x0000\770\377\00\0"
    EXPECT_INT(tkn.pos.line, 8);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 32);
    EXPECT_INT(tkn.type, TOKENTYPE_SCONST);
    EXPECT_STR_L(tkn.strval, tkn.strval_len, "\x00\000\x0\x0000\0770\377\00\0", 9);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // 'A'
    EXPECT_INT(tkn.pos.line, 9);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 3);
    EXPECT_INT(tkn.type, TOKENTYPE_CCONST);
    EXPECT_CHAR(tkn.ival, 'A');
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // "\'\"\?\\\a\b\f\n\r\t\v"
    EXPECT_INT(tkn.pos.line, 10);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 24);
    EXPECT_INT(tkn.type, TOKENTYPE_SCONST);
    EXPECT_STR(tkn.strval, "\'\"\?\\\a\b\f\n\r\t\v");
    tkn_delete(tkn);


    tkn_ctx_delete(tkn_ctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_tkn_basic)


// Test of literal suffix behaviour.
static char *test_c_tkn_litsuffix() {
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

    tokenizer_t *tkn_ctx = c_tkn_create(src, C_STD_max);
    token_t      tkn;

    tkn = c_tkn_next(tkn_ctx); // 0xc0de (sint)
    EXPECT_INT(tkn.pos.line, 0);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 6);
    EXPECT_INT(tkn.type, TOKENTYPE_ICONST);
    EXPECT_INT(tkn.subtype, C_PRIM_SINT);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // 0xc0deu (uint)
    EXPECT_INT(tkn.pos.line, 1);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 7);
    EXPECT_INT(tkn.type, TOKENTYPE_ICONST);
    EXPECT_INT(tkn.subtype, C_PRIM_UINT);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // 0xcafebabe (uint)
    EXPECT_INT(tkn.pos.line, 2);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 10);
    EXPECT_INT(tkn.type, TOKENTYPE_ICONST);
    EXPECT_INT(tkn.subtype, C_PRIM_UINT);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // 0xcafebabellu (ullong)
    EXPECT_INT(tkn.pos.line, 3);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 13);
    EXPECT_INT(tkn.type, TOKENTYPE_ICONST);
    EXPECT_INT(tkn.subtype, C_PRIM_ULLONG);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // 0xcafebabelu (ulong)
    EXPECT_INT(tkn.pos.line, 4);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 12);
    EXPECT_INT(tkn.type, TOKENTYPE_ICONST);
    EXPECT_INT(tkn.subtype, C_PRIM_ULONG);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // 0xcafebabeull (ullong)
    EXPECT_INT(tkn.pos.line, 5);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 13);
    EXPECT_INT(tkn.type, TOKENTYPE_ICONST);
    EXPECT_INT(tkn.subtype, C_PRIM_ULLONG);
    tkn_delete(tkn);

    tkn = c_tkn_next(tkn_ctx); // 0xcafebabeul (ulong)
    EXPECT_INT(tkn.pos.line, 6);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 12);
    EXPECT_INT(tkn.type, TOKENTYPE_ICONST);
    EXPECT_INT(tkn.subtype, C_PRIM_ULONG);
    tkn_delete(tkn);

    tkn_ctx_delete(tkn_ctx);
    cctx_delete(cctx);

    return TEST_OK;
}
LILY_TEST_CASE(test_c_tkn_litsuffix)


// Test of various error messages.
static char *test_c_tkn_errors() {
    // clang-format off
    char const data[] =
    "\'\n"                                                                  // Character constant spans end of line
    "\"\n"                                                                  // String constant spans end of line
    "0x0ffffffffffffffff\n"
    "0x10000000000000000\n"                                                 // Constant is too large and was truncated to 0
    "18446744073709551615\n"
    "18446744073709551616\n"                                                // Constant is too large and was truncated to 0
    "01777777777777777777777\n"
    "02000000000000000000000\n"                                             // Constant is too large and was truncated to 0
    "0b01111111111111111111111111111111111111111111111111111111111111111\n"
    "0b10000000000000000000000000000000000000000000000000000000000000000\n" // Constant is too large and was truncated to 0
    "0x 0b\n"                                                               // Invalid hexadecimal/binary constant
    "0xg 0b2 1a 08\n"                                                       // Invalid hexadecimal/binary/decimal/octal constant
    "\"\\U0\\U00000000\\u0\\u0000\\x\\x0\"\n"                               // Invalid hexadecimal escape sequence (x3)
    "0xc0dellll\n"                                                          // Invalid literal suffix
    ;
    // clang-format on


    cctx_t    *cctx = cctx_create();
    srcfile_t *src  = srcfile_create(cctx, "<c_tkn_errors>", data, sizeof(data) - 1);

    tokenizer_t *tkn_ctx = c_tkn_create(src, C_STD_max);
    token_t      tkn;
    do {
        tkn = c_tkn_next(tkn_ctx);
        tkn_delete(tkn);
    } while (tkn.type != TOKENTYPE_EOF);

    // '
    diagnostic_t *diag = (diagnostic_t *)cctx->diagnostics.head;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->pos.line, 0);
    EXPECT_INT(diag->pos.col, 0);
    EXPECT_INT(diag->pos.len, 1);
    EXPECT_INT(diag->lvl, DIAG_ERR);
    EXPECT_STR(diag->msg, "Character constant spans end of line");
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_STR(diag->msg, "Empty character constant");

    // "
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->lvl, DIAG_ERR);
    EXPECT_STR(diag->msg, "String constant spans end of line");

    // 0x10000000000000000
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->pos.line, 3);
    EXPECT_INT(diag->pos.col, 0);
    EXPECT_INT(diag->pos.len, 19);
    EXPECT_INT(diag->lvl, DIAG_WARN);
    EXPECT_STR(diag->msg, "Constant is too large and was truncated to 0");

    // 18446744073709551616
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->pos.line, 5);
    EXPECT_INT(diag->lvl, DIAG_WARN);
    EXPECT_STR(diag->msg, "Constant is too large and was truncated to 0");

    // 01777777777777777777777
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->pos.line, 7);
    EXPECT_INT(diag->lvl, DIAG_WARN);
    EXPECT_STR(diag->msg, "Constant is too large and was truncated to 0");

    // 0b10000000000000000000000000000000000000000000000000000000000000000
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->pos.line, 9);
    EXPECT_INT(diag->lvl, DIAG_WARN);
    EXPECT_STR(diag->msg, "Constant is too large and was truncated to 0");

    // 0x
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->lvl, DIAG_ERR);
    EXPECT_STR(diag->msg, "Invalid hexadecimal constant");

    // 0b
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->lvl, DIAG_ERR);
    EXPECT_STR(diag->msg, "Invalid binary constant");

    // 0xg
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->lvl, DIAG_ERR);
    EXPECT_STR(diag->msg, "Invalid hexadecimal constant");

    // 0b2
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->lvl, DIAG_ERR);
    EXPECT_STR(diag->msg, "Invalid binary constant");

    // 1a
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->lvl, DIAG_ERR);
    EXPECT_STR(diag->msg, "Invalid decimal constant");

    // 08
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->lvl, DIAG_ERR);
    EXPECT_STR(diag->msg, "Invalid octal constant");

    // "\U0\U00000000\u0\u0000\x\x0"
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->lvl, DIAG_ERR);
    EXPECT_STR(diag->msg, "Invalid hexadecimal escape sequence");
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->lvl, DIAG_ERR);
    EXPECT_STR(diag->msg, "Invalid hexadecimal escape sequence");
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->lvl, DIAG_ERR);
    EXPECT_STR(diag->msg, "Invalid hexadecimal escape sequence");

    // 0xc0dellll
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->lvl, DIAG_ERR);
    EXPECT_STR(diag->msg, "Invalid literal suffix");

    tkn_ctx_delete(tkn_ctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_tkn_errors)
