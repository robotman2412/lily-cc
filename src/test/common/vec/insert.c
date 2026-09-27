
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "testcase.h"
#include "vec.h"



// Insert at the end, front and middle of a vector.
// Uses rawvec_insert_n directly because the vec_insert macro is currently incomplete.
int main() {
    vec_int_t vec = {0};

    // Insert into empty vector.
    int v = 42;
    rawvec_insert_n((rawvec_t *)&vec, sizeof(int), 0, &v, 1);
    TEST_EXPECT_INT(vec.len, 1u);
    TEST_EXPECT_INT(vec.arr[0], 42);

    // Insert at end (index == len).
    v = 99;
    rawvec_insert_n((rawvec_t *)&vec, sizeof(int), vec.len, &v, 1);
    TEST_EXPECT_INT(vec.len, 2u);
    TEST_EXPECT_INT(vec.arr[0], 42);
    TEST_EXPECT_INT(vec.arr[1], 99);

    // Insert at front.
    v = 7;
    rawvec_insert_n((rawvec_t *)&vec, sizeof(int), 0, &v, 1);
    TEST_EXPECT_INT(vec.len, 3u);
    TEST_EXPECT_INT(vec.arr[0], 7);
    TEST_EXPECT_INT(vec.arr[1], 42);
    TEST_EXPECT_INT(vec.arr[2], 99);

    // Insert several elements in the middle.
    int batch[3] = {1, 2, 3};
    rawvec_insert_n((rawvec_t *)&vec, sizeof(int), 2, batch, 3);
    TEST_EXPECT_INT(vec.len, 6u);
    TEST_EXPECT_INT(vec.arr[0], 7);
    TEST_EXPECT_INT(vec.arr[1], 42);
    TEST_EXPECT_INT(vec.arr[2], 1);
    TEST_EXPECT_INT(vec.arr[3], 2);
    TEST_EXPECT_INT(vec.arr[4], 3);
    TEST_EXPECT_INT(vec.arr[5], 99);

    vec_clear(&vec);
    TEST_OK();
}
