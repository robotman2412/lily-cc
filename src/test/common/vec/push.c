
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "testcase.h"
#include "vec.h"



int main() {
    size_t const len = 100000;
    vec_size_t   vec = {0};

    for (size_t i = 0; i < len; i++) {
        vec_push(&vec, i);
    }
    TEST_EXPECT_INT(vec.len, len);
    for (size_t i = 0; i < len; i++) {
        TEST_EXPECT_INT(vec.arr[i], i);
    }

    vec_clear(&vec);
    TEST_EXPECT_INT(vec.len, 0u);
    TEST_EXPECT_INT(vec.cap, 0u);
    TEST_ASSERT(vec.arr == NULL);

    TEST_OK();
}
