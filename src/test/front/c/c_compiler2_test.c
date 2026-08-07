
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_ast.h"
#include "c_compile.h"
#include "c_compile_expr.h"
#include "c_compiler.h"
#include "c_ir.h"
#include "c_parser2.h"
#include "c_std.h"
#include "c_types.h"
#include "c_types1.h"
#include "compiler.h"
#include "list.h"
#include "testcase.h"
#include "tokenizer.h"

#include <stdio.h>

static c_options_t c_compiler2_test_options = {
    .c_std          = C_STD_max,
    .gnu_ext_enable = true,
    .char_is_signed = true,
    .short16        = true,
    .int32          = true,
    .long64         = true,
    .big_endian     = false,
    .size_type      = C_PRIM_SLONG,
};

#define TEST_DIAGNOSTICS()                                                                                             \
    if (cctx->diagnostics.len) {                                                                                       \
        fprintf(stdout, "\n");                                                                                         \
        dlist_foreach_node(diagnostic_t, diag, &cctx->diagnostics) {                                                   \
            print_diagnostic(diag, stdout);                                                                            \
        }                                                                                                              \
        goto fail;                                                                                                     \
    }

static char *c_compiler2_test_expr(char const *name, char const *source) {
    cctx_t       *cctx   = cctx_create();
    srcfile_t    *src    = srcfile_create(cctx, name, source, strlen(source));
    c_compiler_t *cc     = c_compiler_create(cctx, c_compiler2_test_options);
    tokenizer_t  *tkn    = c_tokenizer_create(cc, src, false);
    c_parser_t   *parser = c_parser_create(cc, tkn);
    cir_expr_t   *cir    = NULL;
    char         *res    = TEST_FAIL;

    c_ast_expr_t *ast = c_parse2_expr(parser);
    TEST_DIAGNOSTICS()

    cir_scope_t *scope = cir_scope_create(CIR_SCOPE_GLOBAL, NULL);
    cir                = c_compile2_expr(cc, scope, ast);
    cir_scope_delete(scope);
    TEST_DIAGNOSTICS()

    res = TEST_OK;

fail:
    if (cir) {
        cir_expr_delete(cir);
    }
    c_ast_expr_delete(ast);
    c_parser_delete(parser);
    c_compiler_delete(cc);
    cctx_delete(cctx);

    return res;
}

#define COMPILE_EXPR_TEST(name, source)                                                                                \
    static char *test_c_compile2_expr_##name() {                                                                       \
        return c_compiler2_test_expr("<c_compile2_expr_" #name ">", source);                                           \
    }                                                                                                                  \
    LILY_TEST_CASE(test_c_compile2_expr_##name)

static char *c_compiler2_test_type(char const *name, char const *source, uint64_t size, uint64_t align) {
    cctx_t       *cctx   = cctx_create();
    srcfile_t    *src    = srcfile_create(cctx, name, source, strlen(source));
    c_compiler_t *cc     = c_compiler_create(cctx, c_compiler2_test_options);
    tokenizer_t  *tkn    = c_tokenizer_create(cc, src, false);
    c_parser_t   *parser = c_parser_create(cc, tkn);
    cir_expr_t   *cir    = NULL;
    char         *res    = TEST_FAIL;
    uint64_t      actual_size, actual_align;
    bool          complete_type;

    c_ast_type_name_t *ast = c_parse2_type_name(parser);
    TEST_DIAGNOSTICS()

    cir_scope_t *scope = cir_scope_create(CIR_SCOPE_GLOBAL, NULL);
    c_type_t     type  = c_compile2_spec_qual_list(cc, ast->spec_qual, scope);
    if (c_type_is_valid(type) && ast->decl) {
        type = c_compile2_type(cc, scope, type, ast->decl, NULL);
    }
    if (c_type_is_valid(type)) {
        complete_type = c_type_get_size(cc, type, &actual_size, &actual_align);
        res           = TEST_OK;
    }
    c_type_delete(type);
    cir_scope_delete(scope);
    TEST_DIAGNOSTICS()

    res = TEST_OK;

fail:
    if (cir) {
        cir_expr_delete(cir);
    }
    c_ast_type_name_delete(ast);
    c_parser_delete(parser);
    c_compiler_delete(cc);
    cctx_delete(cctx);

    if (res == TEST_OK) {
        RETURN_ON_FALSE(complete_type);
        EXPECT_INT(actual_align, align);
        EXPECT_INT(actual_size, size);
    }

    return res;
}

#define COMPILE_TYPE_TEST(name, source, size, align)                                                                   \
    static char *test_c_compile2_type_##name() {                                                                       \
        return c_compiler2_test_type("<c_compile2_type_" #name ">", source, size, align);                              \
    }                                                                                                                  \
    LILY_TEST_CASE(test_c_compile2_type_##name)



// Arithmetic infix operators.
COMPILE_EXPR_TEST(add, "1 + 2")
COMPILE_EXPR_TEST(sub, "1 - 2")
COMPILE_EXPR_TEST(mul, "1 * 2")
COMPILE_EXPR_TEST(div, "1 / 2")
COMPILE_EXPR_TEST(mod, "1 % 2")

// Bitwise infix operators.
COMPILE_EXPR_TEST(shl, "1 << 2")
COMPILE_EXPR_TEST(shr, "1 >> 2")
COMPILE_EXPR_TEST(band, "1 & 2")
COMPILE_EXPR_TEST(bor, "1 | 2")
COMPILE_EXPR_TEST(bxor, "1 ^ 2")

// Logical infix operators.
COMPILE_EXPR_TEST(land, "1 && 2")
COMPILE_EXPR_TEST(lor, "1 || 2")

// Comparison infix operators.
COMPILE_EXPR_TEST(eq, "1 == 2")
COMPILE_EXPR_TEST(ne, "1 != 2")
COMPILE_EXPR_TEST(lt, "1 < 2")
COMPILE_EXPR_TEST(le, "1 <= 2")
COMPILE_EXPR_TEST(gt, "1 > 2")
COMPILE_EXPR_TEST(ge, "1 >= 2")

// Prefix operators (those not requiring an lvalue or pointer).
COMPILE_EXPR_TEST(pos, "+1")
COMPILE_EXPR_TEST(neg, "-1")
COMPILE_EXPR_TEST(bnot, "~1")
COMPILE_EXPR_TEST(lnot, "!1")

// Misc expressions.
COMPILE_EXPR_TEST(ternary, "0 ? 1 : 2")
COMPILE_EXPR_TEST(exprs, "(1, 2)")
COMPILE_EXPR_TEST(sizeof, "sizeof 1")
COMPILE_EXPR_TEST(sizeof_type, "sizeof(char)")
COMPILE_EXPR_TEST(alignof, "alignof 1")
COMPILE_EXPR_TEST(alignof_type, "alignof(char)")

// Primitive types.
COMPILE_TYPE_TEST(char, "char", 1, 1)
COMPILE_TYPE_TEST(uchar, "unsigned char", 1, 1)
COMPILE_TYPE_TEST(schar, "signed char", 1, 1)
COMPILE_TYPE_TEST(ushort, "unsigned short", 2, 2)
COMPILE_TYPE_TEST(sshort, "signed short", 2, 2)
COMPILE_TYPE_TEST(uint, "unsigned int", 4, 4)
COMPILE_TYPE_TEST(sint, "signed int", 4, 4)
COMPILE_TYPE_TEST(ulong, "unsigned long", 8, 8)
COMPILE_TYPE_TEST(slong, "signed long", 8, 8)
COMPILE_TYPE_TEST(ullong, "unsigned long long", 8, 8)
COMPILE_TYPE_TEST(sllong, "signed long long", 8, 8)
COMPILE_TYPE_TEST(u128, "unsigned __int128", 16, 16)
COMPILE_TYPE_TEST(s128, "signed __int128", 16, 16)

// Pointer types.
COMPILE_TYPE_TEST(ptr, "void *", 8, 8)

// Fixed-size array types.
COMPILE_TYPE_TEST(arr_of_ptr, "void *[4]", 32, 8)
COMPILE_TYPE_TEST(arr_of_char, "char[9]", 9, 1)
COMPILE_TYPE_TEST(arr2_of_char, "char[2][9]", 18, 1)

// Struct types.
COMPILE_TYPE_TEST(struct, "struct { char a; int b; }", 8, 4)
COMPILE_TYPE_TEST(union, "union { char a; int b; }", 4, 4)
COMPILE_TYPE_TEST(struct_nesting, "struct { char a; struct { long c; }; int b; }", 24, 8)
