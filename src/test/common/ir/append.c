
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir.h"
#include "ir_types.h"
#include "testcase.h"



int main() {
    ir_func_t *func              = ir_func_create("myfunc", "entry", 1);
    func->args[0].arg_type       = IR_ARG_TYPE_VAR;
    func->args[0].var            = ir_var_create(func, IR_PRIM_f32, "myparam");
    func->args[0].var->arg_index = 0;
    ir_code_t *cur               = func->entry;
    ir_var_t  *var1              = ir_var_create(func, IR_PRIM_bool, "var1");

    ir_add_expr1(
        IR_APPEND(cur),
        IR_RETVAL_VAR(var1),
        IR_OP1_snez,
        (ir_operand_t){.type = IR_OPERAND_TYPE_VAR, .var = func->args[0].var}
    );

    ir_func_delete(func);
    TEST_OK();
}
