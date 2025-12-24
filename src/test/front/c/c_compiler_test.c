
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_compiler.h"
#include "c_parser.h"
#include "c_std.h"
#include "c_types.h"
#include "ir.h"
#include "ir_optimizer.h"
#include "ir_serialization.h"
#include "ir_types.h"
#include "list.h"
#include "map.h"
#include "testcase.h"

#include <string.h>



static char *test_c_compile_type() {
    char const   source[] = "void (*foobarb)();";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_compile_type>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = c_tkn_create(src, C_STD_def);
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    c_compiler_t *cc = c_compiler_create(
        cctx,
        (c_options_t){
            .c_std          = C_STD_def,
            .char_is_signed = true,
            .short16        = true,
            .int32          = true,
            .long64         = true,
            .size_type      = C_PRIM_ULONG,
        }
    );

    token_t decl = c_parse_decls(&pctx, false);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_tkn_debug_print(decl);
        return TEST_FAIL;
    }

    rc_t inner = c_compile_spec_qual_list(cc, &decl.params[0], &cc->global_scope);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        if (inner) {
            c_type_explain(inner->data, stdout);
            printf("\n");
        }
        return TEST_FAIL;
    }

    rc_t full = c_compile_decl(cc, &decl.params[1], &cc->global_scope, inner, NULL);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        if (inner) {
            c_type_explain(full->data, stdout);
            printf("\n");
        }
        return TEST_FAIL;
    }

    tkn_delete(decl);
    rc_delete(full);
    c_compiler_destroy(cc);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_compile_type)


static char *test_c_compile_expr() {
    char const   source[] = "1 + 2 * 3";
    cctx_t      *cctx     = cctx_create();
    srcfile_t   *src      = srcfile_create(cctx, "<c_compile_expr>", source, sizeof(source) - 1);
    tokenizer_t *tctx     = c_tkn_create(src, C_STD_def);
    c_parser_t   pctx     = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};

    c_compiler_t *cc = c_compiler_create(
        cctx,
        (c_options_t){
            .c_std          = C_STD_def,
            .char_is_signed = true,
            .short16        = true,
            .int32          = true,
            .long64         = true,
            .size_type      = C_PRIM_ULONG,
        }
    );
    c_prepass_t dummy_prepass = {
        .pointer_taken = PTR_SET_EMPTY,
    };

    token_t expr_tok = c_parse_expr(&pctx, false);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_tkn_debug_print(expr_tok);
        return TEST_FAIL;
    }

    ir_func_t       *func = ir_func_create("c_compile_expr", NULL, 0);
    c_compile_expr_t expr = c_compile_expr(cc, &dummy_prepass, (ir_code_t *)func->code_list.head, NULL, &expr_tok);
    c_value_destroy(expr.res);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        ir_func_serialize(func, NULL, stdout);
        return TEST_FAIL;
    }


    ir_func_delete(func);
    tkn_delete(expr_tok);

    c_compiler_destroy(cc);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_compile_expr)


static char *test_c_compile_func() {
    // clang-format off
    char const source[] =
    "int foobar(int);\n"
    "void functest() {\n"
    "    int a;\n"
    "    a = 3;\n"
    "    if (a != 0) {\n"
    "        a = 1;\n"
    "    } else {\n"
    "        a = 2;\n"
    "    }\n"
    "    return a;\n"
    "}\n"
    ;
    // clang-format on
    cctx_t       *cctx = cctx_create();
    srcfile_t    *src  = srcfile_create(cctx, "<c_compile_func>", source, sizeof(source) - 1);
    tokenizer_t  *tctx = c_tkn_create(src, C_STD_def);
    c_parser_t    pctx = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};
    c_compiler_t *cc   = c_compiler_create(
        cctx,
        (c_options_t){
              .c_std          = C_STD_def,
              .char_is_signed = true,
              .short16        = true,
              .int32          = true,
              .long64         = true,
              .size_type      = C_PRIM_ULONG,
        }
    );

    token_t foobar_tok = c_parse_decls(&pctx, true);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_tkn_debug_print(foobar_tok);
        return TEST_FAIL;
    }

    token_t functest_tok = c_parse_decls(&pctx, true);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_tkn_debug_print(functest_tok);
        return TEST_FAIL;
    }

    c_prepass_t prepass = c_precompile_pass(&functest_tok);
    ir_func_t  *func    = c_compile_func_def(cc, &functest_tok, &prepass);
    c_prepass_destroy(prepass);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        ir_func_serialize(func, NULL, stdout);
        return TEST_FAIL;
    }

    ir_func_delete(func);
    tkn_delete(functest_tok);
    tkn_delete(foobar_tok);

    c_compiler_destroy(cc);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_compile_func)


static char *test_c_compile_enum() {
    // clang-format off
    char const source[] =
    "enum a {\n"
    "    a_0,\n"
    "    a_1 = 9,\n"
    "};\n"
    "enum a enumtest() {\n"
    "    return a_1;\n"
    "}\n"
    ;
    // clang-format on
    cctx_t       *cctx = cctx_create();
    srcfile_t    *src  = srcfile_create(cctx, "<test_c_compile_enum>", source, sizeof(source) - 1);
    tokenizer_t  *tctx = c_tkn_create(src, C_STD_def);
    c_parser_t    pctx = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};
    c_compiler_t *cc   = c_compiler_create(
        cctx,
        (c_options_t){
              .c_std          = C_STD_def,
              .char_is_signed = true,
              .short16        = true,
              .int32          = true,
              .long64         = true,
              .size_type      = C_PRIM_ULONG,
        }
    );

    // Parse the enum.
    token_t enum_tok = c_parse_decls(&pctx, true);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_tkn_debug_print(enum_tok);
        return TEST_FAIL;
    }

    // Parse the function.
    token_t func_tok = c_parse_decls(&pctx, true);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_tkn_debug_print(func_tok);
        return TEST_FAIL;
    }

    // Compile both enum and function.
    c_compile_decls(cc, NULL, NULL, &cc->global_scope, &enum_tok);
    c_prepass_t prepass = c_precompile_pass(&func_tok);
    ir_func_t  *func    = c_compile_func_def(cc, &func_tok, &prepass);
    c_prepass_destroy(prepass);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        ir_func_serialize(func, NULL, stdout);
        return TEST_FAIL;
    }

    // Assert that the function is a simple one that literally returns s32'9
    ir_func_to_ssa(func);
    ir_optimize(func);
    EXPECT_INT(func->code_list.len, 1);
    ir_code_t const *code = func->entry;
    EXPECT_INT(code->insns.len, 1);
    ir_insn_t const *insn = container_of(code->insns.head, ir_insn_t, node);
    EXPECT_INT(insn->type, IR_INSN_RETURN);
    EXPECT_INT(insn->operands[0].type, IR_OPERAND_TYPE_CONST);
    EXPECT_INT(insn->operands[0].iconst.prim_type, IR_PRIM_s32);
    EXPECT_INT(insn->operands[0].iconst.constl, 9);

    ir_func_delete(func);
    tkn_delete(func_tok);
    tkn_delete(enum_tok);

    c_compiler_destroy(cc);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_compile_enum)


static char *test_c_compile_struct() {
    // clang-format off
    char const source[] =
    "struct a {\n"
    "    int x, *y;\n"
    "    char;\n"
    "    union {\n"
    "        long z;\n"
    "        int u;\n"
    "    };\n"
    "};\n"
    ;
    // clang-format on
    cctx_t       *cctx = cctx_create();
    srcfile_t    *src  = srcfile_create(cctx, "<test_c_compile_enum>", source, sizeof(source) - 1);
    tokenizer_t  *tctx = c_tkn_create(src, C_STD_def);
    c_parser_t    pctx = {.tkn_ctx = tctx, .type_names = STR_SET_EMPTY};
    c_compiler_t *cc   = c_compiler_create(
        cctx,
        (c_options_t){
              .c_std          = C_STD_def,
              .char_is_signed = true,
              .short16        = true,
              .int32          = true,
              .long64         = true,
              .size_type      = C_PRIM_ULONG,
        }
    );

    // Parse the struct.
    token_t struct_tok = c_parse_decls(&pctx, true);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        c_tkn_debug_print(struct_tok);
        return TEST_FAIL;
    }

    // Compile the struct declaration.
    c_compile_decls(cc, NULL, NULL, &cc->global_scope, &struct_tok);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        return TEST_FAIL;
    }

    // Check the struct fields.
    rc_t comp_rc = map_get(&cc->global_scope.comp_types, "a");
    RETURN_ON_FALSE(comp_rc);
    c_comp_t const *comp = comp_rc->data;
    EXPECT_INT(comp->type, C_COMP_TYPE_STRUCT);
    EXPECT_INT(comp->fields.len, 3);
    RETURN_ON_FALSE(comp->fields.arr[0].name);
    EXPECT_STR(comp->fields.arr[0].name, "x");
    RETURN_ON_FALSE(comp->fields.arr[1].name);
    EXPECT_STR(comp->fields.arr[1].name, "y");
    RETURN_ON_FALSE(comp->fields.arr[2].name == NULL);
    c_type_t const *inner_type = comp->fields.arr[2].type_rc->data;
    c_comp_t const *inner_comp = inner_type->comp->data;
    EXPECT_INT(inner_comp->fields.len, 2);
    RETURN_ON_FALSE(inner_comp->fields.arr[0].name);
    EXPECT_STR(inner_comp->fields.arr[0].name, "z");
    RETURN_ON_FALSE(inner_comp->fields.arr[1].name);
    EXPECT_STR(inner_comp->fields.arr[1].name, "u");

    // Check struct layout.
    EXPECT_INT(comp->size, 24);
    EXPECT_INT(comp->align, 8);
    EXPECT_INT(comp->fields.arr[0].offset, 0);
    EXPECT_INT(comp->fields.arr[1].offset, 8);
    EXPECT_INT(comp->fields.arr[2].offset, 16);
    EXPECT_INT(inner_comp->fields.arr[0].offset, 0);
    EXPECT_INT(inner_comp->fields.arr[1].offset, 0);

    tkn_delete(struct_tok);

    c_compiler_destroy(cc);
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);
    return TEST_OK;
}
LILY_TEST_CASE(test_c_compile_struct)
