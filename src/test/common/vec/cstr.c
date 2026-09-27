
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "testcase.h"
#include "vec.h"



// Exercise the vector with a non-trivial element type (pointers) and interleave
// pushes and removes; the contents must always reflect the operations performed.
int main() {
    vec_cstr_t vec = {0};

    vec_push(&vec, "alpha");
    vec_push(&vec, "beta");
    vec_push(&vec, "gamma");
    vec_push(&vec, "delta");
    TEST_EXPECT_INT(vec.len, 4u);
    TEST_EXPECT_STR(vec.arr[0], "alpha");
    TEST_EXPECT_STR(vec.arr[1], "beta");
    TEST_EXPECT_STR(vec.arr[2], "gamma");
    TEST_EXPECT_STR(vec.arr[3], "delta");

    // Remove "beta" from the middle.
    char *removed = vec_remove(&vec, 1);
    TEST_EXPECT_STR(removed, "beta");
    TEST_EXPECT_INT(vec.len, 3u);
    TEST_EXPECT_STR(vec.arr[0], "alpha");
    TEST_EXPECT_STR(vec.arr[1], "gamma");
    TEST_EXPECT_STR(vec.arr[2], "delta");

    // Push something new on the end.
    vec_push(&vec, "epsilon");
    TEST_EXPECT_INT(vec.len, 4u);
    TEST_EXPECT_STR(vec.arr[3], "epsilon");

    vec_clear(&vec);
    TEST_OK();
}
