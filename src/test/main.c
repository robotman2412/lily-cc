
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "map.h"
#include "testcase.h"

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    char const *(*function)();
    char const *id;
} testcase_t;

map_t testcases = MAP_EMPTY;


void register_test_case(char const *(*function)(), char const *id) {
    testcase_t *ent = malloc(sizeof(testcase_t));
    ent->function   = function;
    ent->id         = id;
    map_set(&testcases, id, ent);
}

bool run_test_case(testcase_t *testcase) {
    printf("Test %s...", testcase->id);
    fflush(stdout);
    char const *res = testcase->function();
    printf("\b\b\b %s\033[0m\n", !res ? "\033[32mOK" : "\033[31mFAILED");
    if (res && res != (void *)-1) {
        printf("    %s\n", res);
    }
    return !res;
}

int main(int argc, char **argv) {
    size_t total   = 0;
    size_t success = 0;
    if (argc < 2) {
        map_ent_t const *ent = map_next(&testcases, NULL);
        while (ent) {
            total++;
            success += run_test_case(ent->value);
            ent      = map_next(&testcases, ent);
        }
    } else {
        for (int i = 1; i < argc; i++) {
            testcase_t *testcase  = map_get(&testcases, argv[i]);
            success              += run_test_case(testcase);
            total++;
        }
    }
    if (total == 0) {
        printf("No test cases to run\n");
        return 0;
    } else {
        printf("%d/%d test cases succeeded\n", success, total);
        return success != total;
    }
}
