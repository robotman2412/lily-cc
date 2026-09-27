
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "testcase.h"
#include "vec.h"



// Remove the front of a two-element vector then the last remaining element.
// The first remove must shift the tail element down; the second must release the buffer.
int main() {
    vec_int_t vec = {0};

    vec_push(&vec, 100);
    vec_push(&vec, 200);
    TEST_EXPECT_INT(vec.len, 2u);

    int removed = vec_remove(&vec, 0);
    TEST_EXPECT_INT(removed, 100);
    TEST_EXPECT_INT(vec.len, 1u);
    TEST_EXPECT_INT(vec.arr[0], 200);

    removed = vec_remove(&vec, 0);
    TEST_EXPECT_INT(removed, 200);
    TEST_EXPECT_INT(vec.len, 0u);
    TEST_EXPECT_INT(vec.cap, 0u);
    TEST_ASSERT(vec.arr == NULL);

    TEST_OK();
}
