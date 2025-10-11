
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "testcase.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>



static bool do_fork = true;

static bool run_testcase_impl(testcase_t *testcase) {
    printf("Test %s ", testcase->id);
    fflush(stdout);
    char *res = testcase->function();
    printf("%s\033[0m\n", !res ? "\033[32mOK" : "\033[31mFAILED");
    if (res && res != (void *)-1) {
        if (res[0] == (char)0xff) {
            printf("    %s\n", res + 1);
        } else {
            printf("    %s\n", res);
            free(res);
        }
    }
    return !res;
}

static bool fork_testcase(testcase_t *testcase) {
    int pid = fork();
    if (pid < 0) {
        perror(" \033[31mFork failed\033[0m");
        return false;
    } else if (pid == 0) {
        exit(!run_testcase_impl(testcase));
    } else {
        while (1) {
            int stat = 0;
            waitpid(pid, &stat, 0);
            if (WIFSIGNALED(stat)) {
                printf("\033[31m%s\033[0m\n", strsignal(WTERMSIG(stat)));
                return false;
            } else if (WIFEXITED(stat)) {
                return WEXITSTATUS(stat) == 0;
            }
        }
    }
}

static bool run_testcase(testcase_t *testcase) {
    return do_fork ? fork_testcase(testcase) : run_testcase_impl(testcase);
}

int main(int argc, char **argv) {
    char *val = getenv("LILY_TEST_FORK");
    if (val) {
        int fork = 0;
        sscanf(val, "%d", &fork);
        do_fork = fork != 0;
    }

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
