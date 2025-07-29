
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "testcase.h"

#include "char_repr.h"
#include "strong_malloc.h"

#include <stdarg.h>
#include <stdio.h>



// Map of all registered test cases.
map_t testcases = STR_MAP_EMPTY;


static char *heap_sprintf(char const *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    size_t len = vsnprintf(NULL, 0, fmt, va);
    va_end(va);
    char *mem = strong_malloc(len + 1);
    va_start(va, fmt);
    vsnprintf(mem, len + 1, fmt, va);
    va_end(va);
    return mem;
}

void testcase_failed() {
    asm("");
}

char *int_testcase_failed(char const *loc, long long value, char const *real) {
    testcase_failed();
    return heap_sprintf("%s = %lld; expected %s", loc, value, real);
}

char *char_testcase_failed(char const *loc, int value, char const *real) {
    testcase_failed();
    return heap_sprintf("%s = %c; expected %s", loc, value, real);
}

char *str_testcase_failed(char const *loc, char const *value, size_t value_len, char const *real) {
    testcase_failed();
    size_t len = format_cstr_repr(NULL, 0, value, value_len);
    char  *mem = strong_malloc(len + 1);
    format_cstr_repr(mem, len + 1, value, value_len);
    char *mem2 = strong_malloc(strlen(loc) + 4 + len + 12 + strlen(real) + 1);
    *mem2      = 0;
    strcat(mem2, loc);
    strcat(mem2, " = \"");
    strcat(mem2, mem);
    strcat(mem2, "\"; expected ");
    strcat(mem2, real);
    free(mem);
    return mem2;
}

// Register a new test case.
void register_test_case(char *(*function)(), char const *id) {
    testcase_t *ent = malloc(sizeof(testcase_t));
    ent->function   = function;
    ent->id         = id;
    map_set(&testcases, id, ent);
}
