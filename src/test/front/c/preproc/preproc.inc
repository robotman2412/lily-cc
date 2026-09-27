
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "../options.inc"
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
        TEST_ASSERT(tmp.type == TOKENTYPE_IDENT, "%s", tokentype_name[tmp.type]);                                      \
        TEST_EXPECT_STR(tmp.strval, str);                                                                              \
        tkn_delete(tmp);                                                                                               \
    } while (0)

// Assert that the next real token is an integer constant with the given value.
#define EXPECT_ICONST(pre, value)                                                                                      \
    do {                                                                                                               \
        token_t tmp = pp_next(pre);                                                                                    \
        TEST_ASSERT(tmp.type == TOKENTYPE_ICONST, "%s", tokentype_name[tmp.type]);                                     \
        TEST_EXPECT_INT(tmp.ival, value);                                                                              \
        tkn_delete(tmp);                                                                                               \
    } while (0)

// Assert that the next real token is a string literal with the given contents.
#define EXPECT_SCONST(pre, str)                                                                                        \
    do {                                                                                                               \
        token_t tmp = pp_next(pre);                                                                                    \
        TEST_ASSERT(tmp.type == TOKENTYPE_SCONST, "%s", tokentype_name[tmp.type]);                                     \
        TEST_EXPECT_STR(tmp.strval, str);                                                                              \
        tkn_delete(tmp);                                                                                               \
    } while (0)

// Assert that the next real token is the given C punctuation token.
#define EXPECT_PUNCT(pre, c_tkn)                                                                                       \
    do {                                                                                                               \
        token_t tmp = pp_next(pre);                                                                                    \
        TEST_ASSERT(tmp.type == TOKENTYPE_OTHER, "%s", tokentype_name[tmp.type]);                                      \
        TEST_ASSERT(tmp.subtype == c_tkn, "%s", c_token_name[tmp.subtype]); /* NOLINT */                               \
        tkn_delete(tmp);                                                                                               \
    } while (0)

// Assert that the preprocessor has reached EOF.
#define EXPECT_EOF(pre)                                                                                                \
    do {                                                                                                               \
        token_t tmp = pp_next(pre);                                                                                    \
        TEST_ASSERT(tmp.type == TOKENTYPE_EOF, "%s", tokentype_name[tmp.type]);                                        \
        tkn_delete(tmp);                                                                                               \
    } while (0)
