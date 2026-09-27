
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "compiler.inc"

static int c_compiler_test_expr(char const *name, char const *source) {
    cctx_t       *cctx   = cctx_create();
    srcfile_t    *src    = srcfile_create(cctx, name, source, strlen(source));
    c_compiler_t *cc     = c_compiler_create(cctx, c_test_options);
    tokenizer_t  *tkn    = c_tokenizer_create(cc, src, false);
    c_parser_t   *parser = c_parser_create(cc, tkn);
    cir_expr_t   *cir    = NULL;
    int           res    = 1;

    c_ast_expr_t *ast = c_parse_expr(parser);
    if (c_compiler_test_diagnostics(cctx)) {
        goto fail;
    }

    cir_scope_t *scope = cir_scope_create(CIR_SCOPE_GLOBAL, NULL);
    cir                = c_compile_expr(cc, scope, ast);
    cir_scope_delete(scope);
    if (c_compiler_test_diagnostics(cctx)) {
        goto fail;
    }

    res = 0;

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

int main() {
    int failed = 0;

    // Arithmetic infix operators.
    failed += c_compiler_test_expr("<c_compile_expr_add>", "1 + 2");
    failed += c_compiler_test_expr("<c_compile_expr_sub>", "1 - 2");
    failed += c_compiler_test_expr("<c_compile_expr_mul>", "1 * 2");
    failed += c_compiler_test_expr("<c_compile_expr_div>", "1 / 2");
    failed += c_compiler_test_expr("<c_compile_expr_mod>", "1 % 2");

    // Bitwise infix operators.
    failed += c_compiler_test_expr("<c_compile_expr_shl>", "1 << 2");
    failed += c_compiler_test_expr("<c_compile_expr_shr>", "1 >> 2");
    failed += c_compiler_test_expr("<c_compile_expr_band>", "1 & 2");
    failed += c_compiler_test_expr("<c_compile_expr_bor>", "1 | 2");
    failed += c_compiler_test_expr("<c_compile_expr_bxor>", "1 ^ 2");

    // Logical infix operators.
    failed += c_compiler_test_expr("<c_compile_expr_land>", "1 && 2");
    failed += c_compiler_test_expr("<c_compile_expr_lor>", "1 || 2");

    // Comparison infix operators.
    failed += c_compiler_test_expr("<c_compile_expr_eq>", "1 == 2");
    failed += c_compiler_test_expr("<c_compile_expr_ne>", "1 != 2");
    failed += c_compiler_test_expr("<c_compile_expr_lt>", "1 < 2");
    failed += c_compiler_test_expr("<c_compile_expr_le>", "1 <= 2");
    failed += c_compiler_test_expr("<c_compile_expr_gt>", "1 > 2");
    failed += c_compiler_test_expr("<c_compile_expr_ge>", "1 >= 2");

    // Prefix operators (those not requiring an lvalue or pointer).
    failed += c_compiler_test_expr("<c_compile_expr_pos>", "+1");
    failed += c_compiler_test_expr("<c_compile_expr_neg>", "-1");
    failed += c_compiler_test_expr("<c_compile_expr_bnot>", "~1");
    failed += c_compiler_test_expr("<c_compile_expr_lnot>", "!1");

    // Misc expressions.
    failed += c_compiler_test_expr("<c_compile_expr_ternary>", "0 ? 1 : 2");
    failed += c_compiler_test_expr("<c_compile_expr_exprs>", "(1, 2)");
    failed += c_compiler_test_expr("<c_compile_expr_sizeof>", "sizeof 1");
    failed += c_compiler_test_expr("<c_compile_expr_sizeof_type>", "sizeof(char)");
    failed += c_compiler_test_expr("<c_compile_expr_alignof>", "alignof 1");
    failed += c_compiler_test_expr("<c_compile_expr_alignof_type>", "alignof(char)");

    return failed;
}
