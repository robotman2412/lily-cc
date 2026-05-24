
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "lilycc_malloc.h"
#include "testcase.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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
        mkdir("lilycc_malloc", 0755);
        char buf[128];
        snprintf(buf, sizeof(buf) - 1, "lilycc_malloc/%s.log", testcase->id);
        lilycc_alloc_debugfd = fopen(buf, "w");

        size_t pre           = lilycc_total_alloc;
        bool   res           = run_testcase_impl(testcase);
        size_t post          = lilycc_total_alloc;
        lilycc_alloc_debugfd = NULL;
        if (post > pre) {
            printf("\033[33mTest %s leaked %zu bytes of memory\033[0m\n", testcase->id, post - pre);
        }
        exit(!res);
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

static bool matches(char const *pattern, char const *str) {
    if (!strcmp(pattern, str)) {
        return true;
    }
    while (1) {
        if (*pattern == '*') {
            while (*pattern == '*') {
                pattern++;
            }
            if (!*pattern) {
                return true;
            }
            for (; *str; str++) {
                if (matches(pattern, str)) {
                    return true;
                }
            }
            return false;
        }
        if (!*pattern || !*str) {
            return !*pattern && !*str;
        }
        if (*pattern != *str) {
            return false;
        }
        pattern++;
        str++;
    }
}

int main(int argc, char **argv) {
    char *val = getenv("LILY_TEST_FORK");
    if (val) {
        int fork = 0;
        sscanf(val, "%d", &fork);
        do_fork = fork != 0;
    }

    size_t pre = lilycc_total_alloc;
    if (!do_fork) {
        lilycc_alloc_debugfd = fopen("lilycc_malloc.log", "w");
    }

    size_t total   = 0;
    size_t success = 0;
    if (argc < 2) {
        total = testcases.len;
        for (size_t i = 0; i < testcases.len; i++) {
            success += run_testcase(&testcases.arr[i]);
        }
    } else {
        bool *found = calloc(1, argc);
        for (size_t i = 0; i < testcases.len; i++) {
            testcase_t *testcase = &testcases.arr[i];
            for (int x = 1; x < argc; x++) {
                if (matches(argv[x], testcase->id)) {
                    success += run_testcase(testcase);
                    total++;
                    found[x] = true;
                }
            }
        }
        for (int i = 1; i < argc; i++) {
            if (!found[i]) {
                printf("No test cases match %s\n", argv[i]);
            }
        }
        free(found);
    }
    size_t post = lilycc_total_alloc;
    if (!do_fork) {
        lilycc_alloc_debugfd = NULL;
    }
    if (!do_fork && post > pre) {
        printf("\033[33mLeaked %zu bytes of memory\033[0m\n", post - pre);
    }

    if (total == 0) {
        printf("No test cases to run\n");
        return 0;
    } else {
        printf("%zu/%zu test cases succeeded\n", success, total);
        return success != total;
    }
}
