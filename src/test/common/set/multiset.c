
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "set.h"
#include "testcase.h"



int main() {
    set_t a = STR_SET_EMPTY;
    set_t b = STR_SET_EMPTY;

    set_add(&a, "Hi!");
    set_add(&a, "Hello!");

    set_add(&b, "Hi!");
    set_add(&b, "Yes");
    set_add(&b, "No");

    set_removeall(&b, &a);
    TEST_ASSERT(!set_contains(&b, "Hi!"));
    TEST_ASSERT(set_contains(&b, "Yes"));
    TEST_ASSERT(set_contains(&b, "No"));

    set_addall(&a, &b);
    TEST_ASSERT(set_contains(&a, "Yes"));
    TEST_ASSERT(set_contains(&a, "No"));
    TEST_ASSERT(set_contains(&a, "Hi!"));
    TEST_ASSERT(set_contains(&a, "Hello!"));

    set_t c = STR_SET_EMPTY;

    set_add(&c, "True");
    set_add(&c, "False");
    set_add(&c, "Yes");
    set_add(&c, "No");

    set_intersect(&c, &a);
    TEST_ASSERT(!set_contains(&c, "True"));
    TEST_ASSERT(!set_contains(&c, "False"));
    TEST_ASSERT(set_contains(&c, "Yes"));
    TEST_ASSERT(set_contains(&c, "No"));
    TEST_ASSERT(!set_contains(&c, "Hi!"));
    TEST_ASSERT(!set_contains(&c, "Hello!"));

    set_clear(&a);
    set_clear(&b);
    set_clear(&c);

    TEST_OK();
}
