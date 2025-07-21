
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "set.h"
#include "testcase.h"



static char *test_set_basic() {
    set_t set = PTR_SET_EMPTY;

    // Shouldn't crash.
    set_foreach(void, varname, &set);
    // Shouldn't crash.
    set_remove(&set, NULL);

    // Try adding some items.
    RETURN_ON_FALSE(set_add(&set, (void *)1));
    RETURN_ON_FALSE(set_add(&set, (void *)9));
    RETURN_ON_FALSE(!set_add(&set, (void *)1));
    RETURN_ON_FALSE(set_add(&set, (void *)31));
    RETURN_ON_FALSE(!set_add(&set, (void *)9));


    // Test set_foreach.
    struct {
        void *expect;
        int   count;
    } expect[]              = {{(void *)1}, {(void *)9}, {(void *)31}};
    size_t const expect_len = sizeof(expect) / sizeof(*expect);

    set_foreach(void, value, &set) {
        bool found = false;
        for (size_t i = 0; i < expect_len; i++) {
            if (expect[i].expect == value) {
                expect[i].count++;
                found = true;
            }
        }
        RETURN_ON_FALSE(found);
    }

    for (size_t i = 0; i < expect_len; i++) {
        EXPECT_INT(expect[i].count, 1);
    }

    // Test set_contains.
    RETURN_ON_FALSE(set_contains(&set, (void *)1));
    RETURN_ON_FALSE(set_contains(&set, (void *)9));
    RETURN_ON_FALSE(set_contains(&set, (void *)31));
    RETURN_ON_FALSE(!set_contains(&set, (void *)125));
    RETURN_ON_FALSE(!set_contains(&set, (void *)14569));

    // Test set_remove.
    RETURN_ON_FALSE(set_remove(&set, (void *)1));
    RETURN_ON_FALSE(!set_remove(&set, (void *)1));
    RETURN_ON_FALSE(!set_contains(&set, (void *)1));

    // Test set_clear.
    set_clear(&set);
    EXPECT_INT(set.len, 0);
    EXPECT_INT(set.buckets_len, 0);
    RETURN_ON_FALSE(set.buckets == NULL);

    return TEST_OK;
}
LILY_TEST_CASE(test_set_basic)



static char *test_multiset() {
    set_t a = STR_SET_EMPTY;
    set_t b = STR_SET_EMPTY;

    set_add(&a, "Hi!");
    set_add(&a, "Hello!");

    set_add(&b, "Hi!");
    set_add(&b, "Yes");
    set_add(&b, "No");

    set_removeall(&b, &a);
    RETURN_ON_FALSE(!set_contains(&b, "Hi!"));
    RETURN_ON_FALSE(set_contains(&b, "Yes"));
    RETURN_ON_FALSE(set_contains(&b, "No"));

    set_addall(&a, &b);
    RETURN_ON_FALSE(set_contains(&a, "Yes"));
    RETURN_ON_FALSE(set_contains(&a, "No"));
    RETURN_ON_FALSE(set_contains(&a, "Hi!"));
    RETURN_ON_FALSE(set_contains(&a, "Hello!"));

    set_t c = STR_SET_EMPTY;

    set_add(&c, "True");
    set_add(&c, "False");
    set_add(&c, "Yes");
    set_add(&c, "No");

    set_intersect(&c, &a);
    RETURN_ON_FALSE(!set_contains(&c, "True"));
    RETURN_ON_FALSE(!set_contains(&c, "False"));
    RETURN_ON_FALSE(set_contains(&c, "Yes"));
    RETURN_ON_FALSE(set_contains(&c, "No"));
    RETURN_ON_FALSE(!set_contains(&c, "Hi!"));
    RETURN_ON_FALSE(!set_contains(&c, "Hello!"));

    set_clear(&a);
    set_clear(&b);

    return TEST_OK;
}
LILY_TEST_CASE(test_multiset)



static char *test_large_set() {
    set_t set = PTR_SET_EMPTY;

    for (size_t i = 0; i < 1000000; i++) {
        RETURN_ON_FALSE(set_add(&set, (void *)i));
    }
    for (size_t i = 0; i < 1000000; i++) {
        RETURN_ON_FALSE(set_contains(&set, (void *)i));
    }
    for (size_t i = 500000; i < 750000; i++) {
        RETURN_ON_FALSE(set_remove(&set, (void *)i));
    }

    set_clear(&set);

    return TEST_OK;
}
LILY_TEST_CASE(test_large_set)
