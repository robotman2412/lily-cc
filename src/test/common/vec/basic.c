
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "testcase.h"
#include "vec.h"



int main() {
    vec_int_t vec = {0};

    vec_push(&vec, 1);
    TEST_EXPECT_INT(vec.len, 1u);
    TEST_EXPECT_INT(vec.arr[0], 1);

    vec_remove(&vec, 0);
    TEST_EXPECT_INT(vec.len, 0u);
    TEST_EXPECT_INT(vec.cap, 0u);
    TEST_ASSERT(vec.arr == NULL);

    TEST_OK();
}
