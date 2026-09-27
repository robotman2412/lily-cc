
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT



#include "testcase.h"

#if !defined(__SIZEOF_INT128__) || defined(LILY_SOFT_INT128)

#include "arith128.h"

int main() {
    i128_t mul;

    mul = mul128(I128_ZERO, I128_ZERO);
    TEST_EXPECT_INT(lo64(mul), 0);
    TEST_EXPECT_INT(hi64(mul), 0);

    mul = mul128(i128_pack(1, 0), i128_pack(1, 0));
    TEST_EXPECT_INT(lo64(mul), 1);
    TEST_EXPECT_INT(hi64(mul), 0);

    mul = mul128(i128_pack(1, 0), ui128(1));
    TEST_EXPECT_INT(lo64(mul), 0);
    TEST_EXPECT_INT(hi64(mul), 1);

    mul = mul128(ui128(1), i128_pack(-1, -1));
    TEST_EXPECT_INT(lo64(mul), 0);
    TEST_EXPECT_INT(hi64(mul), -1);

    mul = mul128(i128_pack(-1, -1), i128_pack(-1, -1));
    TEST_EXPECT_INT(lo64(mul), 1);
    TEST_EXPECT_INT(hi64(mul), 0);

    mul = mul128(i128_pack(1llu << 34, 0), i128_pack(1llu << 34, 0));
    TEST_EXPECT_INT(lo64(mul), 0);
    TEST_EXPECT_INT(hi64(mul), 1 << 4);

    TEST_OK();
}

#else

int main() {
    TEST_OK();
}

#endif
