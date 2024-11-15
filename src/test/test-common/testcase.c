
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "testcase.h"



// Map of all registered test cases.
map_t testcases = MAP_EMPTY;


// Register a new test case.
void register_test_case(char const *(*function)(), char const *id) {
    testcase_t *ent = malloc(sizeof(testcase_t));
    ent->function   = function;
    ent->id         = id;
    map_set(&testcases, id, ent);
}
