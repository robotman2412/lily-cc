
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir.h"
#include "testcase.h"


static char *ir_append() {
    char const *const arg_names[] = {
        "myparam",
    };
    ir_func_t *func = ir_func_create("myfunc", "entry", 1, arg_names);
    ir_code_t *cur  = func->entry;
    ir_var_t  *var1 = ir_var_create(func, IR_PRIM_BOOL, "var1");

    ir_add_expr1(cur, var1, IR_OP1_SNEZ, (ir_operand_t){.is_const = false, .var = func->args[0]});

    return TEST_OK;
}
LILY_TEST_CASE(ir_append)
