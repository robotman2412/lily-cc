
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "testcase.h"
#include "vec.h"



// Reserve should allocate capacity without growing length, and subsequent pushes
// up to that capacity should not require a reallocation.
int main() {
    vec_int_t vec = {0};

    vec_reserve(&vec, 16);
    TEST_EXPECT_INT(vec.len, 0u);
    TEST_ASSERT(vec.arr != NULL);
    TEST_ASSERT(vec.cap >= 16);

    size_t cap_before = vec.cap;
    int   *arr_before = vec.arr;
    for (int i = 0; i < 16; i++) {
        vec_push(&vec, i);
    }
    TEST_EXPECT_INT(vec.len, 16u);
    TEST_EXPECT_INT(vec.cap, cap_before);
    TEST_ASSERT(vec.arr == arr_before);
    for (int i = 0; i < 16; i++) {
        TEST_EXPECT_INT(vec.arr[i], i);
    }

    vec_clear(&vec);
    TEST_OK();
}
