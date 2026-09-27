
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "set.h"
#include "testcase.h"

#include <stddef.h>



int main() {
    set_t set = PTR_SET_EMPTY;

    for (size_t i = 0; i < 100000; i++) {
        TEST_ASSERT(set_add(&set, (void *)i));
    }
    for (size_t i = 0; i < 100000; i++) {
        TEST_ASSERT(set_contains(&set, (void *)i));
    }
    for (size_t i = 50000; i < 75000; i++) {
        TEST_ASSERT(set_remove(&set, (void *)i));
    }

    set_clear(&set);

    TEST_OK();
}
