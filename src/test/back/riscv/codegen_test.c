
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

    // add %tmp1, %a, s32'433
    ir_var_t *tmp1 = ir_var_create(func, IR_PRIM_s32, "tmp1");
    ir_add_expr2(
        IR_APPEND(func->entry),
        tmp1,
        IR_OP2_add,
        (ir_operand_t){.is_const = false, .var = func->args[0].var},
        (ir_operand_t){.is_const = true, .iconst = (ir_const_t){.prim_type = IR_PRIM_s32, .constl = 8000}}
    );

    // add %tmp2, %tmp1, s32'15
    ir_var_t *tmp2 = ir_var_create(func, IR_PRIM_s32, "tmp2");
    ir_add_expr2(
        IR_APPEND(func->entry),
        tmp2,
        IR_OP2_add,
        (ir_operand_t){.is_const = false, .var = tmp1},
        (ir_operand_t){.is_const = true, .iconst = (ir_const_t){.prim_type = IR_PRIM_s32, .constl = 15}}
    );

    // load %tmp3, %tmp2
    ir_var_t *tmp3 = ir_var_create(func, IR_PRIM_s8, "tmp3");
    ir_add_load(IR_APPEND(func->entry), tmp3, (ir_operand_t){.is_const = false, .var = tmp2});

    backend_profile_t *profile = rv_create_profile();
    profile->backend->init_codegen(profile);
    codegen(profile, func);
    putchar('\n');
    ir_func_serialize(func, stdout);

    return TEST_OK;
}
LILY_TEST_CASE(test_rv_isel)
