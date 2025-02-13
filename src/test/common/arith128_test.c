
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT



#if !defined(__SIZEOF_INT128__) || defined(LILY_SOFT_INT128)

#include "arith128.h"
#include "testcase.h"

char *test_mul128() {
    i128_t mul;

    mul = mul128(int128(0, 0), int128(0, 0));
    EXPECT_INT(lo64(mul), 0);
    EXPECT_INT(hi64(mul), 0);

    mul = mul128(int128(1, 0), int128(1, 0));
    EXPECT_INT(lo64(mul), 1);
    EXPECT_INT(hi64(mul), 0);

    mul = mul128(int128(1, 0), int128(0, 1));
    EXPECT_INT(lo64(mul), 0);
    EXPECT_INT(hi64(mul), 1);

    mul = mul128(int128(0, 1), int128(-1, -1));
    EXPECT_INT(lo64(mul), 0);
    EXPECT_INT(hi64(mul), -1);

    mul = mul128(int128(-1, -1), int128(-1, -1));
    EXPECT_INT(lo64(mul), 1);
    EXPECT_INT(hi64(mul), 0);

    mul = mul128(int128(1llu << 34, 0), int128(1llu << 34, 0));
    EXPECT_INT(lo64(mul), 0);
    EXPECT_INT(hi64(mul), 1 << 4);

    return TEST_OK;
}
LILY_TEST_CASE(test_mul128)

#endif
