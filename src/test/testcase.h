
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once



#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#endif

#ifndef __FILE_NAME__
#define __FILE_NAME__ __FILE__
#endif

void test_failure_message(char const *file, int line, char const *code, char const *fmt, ...)
    __attribute__((format(printf, 4, 5)));
void test_failure_context(char const *fmt, ...) __attribute__((format(printf, 1, 2)));



#define TEST_OK()                                                                                                      \
    do {                                                                                                               \
        return 0;                                                                                                      \
    } while (0)

#define TEST_FAIL(...)                                                                                                 \
    do {                                                                                                               \
        test_failure_message(__FILE_NAME__, __LINE__, (char const *)0, "" __VA_ARGS__);                                \
        return 1;                                                                                                      \
    } while (0)

#define TEST_ASSERT(expr, ...)                                                                                         \
    do {                                                                                                               \
        __auto_type value = (expr);                                                                                    \
        if (!value) {                                                                                                  \
            test_failure_message(__FILE_NAME__, __LINE__, #expr, "" __VA_ARGS__);                                      \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

#define TEST_COMPARE_(lhs_, compar, rhs_, fmt_cast, fmt_spec, fmt_spec_alt, ...)                                       \
    do {                                                                                                               \
        __auto_type lhs = (lhs_);                                                                                      \
        __auto_type rhs = (rhs_);                                                                                      \
        if (!(lhs compar rhs)) {                                                                                       \
            test_failure_message(__FILE_NAME__, __LINE__, #lhs_ " " #compar " " #rhs_, "" __VA_ARGS__);                \
            test_failure_context(#lhs_ ": " fmt_spec " (" fmt_spec_alt ")", fmt_cast lhs, fmt_cast lhs);               \
            test_failure_context(#rhs_ ": " fmt_spec " (" fmt_spec_alt ")", fmt_cast rhs, fmt_cast rhs);               \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

#define TEST_COMPARE_INT(lhs_, compar, rhs_, ...)                                                                      \
    TEST_COMPARE_(lhs_, compar, rhs_, (long long), "%lld", "0x%llx" __VA_OPT__(, ) __VA_ARGS__)

#define TEST_COMPARE_UINT(lhs_, compar, rhs_, ...)                                                                     \
    TEST_COMPARE_(lhs_, compar, rhs_, (unsigned long long), "%llu", "0x%llx" __VA_OPT__(, ) __VA_ARGS__)

#define TEST_EXPECT_INT(lhs_, rhs_, ...)                                                                               \
    TEST_COMPARE_(lhs_, ==, rhs_, (long long), "%lld", "0x%llx" __VA_OPT__(, ) __VA_ARGS__)

#define TEST_EXPECT_UINT(lhs_, rhs_, ...)                                                                              \
    TEST_COMPARE_(lhs_, ==, rhs_, (unsigned long long), "%llu", "0x%llx" __VA_OPT__(, ) __VA_ARGS__)

#define TEST_EXPECT_STR(lhs_, rhs_, ...)                                                                               \
    do {                                                                                                               \
        extern int  strcmp(char const *, char const *);                                                                \
        __auto_type lhs        = (lhs_);                                                                               \
        __auto_type rhs        = (rhs_);                                                                               \
        int         difference = strcmp(lhs, rhs);                                                                     \
        if (difference != 0) {                                                                                         \
            test_failure_message(__FILE_NAME__, __LINE__, "strcmp(" #lhs_ ", " #rhs_ ") == 0", "" __VA_ARGS__);        \
            test_failure_context(#lhs_ ": %s", lhs);                                                                   \
            test_failure_context(#rhs_ ": %s", rhs);                                                                   \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

#define TEST_EXPECT_STR_L(lhs_, lhs_len_, rhs_, rhs_len_, ...)                                                         \
    do {                                                                                                               \
        extern int  memcmp(void const *, void const *, unsigned long);                                                 \
        __auto_type lhs     = (lhs_);                                                                                  \
        __auto_type lhs_len = (lhs_len_);                                                                              \
        __auto_type rhs     = (rhs_);                                                                                  \
        __auto_type rhs_len = (rhs_len_);                                                                              \
        if (lhs_len != rhs_len || memcmp(lhs, rhs, lhs_len) != 0) {                                                    \
            test_failure_message(                                                                                      \
                __FILE_NAME__,                                                                                         \
                __LINE__,                                                                                              \
                "memcmp(" #lhs_ ", " #rhs_ ", " #lhs_len_ ") == 0",                                                    \
                "" __VA_ARGS__                                                                                         \
            );                                                                                                         \
            test_failure_context(#lhs_ ": %.*s", (int)lhs_len, lhs);                                                   \
            test_failure_context(#rhs_ ": %.*s", (int)rhs_len, rhs);                                                   \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)
