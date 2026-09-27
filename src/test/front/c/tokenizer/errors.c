// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "tokenizer.inc"

int main() {
    // clang-format off
    char const data[] =
    "\'\n"                                                                  // Expected '
    "\"\n"                                                                  // Expected "
    "0x0ffffffffffffffff\n"
    "0x10000000000000000\n"                                                 // Constant is too large and was truncated to 0
    "18446744073709551615\n"
    "18446744073709551616\n"                                                // Constant is too large and was truncated to 0
    "01777777777777777777777\n"
    "02000000000000000000000\n"                                             // Constant is too large and was truncated to 0
    "0b01111111111111111111111111111111111111111111111111111111111111111\n"
    "0b10000000000000000000000000000000000000000000000000000000000000000\n" // Constant is too large and was truncated to 0
    "0x0ffffffffffffffffffffffffffffffff_x128\n"
    "0x100000000000000000000000000000000_x128\n"                            // Constant is too large and was truncated to 0
    "0x 0b\n"                                                               // Invalid hexadecimal/binary constant
    "0xg 0b2 1a 08\n"                                                       // Invalid hexadecimal/binary/decimal/octal constant
    "\"\\U0\\U00000000\\u0\\u0000\\x\\x0\"\n"                               // Invalid hexadecimal escape sequence (x3)
    "0xc0dellll\n"                                                          // Invalid literal suffix
    ;
    // clang-format on


    cctx_t    *cctx = cctx_create();
    srcfile_t *src  = srcfile_create(cctx, "<c_tkn_errors>", data, sizeof(data) - 1);

    tokenizer_t *tkn_ctx = &c_tkn_create_impl(src, &c_test_options)->base;
    token_t      tkn;
    do {
        tkn = c_tkn_next(tkn_ctx);
        tkn_delete(tkn);
    } while (tkn.type != TOKENTYPE_EOF);

    // '
    diagnostic_t *diag = (diagnostic_t *)cctx->diagnostics.head;
    TEST_ASSERT(diag != NULL);
    TEST_EXPECT_STR(diag->msg, "Expected \'");
    TEST_EXPECT_INT(diag->pos.line, 0);
    TEST_EXPECT_INT(diag->pos.col, 1);
    TEST_EXPECT_INT(diag->pos.len, 0);
    TEST_ASSERT(diag->lvl == DIAG_ERR);
    diag = (diagnostic_t *)diag->node.next;
    TEST_ASSERT(diag != NULL);
    TEST_EXPECT_STR(diag->msg, "To match this \'");
    TEST_EXPECT_INT(diag->pos.line, 0);
    TEST_EXPECT_INT(diag->pos.col, 0);
    TEST_EXPECT_INT(diag->pos.len, 1);
    diag = (diagnostic_t *)diag->node.next;
    TEST_ASSERT(diag != NULL);
    TEST_EXPECT_STR(diag->msg, "Empty character constant");

    // "
    diag = (diagnostic_t *)diag->node.next;
    TEST_ASSERT(diag != NULL);
    TEST_ASSERT(diag->lvl == DIAG_ERR);
    TEST_EXPECT_STR(diag->msg, "Expected \"");
    TEST_EXPECT_INT(diag->pos.line, 1);
    TEST_EXPECT_INT(diag->pos.col, 1);
    TEST_EXPECT_INT(diag->pos.len, 0);
    diag = (diagnostic_t *)diag->node.next;
    TEST_ASSERT(diag != NULL);
    TEST_EXPECT_STR(diag->msg, "To match this \"");
    TEST_EXPECT_INT(diag->pos.line, 1);
    TEST_EXPECT_INT(diag->pos.col, 0);
    TEST_EXPECT_INT(diag->pos.len, 1);

    // 0x10000000000000000
    diag = (diagnostic_t *)diag->node.next;
    TEST_ASSERT(diag != NULL);
    TEST_EXPECT_STR(diag->msg, "Constant is too large and was truncated to 0 (0x0)");
    TEST_EXPECT_INT(diag->pos.line, 3);
    TEST_EXPECT_INT(diag->pos.col, 0);
    TEST_EXPECT_INT(diag->pos.len, 19);
    TEST_ASSERT(diag->lvl == DIAG_WARN);

    // 18446744073709551616
    diag = (diagnostic_t *)diag->node.next;
    TEST_ASSERT(diag != NULL);
    TEST_EXPECT_STR(diag->msg, "Constant is too large and was truncated to 0 (0x0)");
    TEST_EXPECT_INT(diag->pos.line, 5);
    TEST_ASSERT(diag->lvl == DIAG_WARN);

    // 01777777777777777777777
    diag = (diagnostic_t *)diag->node.next;
    TEST_ASSERT(diag != NULL);
    TEST_EXPECT_STR(diag->msg, "Constant is too large and was truncated to 0 (0x0)");
    TEST_EXPECT_INT(diag->pos.line, 7);
    TEST_ASSERT(diag->lvl == DIAG_WARN);

    // 0b10000000000000000000000000000000000000000000000000000000000000000
    diag = (diagnostic_t *)diag->node.next;
    TEST_ASSERT(diag != NULL);
    TEST_EXPECT_STR(diag->msg, "Constant is too large and was truncated to 0 (0x0)");
    TEST_EXPECT_INT(diag->pos.line, 9);
    TEST_ASSERT(diag->lvl == DIAG_WARN);

    // 0x100000000000000000000000000000000_x128
    diag = (diagnostic_t *)diag->node.next;
    TEST_ASSERT(diag != NULL);
    TEST_EXPECT_STR(diag->msg, "Constant is too large and was truncated to 0 (0x0)");
    TEST_EXPECT_INT(diag->pos.line, 11);
    TEST_ASSERT(diag->lvl == DIAG_WARN);

    // 0x
    diag = (diagnostic_t *)diag->node.next;
    TEST_ASSERT(diag != NULL);
    TEST_EXPECT_STR(diag->msg, "Invalid hexadecimal constant");
    TEST_EXPECT_INT(diag->pos.line, 12);
    TEST_ASSERT(diag->lvl == DIAG_ERR);

    // 0b
    diag = (diagnostic_t *)diag->node.next;
    TEST_ASSERT(diag != NULL);
    TEST_EXPECT_STR(diag->msg, "Invalid binary constant");
    TEST_EXPECT_INT(diag->pos.line, 12);
    TEST_ASSERT(diag->lvl == DIAG_ERR);

    // 0xg
    diag = (diagnostic_t *)diag->node.next;
    TEST_ASSERT(diag != NULL);
    TEST_EXPECT_STR(diag->msg, "Invalid hexadecimal constant");
    TEST_EXPECT_INT(diag->pos.line, 13);
    TEST_ASSERT(diag->lvl == DIAG_ERR);

    // 0b2
    diag = (diagnostic_t *)diag->node.next;
    TEST_ASSERT(diag != NULL);
    TEST_EXPECT_STR(diag->msg, "Invalid binary constant");
    TEST_EXPECT_INT(diag->pos.line, 13);
    TEST_ASSERT(diag->lvl == DIAG_ERR);

    // 1a
    diag = (diagnostic_t *)diag->node.next;
    TEST_ASSERT(diag != NULL);
    TEST_EXPECT_STR(diag->msg, "Invalid decimal constant");
    TEST_EXPECT_INT(diag->pos.line, 13);
    TEST_ASSERT(diag->lvl == DIAG_ERR);

    // 08
    diag = (diagnostic_t *)diag->node.next;
    TEST_ASSERT(diag != NULL);
    TEST_EXPECT_STR(diag->msg, "Invalid octal constant");
    TEST_EXPECT_INT(diag->pos.line, 13);
    TEST_ASSERT(diag->lvl == DIAG_ERR);

    // "\U0\U00000000\u0\u0000\x\x0"
    diag = (diagnostic_t *)diag->node.next;
    TEST_ASSERT(diag != NULL);
    TEST_EXPECT_STR(diag->msg, "Invalid hexadecimal escape sequence");
    TEST_EXPECT_INT(diag->pos.line, 14);
    TEST_ASSERT(diag->lvl == DIAG_ERR);
    diag = (diagnostic_t *)diag->node.next;
    TEST_ASSERT(diag != NULL);
    TEST_EXPECT_STR(diag->msg, "Invalid hexadecimal escape sequence");
    TEST_EXPECT_INT(diag->pos.line, 14);
    TEST_ASSERT(diag->lvl == DIAG_ERR);
    diag = (diagnostic_t *)diag->node.next;
    TEST_ASSERT(diag != NULL);
    TEST_EXPECT_STR(diag->msg, "Invalid hexadecimal escape sequence");
    TEST_EXPECT_INT(diag->pos.line, 14);
    TEST_ASSERT(diag->lvl == DIAG_ERR);

    // 0xc0dellll
    diag = (diagnostic_t *)diag->node.next;
    TEST_ASSERT(diag != NULL);
    TEST_EXPECT_STR(diag->msg, "Invalid literal suffix");
    TEST_EXPECT_INT(diag->pos.line, 15);
    TEST_ASSERT(diag->lvl == DIAG_ERR);

    tkn_ctx_delete(tkn_ctx);
    cctx_delete(cctx);
    TEST_OK();
}
