
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "testcase.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>



static bool run_testcase(testcase_t *testcase) {
    printf("Test %s", testcase->id);
    fflush(stdout);
    char *res = testcase->function();
    printf(" %s\033[0m\n", !res ? "\033[32mOK" : "\033[31mFAILED");
    if (res && res != (void *)-1) {
        if (res[0] == 0xff) {
            printf("    %s\n", res + 1);
        } else {
            printf("    %s\n", res);
            free(res);
        }
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
            success += run_testcase(ent->value);
            ent      = map_next(&testcases, ent);
        }
    } else {
        for (int i = 1; i < argc; i++) {
            testcase_t *testcase = map_get(&testcases, argv[i]);
            if (!testcase) {
                printf("No such testcase %s\n", argv[i]);
                continue;
            }
            success += run_testcase(testcase);
            total++;
        }
    }
    if (total == 0) {
        printf("No test cases to run\n");
        return 0;
    } else {
        printf("%zu/%zu test cases succeeded\n", success, total);
        return success != total;
    }
}
