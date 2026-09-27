
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "testcase.h"
#include "vec.h"



// Remove from front, middle and back; verify remaining elements keep correct order.
int main() {
    vec_int_t vec = {0};

    for (int i = 0; i < 5; i++) {
        vec_push(&vec, i * 10);
    }
    TEST_EXPECT_INT(vec.len, 5u);

    // Remove from middle: [0, 10, 20, 30, 40] -> [0, 10, 30, 40]
    int removed = vec_remove(&vec, 2);
    TEST_EXPECT_INT(removed, 20);
    TEST_EXPECT_INT(vec.len, 4u);
    TEST_EXPECT_INT(vec.arr[0], 0);
    TEST_EXPECT_INT(vec.arr[1], 10);
    TEST_EXPECT_INT(vec.arr[2], 30);
    TEST_EXPECT_INT(vec.arr[3], 40);

    // Remove from front: [0, 10, 30, 40] -> [10, 30, 40]
    removed = vec_remove(&vec, 0);
    TEST_EXPECT_INT(removed, 0);
    TEST_EXPECT_INT(vec.len, 3u);
    TEST_EXPECT_INT(vec.arr[0], 10);
    TEST_EXPECT_INT(vec.arr[1], 30);
    TEST_EXPECT_INT(vec.arr[2], 40);

    // Remove from back: [10, 30, 40] -> [10, 30]
    removed = vec_remove(&vec, vec.len - 1);
    TEST_EXPECT_INT(removed, 40);
    TEST_EXPECT_INT(vec.len, 2u);
    TEST_EXPECT_INT(vec.arr[0], 10);
    TEST_EXPECT_INT(vec.arr[1], 30);

    vec_clear(&vec);
    TEST_OK();
}
