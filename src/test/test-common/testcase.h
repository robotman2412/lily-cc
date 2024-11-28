
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#pragma once

#include "map.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>



#ifndef __FILE_NAME__
#define __FILE_NAME__ __FILE__
#endif

#define TEST_OK          NULL
#define TEST_FAIL        ((void *)-1)
#define TEST_FAIL_MSG(x) (x)

#define STR_OF(x)        STR_OF_HELPER(x)
#define STR_OF_HELPER(x) #x

#define LILY_TEST_CASE(func)                                                                                           \
    __attribute__((constructor)) static void lily_testcase_##func() {                                                  \
        register_test_case(func, #func);                                                                               \
    }

#define RETURN_ON_FALSE(expr)                                                                                          \
    do {                                                                                                               \
        bool tmp = (expr);                                                                                             \
        if (!tmp)                                                                                                      \
            return "\xff" __FILE_NAME__ ":" STR_OF(__LINE__) ": " #expr;                                               \
    } while (0)

#define EXPECT_INT(expr, val)                                                                                          \
    do {                                                                                                               \
        long long expr_tmp = (expr);                                                                                   \
        long long val_tmp  = (val);                                                                                    \
        if (expr_tmp != val_tmp) {                                                                                     \
            return int_testcase_failed(__FILE_NAME__ ":" STR_OF(__LINE__) ": " #expr, expr_tmp, #val);                 \
        }                                                                                                              \
    } while (0)

#define EXPECT_CHAR(expr, val)                                                                                         \
    do {                                                                                                               \
        int expr_tmp = (expr);                                                                                         \
        int val_tmp  = (val);                                                                                          \
        if (expr_tmp != val_tmp) {                                                                                     \
            return char_testcase_failed(__FILE_NAME__ ":" STR_OF(__LINE__) ": " #expr, expr_tmp, #val);                \
        }                                                                                                              \
    } while (0)

#define EXPECT_STR(expr, val)                                                                                          \
    do {                                                                                                               \
        char const *expr_tmp = (expr);                                                                                 \
        char const *val_tmp  = (val);                                                                                  \
        if (strcmp(expr_tmp, val_tmp)) {                                                                               \
            return str_testcase_failed(                                                                                \
                __FILE_NAME__ ":" STR_OF(__LINE__) ": " #expr,                                                         \
                expr_tmp,                                                                                              \
                strlen(expr_tmp),                                                                                      \
                #val                                                                                                   \
            );                                                                                                         \
        }                                                                                                              \
    } while (0)

#define EXPECT_STR_L(expr, expr_len, val, val_len)                                                                     \
    do {                                                                                                               \
        char const *expr_tmp     = (expr);                                                                             \
        size_t      expr_len_tmp = (expr_len);                                                                         \
        char const *val_tmp      = (val);                                                                              \
        size_t      val_len_tmp  = (val_len);                                                                          \
        if (expr_len_tmp != val_len_tmp || memcmp(expr_tmp, val_tmp, expr_len_tmp)) {                                  \
            return str_testcase_failed(__FILE_NAME__ ":" STR_OF(__LINE__) ": " #expr, expr_tmp, expr_len_tmp, #val);   \
        }                                                                                                              \
    } while (0)

char *int_testcase_failed(char const *loc, long long value, char const *real);
char *char_testcase_failed(char const *loc, int value, char const *real);
char *str_testcase_failed(char const *loc, char const *value, size_t value_len, char const *real);


// Test case entry.
typedef struct {
    char const *(*function)();
    char const *id;
} testcase_t;

// Map of all registered test cases.
extern map_t testcases;

// Register a new test case.
void register_test_case(char const *(*function)(), char const *id);
