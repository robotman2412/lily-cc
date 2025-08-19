
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT


#include "backend.h"
#include "codegen.h"
#include "ir.h"
#include "ir_serialization.h"
#include "ir_types.h"
#include "rv_backend.h"
#include "testcase.h"



char *test_rv_isel() {
    ir_func_t *func   = ir_func_create("test_isel", "code0", 1, (char const *const[]){"a"});
    func->enforce_ssa = true;

    // add %tmp1, %a, s32'0xf00
    ir_var_t *tmp1 = ir_var_create(func, IR_PRIM_s32, "tmp1");
    ir_add_expr2(
        IR_APPEND(func->entry),
        tmp1,
        IR_OP2_add,
        (ir_operand_t){.type = IR_OPERAND_TYPE_VAR, .var = func->args[0].var},
        (ir_operand_t){.type = IR_OPERAND_TYPE_CONST, .iconst = (ir_const_t){.prim_type = IR_PRIM_s32, .constl = 0xf00}}
    );

    // add %tmp2, %tmp1, s32'0xcafebabe
    ir_var_t *tmp2 = ir_var_create(func, IR_PRIM_s32, "tmp2");
    ir_add_expr2(
        IR_APPEND(func->entry),
        tmp2,
        IR_OP2_add,
        (ir_operand_t){.type = IR_OPERAND_TYPE_VAR, .var = tmp1},
        (ir_operand_t){.type   = IR_OPERAND_TYPE_CONST,
                       .iconst = (ir_const_t){.prim_type = IR_PRIM_s32, .constl = 0xcafebabe}}
    );

    // sgt %tmp4, s32'0x1000, %tmp2
    ir_var_t *tmp4 = ir_var_create(func, IR_PRIM_bool, "tmp4");
    ir_add_expr2(
        IR_APPEND(func->entry),
        tmp4,
        IR_OP2_slt,
        IR_OPERAND_CONST(IR_CONST_S32(0x1000)),
        IR_OPERAND_VAR(tmp2)
    );

    // branch (%code1), %tmp4
    ir_code_t *code1 = ir_code_create(func, "code1");
    ir_add_branch(IR_APPEND(func->entry), IR_OPERAND_VAR(tmp4), code1);

    // jump (%code2)
    ir_code_t *code2 = ir_code_create(func, "code2");
    ir_add_jump(IR_APPEND(func->entry), code2);

    // load %tmp3, %tmp2
    ir_var_t *tmp3 = ir_var_create(func, IR_PRIM_s8, "tmp3");
    ir_add_load(IR_APPEND(code2), tmp3, (ir_operand_t){.type = IR_OPERAND_TYPE_VAR, .var = tmp2});

    backend_profile_t *profile = rv_create_profile();
    profile->backend->init_codegen(profile);
    codegen(profile, func);
    putchar('\n');
    ir_func_serialize(func, stdout);

    ir_func_delete(func);
    profile->backend->delete_profile(profile);

    return TEST_OK;
}
LILY_TEST_CASE(test_rv_isel)
