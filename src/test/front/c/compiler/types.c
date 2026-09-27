
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "compiler.inc"

static int c_compiler_test_type(char const *name, char const *source, uint64_t *size_out, uint64_t *align_out) {
    cctx_t       *cctx   = cctx_create();
    srcfile_t    *src    = srcfile_create(cctx, name, source, strlen(source));
    c_compiler_t *cc     = c_compiler_create(cctx, c_test_options);
    tokenizer_t  *tkn    = c_tokenizer_create(cc, src, false);
    c_parser_t   *parser = c_parser_create(cc, tkn);
    int           res    = 1;

    c_ast_type_name_t *ast = c_parse_type_name(parser);
    if (c_compiler_test_diagnostics(cctx)) {
        goto fail;
    }

    cir_scope_t *scope = cir_scope_create(CIR_SCOPE_GLOBAL, NULL);
    c_type_t     type  = c_compile_spec_qual_list(cc, ast->spec_qual, scope);
    if (c_type_is_valid(type) && ast->decl) {
        type = c_compile_type(cc, scope, type, ast->decl, NULL);
    }
    if (c_type_is_valid(type) && c_type_get_size(cc, type, size_out, align_out)) {
        res = 0;
    }
    c_type_delete(type);
    cir_scope_delete(scope);
    if (c_compiler_test_diagnostics(cctx)) {
        res = 1;
    }

fail:
    c_ast_type_name_delete(ast);
    c_parser_delete(parser);
    c_compiler_delete(cc);
    cctx_delete(cctx);

    return res;
}

static int check_type(char const *name, char const *source, uint64_t size_exp, uint64_t align_exp) {
    uint64_t size, align;
    int      res = c_compiler_test_type(name, source, &size, &align);
    if (res) {
        return res;
    }
    TEST_EXPECT_UINT(align, align_exp, "%s", source);
    TEST_EXPECT_UINT(size, size_exp, "%s", source);
    TEST_OK();
}

int main() {
    int failed = 0;

    // Primitive types.
    failed += check_type("<c_compile_type_char>", "char", 1, 1);
    failed += check_type("<c_compile_type_uchar>", "unsigned char", 1, 1);
    failed += check_type("<c_compile_type_schar>", "signed char", 1, 1);
    failed += check_type("<c_compile_type_ushort>", "unsigned short", 2, 2);
    failed += check_type("<c_compile_type_sshort>", "signed short", 2, 2);
    failed += check_type("<c_compile_type_uint>", "unsigned int", 4, 4);
    failed += check_type("<c_compile_type_sint>", "signed int", 4, 4);
    failed += check_type("<c_compile_type_ulong>", "unsigned long", 8, 8);
    failed += check_type("<c_compile_type_slong>", "signed long", 8, 8);
    failed += check_type("<c_compile_type_ullong>", "unsigned long long", 8, 8);
    failed += check_type("<c_compile_type_sllong>", "signed long long", 8, 8);
    failed += check_type("<c_compile_type_u128>", "unsigned __int128", 16, 16);
    failed += check_type("<c_compile_type_s128>", "signed __int128", 16, 16);

    // Pointer types.
    failed += check_type("<c_compile_type_ptr>", "void *", 8, 8);

    // Fixed-size array types.
    failed += check_type("<c_compile_type_arr_of_ptr>", "void *[4]", 32, 8);
    failed += check_type("<c_compile_type_arr_of_char>", "char[9]", 9, 1);
    failed += check_type("<c_compile_type_arr2_of_char>", "char[2][9]", 18, 1);

    // Struct types.
    failed += check_type("<c_compile_type_struct>", "struct { char a; int b; }", 8, 4);
    failed += check_type("<c_compile_type_union>", "union { char a; int b; }", 4, 4);
    failed += check_type("<c_compile_type_struct_nesting>", "struct { char a; struct { long c; }; int b; }", 24, 8);

    return failed;
}
