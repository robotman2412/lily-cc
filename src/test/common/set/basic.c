
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "set.h"
#include "testcase.h"

#include <stdbool.h>
#include <stddef.h>



int main() {
    set_t set = PTR_SET_EMPTY;

    // Shouldn't crash.
    set_foreach(void, varname, &set);
    // Shouldn't crash.
    set_remove(&set, NULL);

    // Try adding some items.
    TEST_ASSERT(set_add(&set, (void *)1));
    TEST_ASSERT(set_add(&set, (void *)9));
    TEST_ASSERT(!set_add(&set, (void *)1));
    TEST_ASSERT(set_add(&set, (void *)31));
    TEST_ASSERT(!set_add(&set, (void *)9));


    // Test set_foreach.
    struct {
        void *expect;
        int   count;
    } expect[]              = {{(void *)1, 0}, {(void *)9, 0}, {(void *)31, 0}};
    size_t const expect_len = sizeof(expect) / sizeof(*expect);

    set_foreach(void, value, &set) {
        bool found = false;
        for (size_t i = 0; i < expect_len; i++) {
            if (expect[i].expect == value) {
                expect[i].count++;
                found = true;
            }
        }
        TEST_ASSERT(found);
    }

    for (size_t i = 0; i < expect_len; i++) {
        TEST_EXPECT_INT(expect[i].count, 1);
    }

    // Test set_contains.
    TEST_ASSERT(set_contains(&set, (void *)1));
    TEST_ASSERT(set_contains(&set, (void *)9));
    TEST_ASSERT(set_contains(&set, (void *)31));
    TEST_ASSERT(!set_contains(&set, (void *)125));
    TEST_ASSERT(!set_contains(&set, (void *)14569));

    // Test set_remove.
    TEST_ASSERT(set_remove(&set, (void *)1));
    TEST_ASSERT(!set_remove(&set, (void *)1));
    TEST_ASSERT(!set_contains(&set, (void *)1));

    // Test set_clear.
    set_clear(&set);
    TEST_EXPECT_INT(set.len, 0u);
    TEST_EXPECT_INT(set.buckets_len, 0u);
    TEST_ASSERT(set.buckets == NULL);

    TEST_OK();
}
