
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include <stdarg.h>
#include <stdio.h>
#include <string.h>



void test_failure_message(char const *file, int line, char const *code, char const *fmt, ...) {
    fflush(stdout);
    fprintf(stderr, "%s:%d: ", file, line);
    if (code) {
        fprintf(stderr, "assertion %s failed", code);
    } else {
        fprintf(stderr, "testcase failed");
    }
    if (strlen(fmt) != 0) {
        fprintf(stderr, ": ");
        va_list l;
        va_start(l, fmt);
        vfprintf(stderr, fmt, l);
        va_end(l);
    }
    fprintf(stderr, "\n");
}

void test_failure_context(char const *fmt, ...) {
    fflush(stdout);
    fprintf(stderr, "  context: ");
    va_list l;
    va_start(l, fmt);
    vfprintf(stderr, fmt, l);
    va_end(l);
    fprintf(stderr, "\n");
}
