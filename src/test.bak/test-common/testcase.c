
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "testcase.h"

#include "char_repr.h"
#include "lilycc_malloc.h"
#include "vec.h"

#include <stdarg.h>
#include <stdio.h>



// Map of all registered test cases.
vec_testcase_t testcases;


static char *heap_sprintf(char const *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    size_t len = vsnprintf(NULL, 0, fmt, va);
    va_end(va);
    char *mem = lilycc_malloc(len + 1);
    va_start(va, fmt);
    vsnprintf(mem, len + 1, fmt, va);
    va_end(va);
    return mem;
}

void testcase_failed() {
    asm("");
}

char *int_testcase_failed(char const *loc, long long value, long long real, char const *real_str) {
    testcase_failed();
    return heap_sprintf("%s = %lld; expected %lld (%s)", loc, value, real, real_str);
}

char *char_testcase_failed(char const *loc, int value, char const *real) {
    testcase_failed();
    return heap_sprintf("%s = %c; expected %s", loc, value, real);
}

char *str_testcase_failed(char const *loc, char const *value, size_t value_len, char const *real) {
    testcase_failed();
    size_t len = format_cstr_repr(NULL, 0, value, value_len);
    char  *mem = lilycc_malloc(len + 1);
    format_cstr_repr(mem, len + 1, value, value_len);
    char *mem2 = lilycc_malloc(strlen(loc) + 4 + len + 12 + strlen(real) + 1);
    *mem2      = 0;
    strcat(mem2, loc);
    strcat(mem2, " = \"");
    strcat(mem2, mem);
    strcat(mem2, "\"; expected ");
    strcat(mem2, real);
    lilycc_free(mem);
    return mem2;
}

// Register a new test case.
void register_test_case(char *(*function)(), char const *id) {
    vec_push(
        &testcases,
        ((testcase_t){
            .function = function,
            .id       = id,
        })
    );
}
