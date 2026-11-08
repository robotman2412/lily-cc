
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



#define TEST__FORMAT_IF_PRESENT(first, ...)     first
#define TEST__FORMATARGS_IF_PRESENT(first, ...) __VA_OPT__(, ) __VA_ARGS__

#define TEST_RETRHOW(expr)                                                                                             \
    do {                                                                                                               \
        int test_rethrow_rc_ = (expr);                                                                                 \
        if (test_rethrow_rc_) {                                                                                        \
            return test_rethrow_rc_;                                                                                   \
        }                                                                                                              \
    } while (0)

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
        __auto_type test_assert_rc_ = (expr);                                                                          \
        if (!test_assert_rc_) {                                                                                        \
            test_failure_message(__FILE_NAME__, __LINE__, #expr, "" __VA_ARGS__);                                      \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

#define TEST_COMPARE_(lhs_, compar, rhs_, fmt_cast, fmt_spec, fmt_spec_alt, ...)                                       \
    do {                                                                                                               \
        __auto_type test_compare_lhs_ = (lhs_);                                                                        \
        __auto_type test_compare_rhs_ = (rhs_);                                                                        \
        if (!(test_compare_lhs_ compar test_compare_rhs_)) {                                                           \
            test_failure_message(__FILE_NAME__, __LINE__, #lhs_ " " #compar " " #rhs_, "" __VA_ARGS__);                \
            test_failure_context(                                                                                      \
                "%s: " fmt_spec " (" fmt_spec_alt ")",                                                                 \
                #lhs_,                                                                                                 \
                fmt_cast test_compare_lhs_,                                                                            \
                fmt_cast test_compare_lhs_                                                                             \
            );                                                                                                         \
            test_failure_context(                                                                                      \
                "%s: " fmt_spec " (" fmt_spec_alt ")",                                                                 \
                #rhs_,                                                                                                 \
                fmt_cast test_compare_rhs_,                                                                            \
                fmt_cast test_compare_rhs_                                                                             \
            );                                                                                                         \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

#define TEST_COMPARE_INT(lhs_, compar, rhs_, ...)                                                                      \
    TEST_COMPARE_(lhs_, compar, rhs_, (long long), "%lld", "0x%llx" __VA_OPT__(, ) __VA_ARGS__)

#define TEST_COMPARE_UINT(lhs_, compar, rhs_, ...)                                                                     \
    TEST_COMPARE_(lhs_, compar, rhs_, (unsigned long long), "%llu", "0x%llx" __VA_OPT__(, ) __VA_ARGS__)

#define TEST_EXPECT_BOOL(lhs_, rhs_, ...) TEST_COMPARE_(lhs_, ==, rhs_, (_Bool), "%d", "%d" __VA_OPT__(, ) __VA_ARGS__)

#define TEST_EXPECT_INT(lhs_, rhs_, ...)                                                                               \
    TEST_COMPARE_(lhs_, ==, rhs_, (long long), "%lld", "0x%llx" __VA_OPT__(, ) __VA_ARGS__)

#define TEST_EXPECT_UINT(lhs_, rhs_, ...)                                                                              \
    TEST_COMPARE_(lhs_, ==, rhs_, (unsigned long long), "%llu", "0x%llx" __VA_OPT__(, ) __VA_ARGS__)

#define TEST_EXPECT_STR(lhs_, rhs_, ...)                                                                               \
    do {                                                                                                               \
        extern int  strcmp(char const *, char const *);                                                                \
        __auto_type text_expect_str_lhs_ = (lhs_);                                                                     \
        __auto_type text_expect_str_rhs_ = (rhs_);                                                                     \
        int         test_expect_str_rc_  = strcmp(text_expect_str_lhs_, text_expect_str_rhs_);                         \
        if (test_expect_str_rc_ != 0) {                                                                                \
            test_failure_message(__FILE_NAME__, __LINE__, "strcmp(" #lhs_ ", " #rhs_ ") == 0", "" __VA_ARGS__);        \
            test_failure_context(#lhs_ ": %s", text_expect_str_lhs_);                                                  \
            test_failure_context(#rhs_ ": %s", text_expect_str_rhs_);                                                  \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

#define TEST_EXPECT_STR_L(lhs_, lhs_len_, rhs_, rhs_len_, ...)                                                         \
    do {                                                                                                               \
        extern int  memcmp(void const *, void const *, unsigned long);                                                 \
        __auto_type test_expect_str_l_lhs_     = (lhs_);                                                               \
        __auto_type test_expect_str_l_lhs_len_ = (lhs_len_);                                                           \
        __auto_type test_expect_str_l_rhs_     = (rhs_);                                                               \
        __auto_type test_expect_str_l_rhs_len_ = (rhs_len_);                                                           \
        if (test_expect_str_l_lhs_len_ != test_expect_str_l_rhs_len_                                                   \
            || memcmp(test_expect_str_l_lhs_, test_expect_str_l_rhs_, test_expect_str_l_lhs_len_) != 0) {              \
            test_failure_message(                                                                                      \
                __FILE_NAME__,                                                                                         \
                __LINE__,                                                                                              \
                #lhs_len_ " == " #rhs_len_ " && memcmp(" #lhs_ ", " #rhs_ ", " #lhs_len_ ") == 0",                     \
                "" __VA_ARGS__                                                                                         \
            );                                                                                                         \
            test_failure_context(#lhs_ ": %.*s", (int)test_expect_str_l_lhs_len_, test_expect_str_l_lhs_);             \
            test_failure_context(#rhs_ ": %.*s", (int)test_expect_str_l_rhs_len_, test_expect_str_l_rhs_);             \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)
