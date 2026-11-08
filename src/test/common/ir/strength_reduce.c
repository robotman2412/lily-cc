
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir.h"
#include "ir_interpreter.h"
#include "ir_optimizer.h"
#include "optimizer_test.h"
#include "testcase.h"

#include <stdint.h>



static uint64_t width_mask(ir_prim_t type) {
    return ir_trim_const((ir_const_t){.prim_type = type, .constl = UINT64_MAX}, false).constl;
}

static int64_t signed_value(uint64_t value, ir_prim_t type) {
    return (int64_t)ir_trim_const((ir_const_t){.prim_type = type, .constl = value}, true).constl;
}

// Deliberately find definitions in the instruction stream, not assigned_at.
static ir_insn_t *find_definition(ir_func_t *func, ir_var_t *var) {
    ir_insn_t *found = NULL;
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        dlist_foreach_node(ir_insn_t, insn, &code->insns) {
            for (size_t i = 0; i < insn->returns_len; i++) {
                if (insn->returns[i].type == IR_RETVAL_TYPE_VAR && insn->returns[i].dest_var == var) {
                    if (found) {
                        return NULL;
                    }
                    found = insn;
                }
            }
        }
    }
    return found;
}

// Evaluate pure integer DAGs using the same arithmetic helpers as constant folding.
static int
    evaluate(ir_func_t *func, ir_operand_t operand, ir_const_t const inputs[], unsigned int depth, ir_const_t *result) {
    TEST_ASSERT(depth < 128u, "%s: cyclic expression", func->name);
    if (operand.type == IR_OPERAND_TYPE_CONST) {
        TEST_ASSERT(ir_prim_is_integer_or_bool(operand.iconst.prim_type), "%s", func->name);
        *result = ir_trim_const(operand.iconst, false);
        TEST_OK();
    }
    TEST_EXPECT_INT(operand.type, IR_OPERAND_TYPE_VAR, "%s", func->name);
    ir_var_t *var = operand.var;
    TEST_ASSERT(ir_prim_is_integer_or_bool(var->prim_type), "%s", func->name);
    if (var->arg_index >= 0) {
        TEST_COMPARE_UINT((size_t)var->arg_index, <, func->args_len, "%s", func->name);
        TEST_ASSERT(func->args[var->arg_index].var == var, "%s", func->name);
        *result = ir_trim_const(inputs[var->arg_index], false);
        TEST_OK();
    }
    ir_insn_t *insn = find_definition(func, var);
    TEST_ASSERT(insn, "%s: missing or ambiguous definition", func->name);
    TEST_ASSERT(insn->type == IR_INSN_EXPR1 || insn->type == IR_INSN_EXPR2, "%s", func->name);
    TEST_EXPECT_UINT(insn->returns_len, 1u, "%s", func->name);
    TEST_EXPECT_UINT(insn->operands_len, insn->type == IR_INSN_EXPR1 ? 1u : 2u, "%s", func->name);
    ir_const_t lhs, rhs;
    TEST_EXPECT_INT(evaluate(func, insn->operands[0], inputs, depth + 1, &lhs), 0, "%s", func->name);
    if (insn->type == IR_INSN_EXPR2) {
        TEST_EXPECT_INT(evaluate(func, insn->operands[1], inputs, depth + 1, &rhs), 0, "%s", func->name);
        *result = ir_calc2(insn->op2, lhs, rhs);
    } else if (insn->op1 == IR_OP1_mov) {
        *result = lhs;
    } else if (insn->op1 == IR_OP1_signext || insn->op1 == IR_OP1_zeroext) {
        *result = ir_cast(var->prim_type, lhs, insn->op1 == IR_OP1_signext);
    } else {
        *result = ir_calc1(insn->op1, lhs);
    }
    result->prim_type = var->prim_type;
    *result           = ir_trim_const(*result, false);
    TEST_OK();
}

static ir_var_t *add_argument(ir_func_t *func, size_t index, ir_prim_t type) {
    ir_var_t *var              = ir_var_create(func, type, NULL);
    var->arg_index             = index;
    func->args[index].arg_type = IR_FUNCARG_TYPE_VAR;
    func->args[index].var      = var;
    return var;
}

// Check every signed power-of-two divisor, including the high half of i128.
static int test_i128(void) {
    for (unsigned int bit = 0; bit < 128; bit++) {
        unsigned __int128 power = (unsigned __int128)1 << bit;
        unsigned __int128 samples[]
            = {0,
               1,
               power - 1,
               power,
               power + 1,
               -power - 1,
               -power,
               -power + 1,
               (unsigned __int128)1 << 127,
               ((unsigned __int128)1 << 127) - 1,
               -1};
        for (unsigned int negative = 0; negative < 2; negative++) {
            unsigned __int128 divisor      = negative ? -power : power;
            ir_const_t        constant     = {.prim_type = IR_PRIM_i128, .constl = divisor, .consth = divisor >> 64};
            ir_op2_type_t     operations[] = {IR_OP2_divs, IR_OP2_rems};
            for (size_t op = 0; op < sizeof(operations) / sizeof(*operations); op++) {
                ir_func_t *func  = ir_func_create("signed_i128_power", NULL, 1);
                ir_var_t  *input = add_argument(func, 0, IR_PRIM_i128);
                ir_var_t  *dest  = ir_var_create(func, IR_PRIM_i128, NULL);
                ir_insn_t *expr  = ir_add_expr2(
                    IR_APPEND(func->entry),
                    IR_RETVAL_VAR(dest),
                    operations[op],
                    IR_OPERAND_VAR(input),
                    IR_OPERAND_CONST(constant)
                );
                func->enforce_ssa = true;
                TEST_ASSERT(opt_strength_reduce(func));
                TEST_EXPECT_INT(opt_test_validate(func), 0);
                TEST_ASSERT(!opt_strength_reduce(func));
                dlist_foreach_node(ir_insn_t, insn, &func->entry->insns) {
                    if (insn->type == IR_INSN_EXPR2) {
                        TEST_ASSERT(insn->op2 != IR_OP2_divs && insn->op2 != IR_OP2_rems);
                    }
                }
                TEST_ASSERT(expr->op2 == IR_OP2_shrs || expr->op2 == IR_OP2_sub);
                for (size_t i = 0; i < sizeof(samples) / sizeof(*samples); i++) {
                    if (samples[i] == ((unsigned __int128)1 << 127) && divisor == (unsigned __int128)-1) {
                        continue; // Undefined signed overflow.
                    }
                    ir_const_t value    = {.prim_type = IR_PRIM_i128, .constl = samples[i], .consth = samples[i] >> 64};
                    ir_const_t expected = ir_calc2(operations[op], value, constant), actual;
                    TEST_EXPECT_INT(evaluate(func, IR_OPERAND_VAR(dest), &value, 0, &actual), 0);
                    TEST_EXPECT_UINT(actual.constl, expected.constl, "bit %u, negative %u", bit, negative);
                    TEST_EXPECT_UINT(actual.consth, expected.consth, "bit %u, negative %u", bit, negative);
                }
                ir_func_delete(func);
            }
        }
    }
    TEST_OK();
}

static int check_sample(
    ir_func_t *func, ir_operand_t output, ir_op2_type_t operation, uint64_t constant, bool constant_left, uint64_t input
) {
    ir_prim_t  type     = ir_operand_prim(output);
    ir_const_t inputs[] = {{.prim_type = type, .constl = input}, {.prim_type = type, .constl = constant}};
    ir_const_t lhs = inputs[constant_left], rhs = inputs[!constant_left];
    ir_const_t expected, actual;
    // Signed division truncates toward zero; signed remainder follows C's %.
    // Exclude the only overflowing quotient, including for narrow IR types.
    if (operation == IR_OP2_divs || operation == IR_OP2_rems) {
        int64_t a = signed_value(lhs.constl, type), b = signed_value(rhs.constl, type);
        int64_t minimum = signed_value(UINT64_C(1) << (ir_prim_bits(type) - 1), type);
        TEST_ASSERT(b != 0, "%s: zero divisor", func->name);
        if (a == minimum && b == -1) {
            TEST_OK(); // C leaves both minimum / -1 and minimum % -1 undefined.
        }
    }
    expected = ir_calc2(operation, lhs, rhs);
    TEST_EXPECT_INT(evaluate(func, output, inputs, 0, &actual), 0, "%s", func->name);
    TEST_EXPECT_UINT(
        actual.constl,
        ir_trim_const(expected, false).constl,
        "%s: input %llu, constant %llu",
        func->name,
        (unsigned long long)input,
        (unsigned long long)constant
    );
    TEST_OK();
}

static int test_integer(
    char const   *label,
    ir_prim_t     type,
    ir_op2_type_t operation,
    uint64_t      constant,
    bool          constant_left,
    bool          require_reduction
) {
    char name[160];
    snprintf(
        name,
        sizeof(name),
        "%s_%s_%s_%lld",
        label,
        ir_prim_name[type],
        ir_op2_names[operation],
        (long long)signed_value(constant, type)
    );
    ir_func_t *func       = ir_func_create(name, NULL, 1);
    ir_var_t  *input      = add_argument(func, 0, type);
    ir_var_t  *dest       = ir_var_create(func, type, "result");
    func->rettype         = (ir_funcret_t){.type = IR_FUNCRET_PRIM, .prim_type = type};
    ir_operand_t variable = IR_OPERAND_VAR(input);
    ir_operand_t number   = IR_OPERAND_CONST(((ir_const_t){.prim_type = type, .constl = constant}));
    ir_add_expr2(
        IR_APPEND(func->entry),
        IR_RETVAL_VAR(dest),
        operation,
        constant_left ? number : variable,
        constant_left ? variable : number
    );
    ir_insn_t *ret    = ir_add_return1(IR_APPEND(func->entry), IR_OPERAND_VAR(dest));
    func->enforce_ssa = true;

    TEST_EXPECT_INT(opt_test_validate(func), 0, "%s", name);
    TEST_EXPECT_UINT(opt_test_defs(func, input), 0u, "%s: input must be runtime", name);
    TEST_EXPECT_UINT(opt_test_reads(func, input), 1u, "%s", name);
    TEST_EXPECT_UINT(opt_test_defs(func, dest), 1u, "%s", name);
    bool signed_operation = operation == IR_OP2_divs || operation == IR_OP2_rems;
    if (require_reduction) {
        ir_const_t magnitude = signed_operation ? ir_const_abs(number.iconst) : number.iconst;
        TEST_EXPECT_INT(ir_const_popcnt(ir_trim_const(magnitude, false)), 1, "%s: power-of-two precondition", name);
    }
    bool changed = opt_strength_reduce(func);
    TEST_EXPECT_INT(opt_test_validate(func), 0, "%s", name);
    TEST_EXPECT_UINT(ret->operands_len, 1u, "%s", name);
    TEST_EXPECT_INT(ir_operand_prim(ret->operands[0]), type, "%s: result type changed", name);
    if (require_reduction) {
        TEST_ASSERT(changed, "%s: expected reduction", name);
        // A reduction must actually remove expensive arithmetic. Equivalent
        // remainder graphs may use a mask, or x - ((x >> k) << k).
        dlist_foreach_node(ir_code_t, code, &func->code_list) {
            dlist_foreach_node(ir_insn_t, insn, &code->insns) {
                if (insn->type == IR_INSN_EXPR2) {
                    TEST_ASSERT(
                        insn->op2 != IR_OP2_mul && insn->op2 != IR_OP2_divu && insn->op2 != IR_OP2_divs
                            && insn->op2 != IR_OP2_remu && insn->op2 != IR_OP2_rems,
                        "%s: expensive arithmetic remains",
                        name
                    );
                }
            }
        }
        if (signed_operation) {
            // Signed reductions share a bias DAG; check its behavior below.
        } else if (ret->operands[0].type == IR_OPERAND_TYPE_VAR && ret->operands[0].var == input) {
            TEST_EXPECT_UINT(constant, 1u, "%s: only an identity can return the input", name);
            TEST_ASSERT(operation == IR_OP2_mul || operation == IR_OP2_divu, "%s", name);
        } else if (ret->operands[0].type == IR_OPERAND_TYPE_CONST) {
            TEST_EXPECT_INT(operation, IR_OP2_remu, "%s", name);
            TEST_EXPECT_UINT(constant, 1u, "%s", name);
            TEST_EXPECT_UINT(ret->operands[0].iconst.constl, 0u, "%s", name);
        } else {
            TEST_EXPECT_INT(ret->operands[0].type, IR_OPERAND_TYPE_VAR, "%s", name);
            ir_insn_t *root = find_definition(func, ret->operands[0].var);
            TEST_ASSERT(root, "%s", name);
            if (root->type == IR_INSN_EXPR1) {
                if (operation != IR_OP2_remu) {
                    // Identity reductions may use mov instead of a zero-distance shift.
                    TEST_EXPECT_UINT(constant, 1u, "%s", name);
                    TEST_EXPECT_INT(root->op1, IR_OP1_mov, "%s", name);
                }
                // Remainder DAGs may end in a resize; the evaluator checks it.
            } else {
                TEST_EXPECT_INT(root->type, IR_INSN_EXPR2, "%s", name);
                TEST_EXPECT_UINT(root->operands_len, 2u, "%s", name);
                if (operation != IR_OP2_remu || root->op2 == IR_OP2_band) {
                    // Direct shift/mask forms have a prescribed shift/mask.
                    ir_op2_type_t expected_operation = operation == IR_OP2_mul    ? IR_OP2_shl
                                                       : operation == IR_OP2_divu ? IR_OP2_shru
                                                                                  : IR_OP2_band;
                    uint64_t      expected_constant
                        = operation == IR_OP2_remu ? constant - 1 : (uint64_t)ir_const_ctz(number.iconst);
                    TEST_EXPECT_INT(root->op2, expected_operation, "%s", name);
                    TEST_EXPECT_INT(root->operands[1].type, IR_OPERAND_TYPE_CONST, "%s", name);
                    TEST_EXPECT_INT(root->operands[1].iconst.prim_type, type, "%s", name);
                    TEST_EXPECT_UINT(root->operands[1].iconst.constl & width_mask(type), expected_constant, "%s", name);
                }
                // Check other cheap remainder forms over the i8 domain and wider edges below.
            }
        }
    }
    TEST_ASSERT(!opt_strength_reduce(func), "%s: reduction must reach a fixed point", name);
    TEST_EXPECT_INT(opt_test_validate(func), 0, "%s", name);
    // Exhaust all i8 bit patterns, including negative signed values.
    // Multiplication avoids overflow; check_sample excludes minimum % -1.
    uint64_t limit = UINT8_MAX;
    if (operation == IR_OP2_mul && limit > width_mask(type) / constant) {
        limit = width_mask(type) / constant;
    }
    for (uint64_t sample = 0; sample <= limit; sample++) {
        TEST_EXPECT_INT(
            check_sample(func, ret->operands[0], operation, constant, constant_left, sample),
            0,
            "%s",
            name
        );
    }
    uint64_t mask    = width_mask(type);
    uint64_t edges[] = {0, 1, constant - 1, constant, constant + 1, mask >> 1, (mask >> 1) + 1, mask};
    for (size_t i = 0; i < sizeof(edges) / sizeof(*edges); i++) {
        if (operation == IR_OP2_mul && edges[i] > mask / constant) {
            continue;
        }
        TEST_EXPECT_INT(
            check_sample(func, ret->operands[0], operation, constant, constant_left, edges[i]),
            0,
            "%s",
            name
        );
    }
    if (operation == IR_OP2_divs || operation == IR_OP2_rems) {
        // Nonmultiples distinguish C remainder from masking or floor division:
        // -7 % 4 == -3, 7 % -4 == 3, and -7 % -4 == -3.
        int64_t negatives[] = {-1, -2, -3, -4, -5, -7, -8, -9, -12, -127};
        for (size_t i = 0; i < sizeof(negatives) / sizeof(*negatives); i++) {
            TEST_EXPECT_INT(
                check_sample(func, ret->operands[0], operation, constant, false, (uint64_t)negatives[i] & mask),
                0,
                "%s",
                name
            );
        }
    }
    ir_func_delete(func);
    TEST_OK();
}

static int test_runtime_divisor(ir_op2_type_t operation) {
    char name[80];
    snprintf(name, sizeof(name), "runtime_divisor_%s", ir_op2_names[operation]);
    ir_func_t *func = ir_func_create(name, NULL, 2);
    ir_var_t  *lhs  = add_argument(func, 0, IR_PRIM_i32);
    ir_var_t  *rhs  = add_argument(func, 1, IR_PRIM_i32);
    ir_var_t  *dest = ir_var_create(func, IR_PRIM_i32, NULL);
    func->rettype   = (ir_funcret_t){.type = IR_FUNCRET_PRIM, .prim_type = IR_PRIM_i32};
    ir_add_expr2(IR_APPEND(func->entry), IR_RETVAL_VAR(dest), operation, IR_OPERAND_VAR(lhs), IR_OPERAND_VAR(rhs));
    ir_insn_t *ret    = ir_add_return1(IR_APPEND(func->entry), IR_OPERAND_VAR(dest));
    func->enforce_ssa = true;
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_EXPECT_UINT(opt_test_defs(func, lhs), 0u);
    TEST_EXPECT_UINT(opt_test_defs(func, rhs), 0u);
    opt_strength_reduce(func);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_EXPECT_INT(ir_operand_prim(ret->operands[0]), IR_PRIM_i32);
    uint64_t samples[][2] = {{0, 1}, {1, 2}, {31, 3}, {255, 4}, {UINT32_MAX, 7}, {UINT32_MAX, UINT32_MAX}};
    for (size_t i = 0; i < sizeof(samples) / sizeof(*samples); i++) {
        TEST_EXPECT_INT(
            check_sample(func, ret->operands[0], operation, samples[i][1], false, samples[i][0]),
            0,
            "%s",
            name
        );
    }
    if (operation == IR_OP2_rems) {
        int64_t signed_samples[][2] = {
            {-7, 4},
            {7, -4},
            {-7, -4},
            {-8, 4},
            {8, -4},
            {-1, 4},
            {1, -4},
            {INT32_MIN, 3},
            {INT32_MIN, -3},
            {INT32_MAX, -4},
            {0, -1},
            {-7, -1},
        };
        for (size_t i = 0; i < sizeof(signed_samples) / sizeof(*signed_samples); i++) {
            TEST_EXPECT_INT(
                check_sample(func, ret->operands[0], operation, signed_samples[i][1], false, signed_samples[i][0]),
                0,
                "%s",
                name
            );
        }
    }
    ir_func_delete(func);
    TEST_OK();
}

static int test_float(ir_prim_t type) {
    ir_func_t *func   = ir_func_create("float_multiply", NULL, 1);
    ir_var_t  *src    = add_argument(func, 0, type);
    ir_var_t  *dest   = ir_var_create(func, type, NULL);
    func->rettype     = (ir_funcret_t){.type = IR_FUNCRET_PRIM, .prim_type = type};
    ir_const_t factor = type == IR_PRIM_f32 ? IR_CONST_F32(8.0f) : IR_CONST_F64(8.0);
    ir_insn_t *mul    = ir_add_expr2(
        IR_APPEND(func->entry),
        IR_RETVAL_VAR(dest),
        IR_OP2_mul,
        IR_OPERAND_VAR(src),
        IR_OPERAND_CONST(factor)
    );
    ir_add_return1(IR_APPEND(func->entry), IR_OPERAND_VAR(dest));
    func->enforce_ssa = true;
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_EXPECT_UINT(opt_test_defs(func, src), 0u);
    opt_strength_reduce(func);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    // Integer shift/mask rewrites are inapplicable to floating-point values.
    TEST_ASSERT(find_definition(func, dest) == mul);
    TEST_EXPECT_INT(mul->type, IR_INSN_EXPR2);
    TEST_EXPECT_INT(mul->op2, IR_OP2_mul);
    TEST_EXPECT_INT(mul->operands[0].type, IR_OPERAND_TYPE_VAR);
    TEST_ASSERT(mul->operands[0].var == src);
    TEST_EXPECT_INT(mul->operands[1].type, IR_OPERAND_TYPE_CONST);
    TEST_EXPECT_INT(mul->operands[1].iconst.prim_type, type);
    TEST_ASSERT(ir_const_identical(mul->operands[1].iconst, factor));
    ir_func_delete(func);
    TEST_OK();
}

int main(void) {
    int       result  = 0;
    ir_prim_t types[] = {IR_PRIM_i8, IR_PRIM_i16, IR_PRIM_i32, IR_PRIM_i64};
    for (size_t i = 0; i < sizeof(types) / sizeof(*types); i++) {
        result |= test_integer("multiply_right", types[i], IR_OP2_mul, 8, false, true);
        result |= test_integer("multiply_left", types[i], IR_OP2_mul, 8, true, true);
        result |= test_integer("divide_power_two", types[i], IR_OP2_divu, 4, false, true);
        result |= test_integer("remainder_power_two", types[i], IR_OP2_remu, 4, false, true);
        result |= test_integer("multiply_one", types[i], IR_OP2_mul, 1, false, true);
        result |= test_integer("divide_one", types[i], IR_OP2_divu, 1, false, true);
        result |= test_integer("remainder_one", types[i], IR_OP2_remu, 1, false, true);
        result |= test_integer("multiply_nonpower", types[i], IR_OP2_mul, 3, false, false);
        result |= test_integer("divide_nonpower", types[i], IR_OP2_divu, 3, false, false);
        result |= test_integer("signed_division", types[i], IR_OP2_divs, 4, false, false);
        result |= test_integer("signed_remainder", types[i], IR_OP2_rems, 4, false, false);
        result |= test_integer("signed_remainder_negative_divisor", types[i], IR_OP2_rems, (uint64_t)-4, false, false);
        result |= test_integer("signed_remainder_nonpower", types[i], IR_OP2_rems, 3, false, false);
        result |= test_integer("signed_remainder_negative_nonpower", types[i], IR_OP2_rems, (uint64_t)-3, false, false);
        result |= test_integer("signed_remainder_one", types[i], IR_OP2_rems, 1, false, false);
        result |= test_integer("signed_remainder_negative_one", types[i], IR_OP2_rems, (uint64_t)-1, false, false);
        for (unsigned int bit = 0; bit < ir_prim_bits(types[i]); bit++) {
            uint64_t      power        = UINT64_C(1) << bit;
            ir_op2_type_t operations[] = {IR_OP2_divs, IR_OP2_rems};
            for (size_t op = 0; op < sizeof(operations) / sizeof(*operations); op++) {
                result |= test_integer("signed_power", types[i], operations[op], power, false, true);
                result |= test_integer(
                    "signed_negative_power",
                    types[i],
                    operations[op],
                    UINT64_C(0) - power,
                    false,
                    true
                );
            }
        }
    }
    result |= test_runtime_divisor(IR_OP2_divu);
    result |= test_runtime_divisor(IR_OP2_remu);
    result |= test_runtime_divisor(IR_OP2_rems);
    result |= test_float(IR_PRIM_f32);
    result |= test_float(IR_PRIM_f64);
    result |= test_i128();
    return result;
}
