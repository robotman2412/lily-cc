
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_preproc.h"
#include "c_std.h"
#include "c_tokenizer.h"
#include "testcase.h"



// Pull the next "real" token from a preprocessor, skipping whitespace,
// newlines, and placemarkers (which never leak past the macro pipeline in a
// well-formed expansion but are harmless to discard if they do).
static token_t pp_next(c_preproc_t *pre) {
    while (1) {
        token_t t = c_preproc_next(&pre->base);
        if (t.type == TOKENTYPE_WHITESPACE || t.type == TOKENTYPE_EOL) {
            tkn_delete(t);
            continue;
        }
        if (t.type == TOKENTYPE_OTHER && t.subtype == C_TKN_MARKER) {
            tkn_delete(t);
            continue;
        }
        return t;
    }
}

// Assert that the next real token is an identifier with the given spelling.
#define EXPECT_IDENT(pre, str)                                                                                         \
    do {                                                                                                               \
        token_t tmp = pp_next(pre);                                                                                    \
        EXPECT_INT(tmp.type, TOKENTYPE_IDENT);                                                                         \
        EXPECT_STR(tmp.strval, str);                                                                                   \
        tkn_delete(tmp);                                                                                               \
    } while (0)

// Assert that the next real token is an integer constant with the given value.
#define EXPECT_ICONST(pre, value)                                                                                      \
    do {                                                                                                               \
        token_t tmp = pp_next(pre);                                                                                    \
        EXPECT_INT(tmp.type, TOKENTYPE_ICONST);                                                                        \
        EXPECT_INT(tmp.ival, value);                                                                                   \
        tkn_delete(tmp);                                                                                               \
    } while (0)

// Assert that the next real token is a string literal with the given contents.
#define EXPECT_SCONST(pre, str)                                                                                        \
    do {                                                                                                               \
        token_t tmp = pp_next(pre);                                                                                    \
        EXPECT_INT(tmp.type, TOKENTYPE_SCONST);                                                                        \
        EXPECT_STR(tmp.strval, str);                                                                                   \
        tkn_delete(tmp);                                                                                               \
    } while (0)

// Assert that the next real token is the given C punctuation token.
#define EXPECT_PUNCT(pre, c_tkn)                                                                                       \
    do {                                                                                                               \
        token_t tmp = pp_next(pre);                                                                                    \
        EXPECT_INT(tmp.type, TOKENTYPE_OTHER);                                                                         \
        EXPECT_INT(tmp.subtype, c_tkn);                                                                                \
        tkn_delete(tmp);                                                                                               \
    } while (0)

// Assert that the preprocessor has reached EOF.
#define EXPECT_EOF(pre)                                                                                                \
    do {                                                                                                               \
        token_t tmp = pp_next(pre);                                                                                    \
        EXPECT_INT(tmp.type, TOKENTYPE_EOF);                                                                           \
        tkn_delete(tmp);                                                                                               \
    } while (0)


// __COUNTER__ produces a fresh integer each time.
static char *test_preproc_counter() {
    char const   data[] = "__COUNTER__\n__COUNTER__\n";
    cctx_t      *cctx   = cctx_create();
    srcfile_t   *src    = srcfile_create(cctx, "<test_preproc_counter>", data, sizeof(data) - 1);
    c_preproc_t *pre    = c_preproc_create(src, C_STD_max, false, false);

    EXPECT_ICONST(pre, 0);
    EXPECT_ICONST(pre, 1);
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_preproc_counter)


// Forward references inside a macro body are resolved at expansion time, so
// redefining a referenced macro between two expansions changes the result.
static char *test_preproc_object_macros() {
    // clang-format off
    char const data[] =
        "#define FOO BAR\n"
        "FOO\n"
        "#define BAR baz\n"
        "FOO\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_object_macros>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, C_STD_max, false, false);

    EXPECT_IDENT(pre, "BAR");
    EXPECT_IDENT(pre, "baz");
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_preproc_object_macros)


// `##` in an object-like macro pastes two identifiers, and a stray `#` in the
// body has no special meaning (no parameter to stringize).
static char *test_preproc_object_paste_and_hash() {
    // clang-format off
    char const data[] =
        "#define A0 foo##bar\n"
        "A0\n"
        "#define A1 #ok\n"
        "A1\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_object_paste_and_hash>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, C_STD_max, false, false);

    EXPECT_IDENT(pre, "foobar");
    EXPECT_PUNCT(pre, C_TKN_HASH);
    EXPECT_IDENT(pre, "ok");
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_preproc_object_paste_and_hash)


// Line-continuation backslashes are folded both inside a definition body and
// inside a reference to the macro.
static char *test_preproc_line_continuation() {
    // clang-format off
    char const data[] =
        "#define A2 foo\\\n"
        "bar\n"
        "A2\n"
        "A\\\n"
        "2\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_line_continuation>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, C_STD_max, false, false);

    EXPECT_IDENT(pre, "foobar");
    EXPECT_IDENT(pre, "foobar");
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_preproc_line_continuation)


// A function-like macro substitutes each parameter and emits the body's
// punctuation verbatim.
static char *test_preproc_func_basic() {
    // clang-format off
    char const data[] =
        "#define A(X, Y, Z) X | Y | Z\n"
        "A(a, b, c)\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_func_basic>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, C_STD_max, false, false);

    EXPECT_IDENT(pre, "a");
    EXPECT_PUNCT(pre, C_TKN_OR);
    EXPECT_IDENT(pre, "b");
    EXPECT_PUNCT(pre, C_TKN_OR);
    EXPECT_IDENT(pre, "c");
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_preproc_func_basic)


// `__VA_ARGS__` expansion.
static char *test_preproc_va_args() {
    // clang-format off
    char const data[] =
        "#define FOO(a, b, ...) __VA_ARGS__\n"
        "FOO(no1, no2, yes1, yes2)\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_va_args>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, C_STD_max, false, false);

    EXPECT_IDENT(pre, "yes1");
    EXPECT_PUNCT(pre, C_TKN_COMMA);
    EXPECT_IDENT(pre, "yes2");
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_preproc_va_args)


// `__VA_OPT__` expansion.
static char *test_preproc_va_opt() {
    // clang-format off
    char const data[] =
        "#define FOO(a, b, ...) __VA_OPT__((yes))\n"
        "FOO(no1, no2, yes1, yes2)\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_va_opt>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, C_STD_max, false, false);

    EXPECT_PUNCT(pre, C_TKN_LPAR);
    EXPECT_IDENT(pre, "yes");
    EXPECT_PUNCT(pre, C_TKN_RPAR);
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_preproc_va_opt)


// `##` between two identifier arguments fuses them into a single identifier.
static char *test_preproc_func_paste() {
    // clang-format off
    char const data[] =
        "#define PASTE(A, B) A##B\n"
        "PASTE(foo, bar)\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_func_paste>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, C_STD_max, false, false);

    EXPECT_IDENT(pre, "foobar");
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_preproc_func_paste)


// `#` stringizes its argument verbatim (no pre-expansion of the argument).
// `STR2` defers through another macro layer so the argument *is* expanded
// before stringization.
static char *test_preproc_stringize() {
    // clang-format off
    char const data[] =
        "#define STR(x)  # x\n"
        "#define STR2(x) STR(x)\n"
        "#define PASTE(A, B) A##B\n"
        "STR(This is some text)\n"
        "STR(PASTE(foo, bar))\n"
        "STR2(PASTE(foo, bar))\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_stringize>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, C_STD_max, false, false);

    EXPECT_SCONST(pre, "This is some text");
    EXPECT_SCONST(pre, "PASTE(foo, bar)");
    EXPECT_SCONST(pre, "foobar");
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_preproc_stringize)


// A macro that names itself transitively does not recurse: once the
// expansion of `F0()` re-encounters `F0`, that occurrence is left as a
// plain identifier followed by its parentheses.
static char *test_preproc_self_reference() {
    // clang-format off
    char const data[] =
        "#define F0() F1()\n"
        "#define F1   F0\n"
        "F0()\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_self_reference>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, C_STD_max, false, false);

    EXPECT_IDENT(pre, "F0");
    EXPECT_PUNCT(pre, C_TKN_LPAR);
    EXPECT_PUNCT(pre, C_TKN_RPAR);
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_preproc_self_reference)


// A function-like macro can supply its own opening parenthesis from another
// macro and match a closing parenthesis from the surrounding source. The
// chain Q2()()() collapses through three nullary macros to "fin".
static char *test_preproc_nested_calls() {
    // clang-format off
    char const data[] =
        "#define R2() fin\n"
        "#define R1() R2(\n"
        "#define R0 R1(\n"
        "R0))\n"
        "#define Q0() fin\n"
        "#define Q1() Q0\n"
        "#define Q2() Q1\n"
        "Q2()()()\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_nested_calls>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, C_STD_max, false, false);

    EXPECT_IDENT(pre, "fin");
    EXPECT_IDENT(pre, "fin");
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_preproc_nested_calls)


// `#if` evaluates a constant expression with C operator precedence. Both
// expressions here are false, so neither inner `#error` fires and no
// diagnostics are produced.
static char *test_preproc_if_arith() {
    // clang-format off
    char const data[] =
        "#if 2 + 2 * -1\n"
        "#error fail1\n"
        "#endif\n"
        "#if !0 - 1\n"
        "#error fail2\n"
        "#endif\n"
        "done\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_if_arith>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, C_STD_max, false, false);

    EXPECT_IDENT(pre, "done");
    EXPECT_EOF(pre);
    RETURN_ON_FALSE(cctx->diagnostics.len == 0);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_preproc_if_arith)


// The `defined` operator works with and without parentheses, and reports
// whether a macro is currently defined.
static char *test_preproc_defined_op() {
    // clang-format off
    char const data[] =
        "#define YES\n"
        "#if defined NO\n"
        "absent_no\n"
        "#endif\n"
        "#if defined ( NO )\n"
        "absent_no_paren\n"
        "#endif\n"
        "#if defined YES\n"
        "present_yes\n"
        "#endif\n"
        "#if defined ( YES )\n"
        "present_yes_paren\n"
        "#endif\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_defined_op>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, C_STD_max, false, false);

    EXPECT_IDENT(pre, "present_yes");
    EXPECT_IDENT(pre, "present_yes_paren");
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_preproc_defined_op)


// `#if` / `#elif` / `#else` selects exactly one branch. The first truthy
// condition wins, and `#else` only fires if every prior branch was false.
static char *test_preproc_if_branches() {
    // clang-format off
    char const data[] =
        "#if 1\n"
        "first\n"
        "#elif 1\n"
        "first_elif\n"
        "#else\n"
        "first_else\n"
        "#endif\n"
        "#if 0\n"
        "second\n"
        "#elif 1\n"
        "second_elif\n"
        "#else\n"
        "second_else\n"
        "#endif\n"
        "#if 0\n"
        "third\n"
        "#elif 0\n"
        "third_elif\n"
        "#else\n"
        "third_else\n"
        "#endif\n";
    // clang-format on
    cctx_t      *cctx = cctx_create();
    srcfile_t   *src  = srcfile_create(cctx, "<test_preproc_if_branches>", data, sizeof(data) - 1);
    c_preproc_t *pre  = c_preproc_create(src, C_STD_max, false, false);

    EXPECT_IDENT(pre, "first");
    EXPECT_IDENT(pre, "second_elif");
    EXPECT_IDENT(pre, "third_else");
    EXPECT_EOF(pre);

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_preproc_if_branches)
