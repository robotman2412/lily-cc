
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "testcase.h"
#include "vec.h"



static char *test_vec_basic() {
    vec_int_t vec = {0};

    vec_push(&vec, 1);
    EXPECT_INT(vec.len, 1);
    EXPECT_INT(vec.arr[0], 1);

    vec_remove(&vec, 0);
    EXPECT_INT(vec.len, 0);
    EXPECT_INT(vec.cap, 0);
    RETURN_ON_FALSE(vec.arr == NULL);

    return TEST_OK;
}
LILY_TEST_CASE(test_vec_basic)



static char *test_vec_push() {
    size_t const len = 100000;
    vec_int_t    vec = {0};

    for (size_t i = 0; i < len; i++) {
        vec_push(&vec, i);
    }
    EXPECT_INT(vec.len, len);
    for (size_t i = 0; i < len; i++) {
        EXPECT_INT(vec.arr[i], i);
    }

    vec_clear(&vec);
    EXPECT_INT(vec.len, 0);
    EXPECT_INT(vec.cap, 0);
    RETURN_ON_FALSE(vec.arr == NULL);

    return TEST_OK;
}
LILY_TEST_CASE(test_vec_push)



// Remove from front, middle and back; verify remaining elements keep correct order.
static char *test_vec_remove_positions() {
    vec_int_t vec = {0};

    for (int i = 0; i < 5; i++) {
        vec_push(&vec, i * 10);
    }
    EXPECT_INT(vec.len, 5);

    // Remove from middle: [0, 10, 20, 30, 40] -> [0, 10, 30, 40]
    int removed = vec_remove(&vec, 2);
    EXPECT_INT(removed, 20);
    EXPECT_INT(vec.len, 4);
    EXPECT_INT(vec.arr[0], 0);
    EXPECT_INT(vec.arr[1], 10);
    EXPECT_INT(vec.arr[2], 30);
    EXPECT_INT(vec.arr[3], 40);

    // Remove from front: [0, 10, 30, 40] -> [10, 30, 40]
    removed = vec_remove(&vec, 0);
    EXPECT_INT(removed, 0);
    EXPECT_INT(vec.len, 3);
    EXPECT_INT(vec.arr[0], 10);
    EXPECT_INT(vec.arr[1], 30);
    EXPECT_INT(vec.arr[2], 40);

    // Remove from back: [10, 30, 40] -> [10, 30]
    removed = vec_remove(&vec, vec.len - 1);
    EXPECT_INT(removed, 40);
    EXPECT_INT(vec.len, 2);
    EXPECT_INT(vec.arr[0], 10);
    EXPECT_INT(vec.arr[1], 30);

    vec_clear(&vec);
    return TEST_OK;
}
LILY_TEST_CASE(test_vec_remove_positions)



// Remove the front of a two-element vector then the last remaining element.
// The first remove must shift the tail element down; the second must release the buffer.
static char *test_vec_remove_to_empty() {
    vec_int_t vec = {0};

    vec_push(&vec, 100);
    vec_push(&vec, 200);
    EXPECT_INT(vec.len, 2);

    int removed = vec_remove(&vec, 0);
    EXPECT_INT(removed, 100);
    EXPECT_INT(vec.len, 1);
    EXPECT_INT(vec.arr[0], 200);

    removed = vec_remove(&vec, 0);
    EXPECT_INT(removed, 200);
    EXPECT_INT(vec.len, 0);
    EXPECT_INT(vec.cap, 0);
    RETURN_ON_FALSE(vec.arr == NULL);

    return TEST_OK;
}
LILY_TEST_CASE(test_vec_remove_to_empty)



// Reserve should allocate capacity without growing length, and subsequent pushes
// up to that capacity should not require a reallocation.
static char *test_vec_reserve() {
    vec_int_t vec = {0};

    vec_reserve(&vec, 16);
    EXPECT_INT(vec.len, 0);
    RETURN_ON_FALSE(vec.arr != NULL);
    RETURN_ON_FALSE(vec.cap >= 16);

    size_t cap_before = vec.cap;
    int   *arr_before = vec.arr;
    for (int i = 0; i < 16; i++) {
        vec_push(&vec, i);
    }
    EXPECT_INT(vec.len, 16);
    EXPECT_INT(vec.cap, cap_before);
    RETURN_ON_FALSE(vec.arr == arr_before);
    for (int i = 0; i < 16; i++) {
        EXPECT_INT(vec.arr[i], i);
    }

    vec_clear(&vec);
    return TEST_OK;
}
LILY_TEST_CASE(test_vec_reserve)



// Insert at the end, front and middle of a vector.
// Uses rawvec_insert_n directly because the vec_insert macro is currently incomplete.
static char *test_vec_insert() {
    vec_int_t vec = {0};

    // Insert into empty vector.
    int v = 42;
    rawvec_insert_n((rawvec_t *)&vec, sizeof(int), 0, &v, 1);
    EXPECT_INT(vec.len, 1);
    EXPECT_INT(vec.arr[0], 42);

    // Insert at end (index == len).
    v = 99;
    rawvec_insert_n((rawvec_t *)&vec, sizeof(int), vec.len, &v, 1);
    EXPECT_INT(vec.len, 2);
    EXPECT_INT(vec.arr[0], 42);
    EXPECT_INT(vec.arr[1], 99);

    // Insert at front.
    v = 7;
    rawvec_insert_n((rawvec_t *)&vec, sizeof(int), 0, &v, 1);
    EXPECT_INT(vec.len, 3);
    EXPECT_INT(vec.arr[0], 7);
    EXPECT_INT(vec.arr[1], 42);
    EXPECT_INT(vec.arr[2], 99);

    // Insert several elements in the middle.
    int batch[3] = {1, 2, 3};
    rawvec_insert_n((rawvec_t *)&vec, sizeof(int), 2, batch, 3);
    EXPECT_INT(vec.len, 6);
    EXPECT_INT(vec.arr[0], 7);
    EXPECT_INT(vec.arr[1], 42);
    EXPECT_INT(vec.arr[2], 1);
    EXPECT_INT(vec.arr[3], 2);
    EXPECT_INT(vec.arr[4], 3);
    EXPECT_INT(vec.arr[5], 99);

    vec_clear(&vec);
    return TEST_OK;
}
LILY_TEST_CASE(test_vec_insert)



// Exercise the vector with a non-trivial element type (pointers) and interleave
// pushes and removes; the contents must always reflect the operations performed.
static char *test_vec_cstr() {
    vec_cstr_t vec = {0};

    vec_push(&vec, "alpha");
    vec_push(&vec, "beta");
    vec_push(&vec, "gamma");
    vec_push(&vec, "delta");
    EXPECT_INT(vec.len, 4);
    EXPECT_STR(vec.arr[0], "alpha");
    EXPECT_STR(vec.arr[1], "beta");
    EXPECT_STR(vec.arr[2], "gamma");
    EXPECT_STR(vec.arr[3], "delta");

    // Remove "beta" from the middle.
    char *removed = vec_remove(&vec, 1);
    EXPECT_STR(removed, "beta");
    EXPECT_INT(vec.len, 3);
    EXPECT_STR(vec.arr[0], "alpha");
    EXPECT_STR(vec.arr[1], "gamma");
    EXPECT_STR(vec.arr[2], "delta");

    // Push something new on the end.
    vec_push(&vec, "epsilon");
    EXPECT_INT(vec.len, 4);
    EXPECT_STR(vec.arr[3], "epsilon");

    vec_clear(&vec);
    return TEST_OK;
}
LILY_TEST_CASE(test_vec_cstr)
