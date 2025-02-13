
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "compiler.h"
#include "testcase.h"



// Simple test of the various tokens.
static char *test_srcfile_ram() {
    // clang-format off
    char const data[] =
    "012\n"
    "abc\r"
    "efg\r\n"
    "ijk!"
    ;
    // clang-format on
    char const *data_exp = "012\nabc\nefg\nijk!";

    cctx_t    *cctx = cctx_create();
    srcfile_t *src  = srcfile_create(cctx, "<srcfile_ram>", data, sizeof(data) - 1);


    pos_t pos = {0};
    int   off = 0;
    for (int line = 0; line < 4; line++) {
        for (int col = 0; col < 4; col++) {
            EXPECT_INT(pos.line, line);
            EXPECT_INT(pos.col, col);
            EXPECT_INT(pos.off, off);
            int c = srcfile_getc(src, &pos);
            off++;
            if (off == 12)
                off++;
            EXPECT_CHAR(c, *(data_exp++));
        }
    }


    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_srcfile_ram)
