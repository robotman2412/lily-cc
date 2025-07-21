
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir.h"
#include "ir/ir_optimizer.h"
#include "ir_serialization.h"
#include "ir_types.h"
#include "testcase.h"


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

    printf("\nOriginal:\n");
    ir_func_serialize(func, stdout);

    ir_func_to_ssa(func);
    printf("\n\nSSA form:\n");
    ir_func_serialize(func, stdout);

    ir_optimize(func);
    printf("\n\nOptimized:\n");
    ir_func_serialize(func, stdout);

    ir_func_delete(func);
    return TEST_OK;
}
LILY_TEST_CASE(test_ir_to_ssa)
