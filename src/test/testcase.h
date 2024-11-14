
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#pragma once

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
        void register_test_case(char const *(*)(), char const *);                                                      \
        register_test_case(func, #func);                                                                               \
    }

#define RETURN_ON_FALSE(expr)                                                                                          \
    do {                                                                                                               \
        bool tmp = (expr);                                                                                             \
        if (!tmp)                                                                                                      \
            return __FILE_NAME__ ":" STR_OF(__LINE__) ": " #expr;                                                      \
    } while (0)

#define EXPECT_INT(expr, val)                                                                                          \
    do {                                                                                                               \
        long long expr_tmp = (expr);                                                                                   \
        long long val_tmp  = (val);                                                                                    \
        if (expr_tmp != val_tmp) {                                                                                     \
            char const *fmt = __FILE_NAME__ ":" STR_OF(__LINE__) ": " #expr " = %lld; expected %lld";                  \
            size_t      len = snprintf(NULL, 0, fmt, expr_tmp, val_tmp) + 1;                                           \
            char       *buf = malloc(len);                                                                             \
            snprintf(buf, len, fmt, expr_tmp, val_tmp);                                                                \
            return buf;                                                                                                \
        }                                                                                                              \
    } while (0)

#define EXPECT_CHAR(expr, val)                                                                                         \
    do {                                                                                                               \
        char expr_tmp = (expr);                                                                                        \
        char val_tmp  = (val);                                                                                         \
        if (expr_tmp != val_tmp) {                                                                                     \
            char const *fmt = __FILE_NAME__ ":" STR_OF(__LINE__) ": " #expr " = '%c'; expected '%c'";                  \
            size_t      len = snprintf(NULL, 0, fmt, expr_tmp, val_tmp) + 1;                                           \
            char       *buf = malloc(len);                                                                             \
            snprintf(buf, len, fmt, expr_tmp, val_tmp);                                                                \
            return buf;                                                                                                \
        }                                                                                                              \
    } while (0)

#define EXPECT_STR(expr, val)                                                                                          \
    do {                                                                                                               \
        char const *expr_tmp = (expr);                                                                                 \
        char const *val_tmp  = (val);                                                                                  \
        if (strcmp(expr_tmp, val_tmp)) {                                                                               \
            char const *fmt = __FILE_NAME__ ":" STR_OF(__LINE__) ": " #expr " = \"%s\"; expected \"%s\"";              \
            size_t      len = snprintf(NULL, 0, fmt, expr_tmp, val_tmp) + 1;                                           \
            char       *buf = malloc(len);                                                                             \
            snprintf(buf, len, fmt, expr_tmp, val_tmp);                                                                \
            return buf;                                                                                                \
        }                                                                                                              \
    } while (0)
