
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "frontend.h"

#include "testcase.h"



// Simple test of the various tokens.
static char const *srcfile_ram() {
    // clang-format off
    char const data[] =
    "012\n"
    "abc\r"
    "efg\r\n"
    "ijk!"
    ;
    // clang-format on
    char const *data_exp = "012\nabc\nefg\nijk!";

    front_ctx_t *fe_ctx = front_create();
    srcfile_t   *src    = srcfile_create(fe_ctx, "<srcfile_ram>", data, sizeof(data) - 1);


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


    return TEST_OK;
}
LILY_TEST_CASE(srcfile_ram)
