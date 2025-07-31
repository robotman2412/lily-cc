
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "compiler.h"
#include "ir.h"
#include "ir/ir_optimizer.h"
#include "ir_serialization.h"
#include "ir_tokenizer.h"
#include "ir_types.h"
#include "testcase.h"
#include "tokenizer.h"

#include <stdio.h>



static char *test_ir_tokenize() {
    // clang-format off
    char const src[] =
    "function var\n"
    "s32'0xcafebabe\n"
    "// Line comment things\n"
    "/*\n"
    "Block comment things\n"
    "*/\n"
    "%localname\n"
    "varnota_keyword\n"
    "<globalname>\n"
    ",\n"
    "(\n"
    ")\n"
    "bool'?\n"
    ;
    // clang-format on

    // Create tokenizer context.
    cctx_t      *cctx    = cctx_create();
    srcfile_t   *srcfile = srcfile_create(cctx, "<test_ir_deserialize>.lily_ir", src, sizeof(src) - 1);
    tokenizer_t *tctx    = ir_tkn_create(srcfile);
    token_t      tkn;

#define EXPECT_EOL_TKN()                                                                                               \
    {                                                                                                                  \
        tkn = tkn_next(tctx);                                                                                          \
        EXPECT_INT(tkn.type, TOKENTYPE_EOL);                                                                           \
        tkn_delete(tkn);                                                                                               \
        while (tkn_peek(tctx).type == TOKENTYPE_EOL) {                                                                 \
            tkn_delete(tkn_next(tctx));                                                                                \
        }                                                                                                              \
    }


    tkn = tkn_next(tctx); // function
    EXPECT_INT(tkn.pos.line, 0);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.type, TOKENTYPE_KEYWORD);
    EXPECT_INT(tkn.pos.len, 8);
    EXPECT_INT(tkn.subtype, IR_KEYW_function);
    tkn_delete(tkn);

    tkn = tkn_next(tctx); // var
    EXPECT_INT(tkn.pos.line, 0);
    EXPECT_INT(tkn.pos.col, 9);
    EXPECT_INT(tkn.type, TOKENTYPE_KEYWORD);
    EXPECT_INT(tkn.pos.len, 3);
    EXPECT_INT(tkn.subtype, IR_KEYW_var);
    tkn_delete(tkn);
    EXPECT_EOL_TKN();

    tkn = tkn_next(tctx); // s32'0xcafebabe
    EXPECT_INT(tkn.pos.line, 1);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.type, TOKENTYPE_ICONST);
    EXPECT_INT(tkn.pos.len, 14);
    EXPECT_INT(tkn.subtype, IR_PRIM_s32);
    EXPECT_INT(tkn.ival, (int)0xcafebabe);
    tkn_delete(tkn);
    EXPECT_EOL_TKN();

    tkn = tkn_next(tctx); // %localname
    EXPECT_INT(tkn.pos.line, 6);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.type, TOKENTYPE_IDENT);
    EXPECT_INT(tkn.pos.len, 10);
    EXPECT_INT(tkn.subtype, IR_IDENT_LOCAL);
    EXPECT_STR_L(tkn.strval, tkn.strval_len, "localname", 9);
    tkn_delete(tkn);
    EXPECT_EOL_TKN();

    tkn = tkn_next(tctx); // varnota_keyword
    EXPECT_INT(tkn.pos.line, 7);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.type, TOKENTYPE_IDENT);
    EXPECT_INT(tkn.pos.len, 15);
    EXPECT_INT(tkn.subtype, IR_IDENT_BARE);
    EXPECT_STR_L(tkn.strval, tkn.strval_len, "varnota_keyword", 15);
    tkn_delete(tkn);
    EXPECT_EOL_TKN();

    tkn = tkn_next(tctx); // <globalname>
    EXPECT_INT(tkn.pos.line, 8);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.type, TOKENTYPE_IDENT);
    EXPECT_INT(tkn.pos.len, 12);
    EXPECT_INT(tkn.subtype, IR_IDENT_GLOBAL);
    EXPECT_STR_L(tkn.strval, tkn.strval_len, "globalname", 10);
    tkn_delete(tkn);
    EXPECT_EOL_TKN();

    tkn = tkn_next(tctx); // ,
    EXPECT_INT(tkn.pos.line, 9);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.type, TOKENTYPE_OTHER);
    EXPECT_INT(tkn.pos.len, 1);
    EXPECT_INT(tkn.subtype, IR_TKN_COMMA);
    tkn_delete(tkn);
    EXPECT_EOL_TKN();

    tkn = tkn_next(tctx); // (
    EXPECT_INT(tkn.pos.line, 10);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.type, TOKENTYPE_OTHER);
    EXPECT_INT(tkn.pos.len, 1);
    EXPECT_INT(tkn.subtype, IR_TKN_LPAR);
    tkn_delete(tkn);
    EXPECT_EOL_TKN();

    tkn = tkn_next(tctx); // )
    EXPECT_INT(tkn.pos.line, 11);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.type, TOKENTYPE_OTHER);
    EXPECT_INT(tkn.pos.len, 1);
    EXPECT_INT(tkn.subtype, IR_TKN_RPAR);
    tkn_delete(tkn);
    EXPECT_EOL_TKN();

    tkn = tkn_next(tctx); // bool'?
    EXPECT_INT(tkn.pos.line, 12);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.type, TOKENTYPE_OTHER);
    EXPECT_INT(tkn.pos.len, 6);
    EXPECT_INT(tkn.subtype, IR_TKN_UNDEF);
    EXPECT_INT(tkn.ival, IR_PRIM_bool);
    tkn_delete(tkn);
    EXPECT_EOL_TKN();


    // Clean up.
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);

    return TEST_OK;
}
LILY_TEST_CASE(test_ir_tokenize)



static char *test_ir_deserialize() {
    // clang-format off
    char const src[] =
    "function <test_ir_deserialize>\n"
    "    var %a s32\n"
    "    var %b s32\n"
    "    var %c bool\n"
    "code %code0\n"
    "    %b = add %a, s32'-3\n"
    "    %c = seqz %b\n"
    // "    branch %c, (%code1)\n"
    "    return %b\n"
    "code %code1\n"
    "    return s32'1\n"
    "entry %code0\n"
    ;
    // clang-format on

    // Create tokenizer context.
    cctx_t      *cctx    = cctx_create();
    srcfile_t   *srcfile = srcfile_create(cctx, "<test_ir_deserialize>.lily_ir", src, sizeof(src) - 1);
    tokenizer_t *tctx    = ir_tkn_create(srcfile);

    // Try to deserialize this function.
    ir_func_t *func = ir_func_deserialize(tctx);
    if (func) {
        printf("\n");
        ir_func_serialize(func, stdout);
        ir_func_delete(func);
    }

    // Report errors.
    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag);
            diag = (diagnostic_t const *)diag->node.next;
        }
        tkn_ctx_delete(tctx);
        cctx_delete(cctx);
        return TEST_FAIL;
    }

    // Clean up.
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);

    return TEST_OK;
}
LILY_TEST_CASE(test_ir_deserialize)



static char *test_ir_append() {
    char const *const arg_names[] = {
        "myparam",
    };
    ir_func_t *func = ir_func_create("myfunc", "entry", 1, arg_names);
    ir_code_t *cur  = func->entry;
    ir_var_t  *var1 = ir_var_create(func, IR_PRIM_bool, "var1");

    ir_add_expr1(
        IR_APPEND(cur),
        var1,
        IR_OP1_snez,
        (ir_operand_t){.type = IR_OPERAND_TYPE_VAR, .var = func->args[0].var}
    );

    ir_func_delete(func);
    return TEST_OK;
}
LILY_TEST_CASE(test_ir_append)



static char *test_ir_to_ssa() {
    ir_func_t *func = ir_func_create("ir_to_ssa", NULL, 0, NULL);

    ir_var_t *var0 = ir_var_create(func, IR_PRIM_s32, NULL);
    ir_var_t *var1 = ir_var_create(func, IR_PRIM_s32, NULL);
    ir_var_t *var2 = ir_var_create(func, IR_PRIM_bool, NULL);

    ir_code_t *code0 = container_of(func->code_list.head, ir_code_t, node);
    ir_code_t *code1 = ir_code_create(func, NULL);
    ir_code_t *code2 = ir_code_create(func, NULL);
    ir_code_t *code3 = ir_code_create(func, NULL);

    ir_add_expr1(
        IR_APPEND(code0),
        var0,
        IR_OP1_mov,
        (ir_operand_t){.type = IR_OPERAND_TYPE_CONST, .iconst = {.prim_type = IR_PRIM_s32, .constl = 0xdeadbeef}}
    );
    ir_add_expr1(
        IR_APPEND(code0),
        var1,
        IR_OP1_mov,
        (ir_operand_t){.type = IR_OPERAND_TYPE_CONST, .iconst = {.prim_type = IR_PRIM_s32, .constl = 0}}
    );
    ir_add_jump(IR_APPEND(code0), code1);

    ir_add_expr1(IR_APPEND(code1), var2, IR_OP1_snez, (ir_operand_t){.type = IR_OPERAND_TYPE_VAR, .var = var1});
    ir_add_branch(IR_APPEND(code1), (ir_operand_t){.type = IR_OPERAND_TYPE_VAR, .var = var2}, code2);
    ir_add_jump(IR_APPEND(code1), code3);

    ir_add_expr2(
        IR_APPEND(code2),
        var0,
        IR_OP2_shr,
        (ir_operand_t){.type = IR_OPERAND_TYPE_VAR, .var = var0},
        (ir_operand_t){.type = IR_OPERAND_TYPE_CONST, .iconst = {.prim_type = IR_PRIM_s32, .constl = 3}}
    );
    ir_add_jump(IR_APPEND(code2), code1);

    ir_add_return1(IR_APPEND(code3), (ir_operand_t){.type = IR_OPERAND_TYPE_VAR, .var = var0});

    ir_func_to_ssa(func);
    ir_optimize(func);

    ir_func_delete(func);
    return TEST_OK;
}
LILY_TEST_CASE(test_ir_to_ssa)
