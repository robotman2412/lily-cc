
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "testcase.h"
#include "vec.h"

#include <limits.h>



static VEC_BSEARCH_DEF_DEFAULT(vec_bsearch_test, vec_int_t);

int main() {
    vec_int_t     vec = VEC_INIT;
    vec_bsearch_t res;

    vec_push(&vec, 1);
    vec_push(&vec, 2);
    vec_push(&vec, 4);
    vec_push(&vec, 5);

    res = vec_bsearch_test(&vec, INT_MIN);
    TEST_EXPECT_INT(res.index, 0u);
    TEST_ASSERT(res.found == false);

    res = vec_bsearch_test(&vec, 0);
    TEST_EXPECT_INT(res.index, 0u);
    TEST_ASSERT(res.found == false);

    res = vec_bsearch_test(&vec, 1);
    TEST_EXPECT_INT(res.index, 0u);
    TEST_ASSERT(res.found == true);

    res = vec_bsearch_test(&vec, 3);
    TEST_EXPECT_INT(res.index, 2u);
    TEST_ASSERT(res.found == false);

    res = vec_bsearch_test(&vec, 2);
    TEST_EXPECT_INT(res.index, 1u);
    TEST_ASSERT(res.found == true);

    res = vec_bsearch_test(&vec, 4);
    TEST_EXPECT_INT(res.index, 2u);
    TEST_ASSERT(res.found == true);

    res = vec_bsearch_test(&vec, 5);
    TEST_EXPECT_INT(res.index, 3u);
    TEST_ASSERT(res.found == true);

    res = vec_bsearch_test(&vec, 6);
    TEST_EXPECT_INT(res.index, 4u);
    TEST_ASSERT(res.found == false);

    res = vec_bsearch_test(&vec, INT_MAX);
    TEST_EXPECT_INT(res.index, 4u);
    TEST_ASSERT(res.found == false);

    TEST_OK();
}
