
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir.h"
#include "ir_optimizer.h"
#include "lilycc_malloc.h"
#include "optimizer_test.h"
#include "testcase.h"



static ir_func_t *new_func(char const *name, ir_prim_t result, size_t args) {
    ir_func_t *func         = ir_func_create(name, NULL, args);
    func->enforce_ssa       = true;
    func->rettype.type      = IR_FUNCRET_PRIM;
    func->rettype.prim_type = result;
    return func;
}

static ir_var_t *add_arg(ir_func_t *func, size_t index, ir_prim_t type, char const *name) {
    ir_var_t *var              = ir_var_create(func, type, name);
    var->arg_index             = index;
    func->args[index].arg_type = IR_FUNCARG_TYPE_VAR;
    func->args[index].var      = var;
    return var;
}

// Explicit expected constants are the oracle; do not call the compiler evaluator.
static int expect_constant(char const *name, ir_operand_t operand, ir_const_t expected) {
    TEST_EXPECT_INT(operand.type, IR_OPERAND_TYPE_CONST, "%s", name);
    TEST_EXPECT_INT(operand.iconst.prim_type, expected.prim_type, "%s", name);
    if (expected.prim_type == IR_PRIM_f32) {
        TEST_ASSERT(operand.iconst.constf32 == expected.constf32, "%s", name);
    } else if (expected.prim_type == IR_PRIM_f64) {
        TEST_ASSERT(operand.iconst.constf64 == expected.constf64, "%s", name);
    } else {
        unsigned int bits = ir_prim_bits(expected.prim_type);
        uint64_t     mask = bits >= 64 ? UINT64_MAX : (UINT64_C(1) << bits) - 1;
        TEST_EXPECT_UINT(operand.iconst.constl & mask, expected.constl & mask, "%s", name);
        if (bits == 128) {
            TEST_EXPECT_UINT(operand.iconst.consth, expected.consth, "%s", name);
        }
    }
    TEST_OK();
}

static int propagate(ir_func_t *func, bool expect_change) {
    ir_func_recalc_flow(func);
    TEST_EXPECT_INT(opt_test_validate(func), 0, "%s before propagation", func->name);
    bool changed = false;
    // A chain may need several applications of the public single-pass API.
    for (unsigned int i = 0; i < 32; i++) {
        bool step  = opt_const_prop(func);
        changed   |= step;
        TEST_EXPECT_INT(opt_test_validate(func), 0, "%s after propagation", func->name);
        if (!step) {
            TEST_EXPECT_BOOL(changed, expect_change, "%s: unexpected change status", func->name);
            TEST_OK();
        }
    }
    TEST_FAIL("%s: propagation did not converge", func->name);
}

static int test_unary(char const *name, ir_op1_type_t op, ir_const_t input, ir_const_t expected) {
    ir_func_t *func = new_func(name, expected.prim_type, 0);
    ir_var_t  *src  = ir_var_create(func, input.prim_type, "src");
    ir_var_t  *dest = ir_var_create(func, expected.prim_type, "dest");
    ir_add_expr1(IR_APPEND(func->entry), IR_RETVAL_VAR(src), IR_OP1_mov, IR_OPERAND_CONST(input));
    ir_add_expr1(IR_APPEND(func->entry), IR_RETVAL_VAR(dest), op, IR_OPERAND_VAR(src));
    ir_insn_t *ret = ir_add_return1(IR_APPEND(func->entry), IR_OPERAND_VAR(dest));
    TEST_EXPECT_UINT(opt_test_defs(func, src), 1u, "%s", name);
    TEST_EXPECT_UINT(opt_test_defs(func, dest), 1u, "%s", name);
    TEST_EXPECT_UINT(opt_test_reads(func, src), 1u, "%s", name);
    TEST_EXPECT_UINT(opt_test_reads(func, dest), 1u, "%s", name);
    int result = propagate(func, true);
    if (!result) {
        result = expect_constant(name, ret->operands[0], expected);
    }
    ir_func_delete(func);
    return result;
}

static int test_binary(char const *name, ir_op2_type_t op, ir_const_t lhs, ir_const_t rhs, ir_const_t expected) {
    ir_func_t *func = new_func(name, expected.prim_type, 0);
    ir_var_t  *dest = ir_var_create(func, expected.prim_type, "dest");
    TEST_EXPECT_INT(lhs.prim_type, rhs.prim_type, "%s", name);
    ir_add_expr2(IR_APPEND(func->entry), IR_RETVAL_VAR(dest), op, IR_OPERAND_CONST(lhs), IR_OPERAND_CONST(rhs));
    ir_insn_t *ret = ir_add_return1(IR_APPEND(func->entry), IR_OPERAND_VAR(dest));
    TEST_EXPECT_UINT(opt_test_defs(func, dest), 1u, "%s", func->name);
    TEST_EXPECT_UINT(opt_test_reads(func, dest), 1u, "%s", func->name);
    int result = propagate(func, true);
    if (!result) {
        result = expect_constant(name, ret->operands[0], expected);
    }
    ir_func_delete(func);
    return result;
}

static int test_chain_and_consumers(char const *name, uint32_t initial, uint32_t expected) {
    ir_func_t *func  = new_func(name, IR_PRIM_i32, 0);
    ir_var_t  *value = ir_var_create(func, IR_PRIM_i32, "value");
    ir_var_t  *copy  = ir_var_create(func, IR_PRIM_i32, "copy");
    ir_var_t  *sum   = ir_var_create(func, IR_PRIM_i32, "sum");
    ir_add_expr1(IR_APPEND(func->entry), IR_RETVAL_VAR(value), IR_OP1_mov, IR_OPERAND_CONST(IR_CONST_U32(initial)));
    ir_add_expr1(IR_APPEND(func->entry), IR_RETVAL_VAR(copy), IR_OP1_mov, IR_OPERAND_VAR(value));
    ir_add_expr2(IR_APPEND(func->entry), IR_RETVAL_VAR(sum), IR_OP2_add, IR_OPERAND_VAR(copy), IR_OPERAND_VAR(value));
    ir_insn_t *store = ir_add_store(
        IR_APPEND(func->entry),
        IR_OPERAND_VAR(value),
        IR_MEMREF(IR_PRIM_i32, IR_BADDR_SYM("observable"))
    );
    ir_operand_t argument = IR_OPERAND_VAR(value);
    ir_insn_t   *call     = ir_add_call(
        IR_APPEND(func->entry),
        IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM("observe")),
        false,
        (ir_retval_t){0},
        1,
        &argument
    );
    ir_insn_t *ret = ir_add_return1(IR_APPEND(func->entry), IR_OPERAND_VAR(sum));
    TEST_EXPECT_UINT(opt_test_defs(func, value), 1u, "%s", name);
    TEST_EXPECT_UINT(opt_test_reads(func, value), 4u, "%s", name);
    TEST_EXPECT_UINT(opt_test_defs(func, copy), 1u, "%s", name);
    TEST_EXPECT_UINT(opt_test_reads(func, copy), 1u, "%s", name);
    int result = propagate(func, true);
    if (!result) {
        result |= expect_constant(func->name, ret->operands[0], IR_CONST_U32(expected));
        result |= expect_constant(func->name, store->operands[1], IR_CONST_U32(initial));
        result |= expect_constant(func->name, call->operands[1], IR_CONST_U32(initial));
    }
    ir_func_delete(func);
    return result;
}

static int test_runtime_input(char const *name, bool copy, uint32_t addend, char const *arg_name) {
    ir_func_t *func = new_func(name, IR_PRIM_i32, 1);
    ir_var_t  *arg  = add_arg(func, 0, IR_PRIM_i32, arg_name);
    ir_var_t  *dest = ir_var_create(func, IR_PRIM_i32, "dest");
    if (copy) {
        ir_add_expr1(IR_APPEND(func->entry), IR_RETVAL_VAR(dest), IR_OP1_mov, IR_OPERAND_VAR(arg));
    } else {
        ir_add_expr2(
            IR_APPEND(func->entry),
            IR_RETVAL_VAR(dest),
            IR_OP2_add,
            IR_OPERAND_VAR(arg),
            IR_OPERAND_CONST(IR_CONST_U32(addend))
        );
    }
    ir_insn_t *ret = ir_add_return1(IR_APPEND(func->entry), IR_OPERAND_VAR(dest));
    TEST_EXPECT_UINT(opt_test_defs(func, arg), 0u, "%s", name);
    TEST_EXPECT_UINT(opt_test_reads(func, arg), 1u, "%s", name);
    TEST_EXPECT_UINT(opt_test_defs(func, dest), 1u, "%s", func->name);
    int result = propagate(func, copy);
    if (!result) {
        TEST_EXPECT_INT(ret->operands[0].type, IR_OPERAND_TYPE_VAR, "%s", name);
        if (copy) {
            TEST_ASSERT(ret->operands[0].var == arg, "%s", name);
        } else {
            TEST_ASSERT(ret->operands[0].var == dest, "%s", name);
            TEST_EXPECT_UINT(opt_test_reads(func, arg), 1u, "%s", name);
            TEST_EXPECT_UINT(opt_test_defs(func, dest), 1u, "%s", func->name);
        }
    }
    ir_func_delete(func);
    return result;
}

typedef enum {
    PHI_CONSTANTS,
    PHI_ALIAS,
    PHI_DIFFERENT_VARS,
} phi_case_t;

static int test_combinator(
    char const *name, phi_case_t kind, uint32_t lhs, uint32_t rhs, char const *lhs_name, char const *rhs_name
) {
    bool             variables  = kind != PHI_CONSTANTS;
    bool             equal      = kind == PHI_ALIAS || (kind == PHI_CONSTANTS && lhs == rhs);
    size_t           args       = kind == PHI_DIFFERENT_VARS ? 3 : variables ? 2 : 1;
    ir_func_t       *func       = new_func(name, IR_PRIM_i32, args);
    ir_var_t        *cond       = add_arg(func, 0, IR_PRIM_bool, "condition");
    ir_var_t        *left_arg   = variables ? add_arg(func, 1, IR_PRIM_i32, lhs_name) : NULL;
    ir_var_t        *right_arg  = kind == PHI_DIFFERENT_VARS ? add_arg(func, 2, IR_PRIM_i32, rhs_name) : left_arg;
    ir_var_t        *dest       = ir_var_create(func, IR_PRIM_i32, "dest");
    ir_code_t       *left       = ir_code_create(func, "left");
    ir_code_t       *right      = ir_code_create(func, "right");
    ir_code_t       *join       = ir_code_create(func, "join");
    ir_insn_t       *branch     = ir_add_branch(IR_APPEND(func->entry), IR_OPERAND_VAR(cond), left);
    ir_insn_t       *otherwise  = ir_add_jump(IR_APPEND(func->entry), right);
    ir_insn_t       *left_jump  = ir_add_jump(IR_APPEND(left), join);
    ir_insn_t       *right_jump = ir_add_jump(IR_APPEND(right), join);
    ir_combinator_t *bindings   = lilycc_calloc(2, sizeof(*bindings));
    bindings[0]                 = (ir_combinator_t){
        .pred = left,
        .bind = variables ? IR_OPERAND_VAR(left_arg) : IR_OPERAND_CONST(IR_CONST_U32(lhs))
    };
    bindings[1] = (ir_combinator_t){
        .pred = right,
        .bind = variables ? IR_OPERAND_VAR(right_arg) : IR_OPERAND_CONST(IR_CONST_U32(rhs))
    };
    ir_insn_t *phi = ir_add_combinator(IR_APPEND(join), dest, 2, bindings);
    ir_insn_t *ret = ir_add_return1(IR_APPEND(join), IR_OPERAND_VAR(dest));
    TEST_EXPECT_UINT(opt_test_defs(func, cond), 0u, "%s", name);
    TEST_EXPECT_UINT(opt_test_reads(func, cond), 1u, "%s", name);
    TEST_EXPECT_UINT(opt_test_defs(func, dest), 1u, "%s", name);
    TEST_EXPECT_UINT(opt_test_reads(func, dest), 1u, "%s", name);
    if (variables) {
        TEST_EXPECT_UINT(opt_test_defs(func, left_arg), 0u, "%s", name);
        TEST_EXPECT_UINT(opt_test_defs(func, right_arg), 0u, "%s", name);
        TEST_EXPECT_UINT(opt_test_reads(func, left_arg), 1u, "%s", name);
        TEST_EXPECT_UINT(opt_test_reads(func, right_arg), 1u, "%s", name);
        TEST_ASSERT(phi->combinators[0].bind.var == left_arg, "%s", name);
        TEST_ASSERT(phi->combinators[1].bind.var == right_arg, "%s", name);
    } else {
        TEST_EXPECT_INT(expect_constant(name, phi->combinators[0].bind, IR_CONST_U32(lhs)), 0, "%s", name);
        TEST_EXPECT_INT(expect_constant(name, phi->combinators[1].bind, IR_CONST_U32(rhs)), 0, "%s", name);
    }
    // Inspect both live incoming bindings and executable targets without predecessor caches.
    TEST_ASSERT(phi->combinators[0].pred == left && phi->combinators[1].pred == right, "%s", name);
    TEST_EXPECT_INT(branch->operands[0].type, IR_OPERAND_TYPE_MEM, "%s", name);
    TEST_EXPECT_INT(branch->operands[0].mem.base_type, IR_MEMBASE_CODE, "%s", name);
    TEST_ASSERT(branch->operands[0].mem.base_code == left, "%s", name);
    TEST_ASSERT(otherwise->operands[0].mem.base_code == right, "%s", name);
    TEST_ASSERT(left_jump->operands[0].mem.base_code == join, "%s", name);
    TEST_ASSERT(right_jump->operands[0].mem.base_code == join, "%s", name);
    int result = propagate(func, equal);
    if (!result) {
        if (kind == PHI_ALIAS) {
            TEST_EXPECT_INT(ret->operands[0].type, IR_OPERAND_TYPE_VAR, "%s", name);
            TEST_ASSERT(ret->operands[0].var == left_arg, "%s", name);
        } else if (equal) {
            result = expect_constant(name, ret->operands[0], IR_CONST_U32(lhs));
        } else {
            TEST_EXPECT_INT(ret->operands[0].type, IR_OPERAND_TYPE_VAR, "%s", name);
            TEST_ASSERT(ret->operands[0].var == dest, "%s", name);
            TEST_EXPECT_UINT(phi->combinators_len, 2u, "%s", name);
            if (variables) {
                TEST_EXPECT_INT(phi->combinators[0].bind.type, IR_OPERAND_TYPE_VAR, "%s", name);
                TEST_EXPECT_INT(phi->combinators[1].bind.type, IR_OPERAND_TYPE_VAR, "%s", name);
                TEST_ASSERT(phi->combinators[0].bind.var == left_arg, "%s", name);
                TEST_ASSERT(phi->combinators[1].bind.var == right_arg, "%s", name);
            } else {
                result |= expect_constant(name, phi->combinators[0].bind, IR_CONST_U32(lhs));
                result |= expect_constant(name, phi->combinators[1].bind, IR_CONST_U32(rhs));
            }
        }
    }
    ir_func_delete(func);
    return result;
}

// Stringify the original spelling so diagnostics retain initial types and values,
// rather than the optimizer's rewritten operands or expanded constant structures.
#define UNARY_CASE(op, input, expected)                                                                                \
    result |= test_unary(#op "(" #input ") -> " #expected, IR_OP1_##op, (input), (expected))
#define BINARY_CASE(op, lhs, rhs, expected)                                                                            \
    result |= test_binary(#op "(" #lhs ", " #rhs ") -> " #expected, IR_OP2_##op, (lhs), (rhs), (expected))
#define CHAIN_CASE(initial, expected)                                                                                  \
    result |= test_chain_and_consumers("chain(value=" #initial ", copy=%value, sum=%copy+%value)", initial, expected)
#define RUNTIME_CASE(copy, addend, arg)                                                                                \
    result |= test_runtime_input("runtime(copy=" #copy ", src=%" #arg ", addend=" #addend ")", copy, addend, #arg)
#define PHI_CONSTANT_CASE(lhs, rhs)                                                                                    \
    result |= test_combinator("comb(left=" #lhs ", right=" #rhs ")", PHI_CONSTANTS, lhs, rhs, NULL, NULL)
#define PHI_VARIABLE_CASE(kind, lhs, rhs)                                                                              \
    result |= test_combinator(#kind "(left=%" #lhs ", right=%" #rhs ")", kind, 0, 0, #lhs, #rhs)

int main(void) {
    int result = 0;

    // Every unary opcode has at least two distinct expected results.
    UNARY_CASE(mov, IR_CONST_U8(0), IR_CONST_U8(0));
    UNARY_CASE(mov, IR_CONST_U8(255), IR_CONST_U8(255));
    UNARY_CASE(mov, IR_CONST_U16(0), IR_CONST_U16(0));
    UNARY_CASE(mov, IR_CONST_U16(65535), IR_CONST_U16(65535));
    UNARY_CASE(mov, IR_CONST_U32(0), IR_CONST_U32(0));
    UNARY_CASE(mov, IR_CONST_U32(UINT32_MAX), IR_CONST_U32(UINT32_MAX));
    UNARY_CASE(mov, IR_CONST_U64(0), IR_CONST_U64(0));
    UNARY_CASE(mov, IR_CONST_U64(UINT64_MAX), IR_CONST_U64(UINT64_MAX));
    UNARY_CASE(mov, IR_CONST_U128(I128_ZERO), IR_CONST_U128(I128_ZERO));
    UNARY_CASE(mov, IR_CONST_U128(UI128_MAX), IR_CONST_U128(UI128_MAX));
    UNARY_CASE(mov, IR_CONST_BOOL(false), IR_CONST_BOOL(false));
    UNARY_CASE(mov, IR_CONST_BOOL(true), IR_CONST_BOOL(true));
    UNARY_CASE(mov, IR_CONST_F32(1.5f), IR_CONST_F32(1.5f));
    UNARY_CASE(mov, IR_CONST_F32(-2.25f), IR_CONST_F32(-2.25f));
    UNARY_CASE(mov, IR_CONST_F64(1.5), IR_CONST_F64(1.5));
    UNARY_CASE(mov, IR_CONST_F64(-2.25), IR_CONST_F64(-2.25));
    UNARY_CASE(signext, IR_CONST_U8(128), IR_CONST_S32(-128));
    UNARY_CASE(signext, IR_CONST_U8(127), IR_CONST_S32(127));
    UNARY_CASE(signext, IR_CONST_U16(0x8000), IR_CONST_S64(-32768));
    UNARY_CASE(signext, IR_CONST_U16(0x7fff), IR_CONST_S64(32767));
    UNARY_CASE(zeroext, IR_CONST_U8(128), IR_CONST_U32(128));
    UNARY_CASE(zeroext, IR_CONST_U8(255), IR_CONST_U32(255));
    UNARY_CASE(zeroext, IR_CONST_U32(0x1234abcd), IR_CONST_U16(0xabcd));
    UNARY_CASE(zeroext, IR_CONST_U32(0x43211234), IR_CONST_U16(0x1234));
    UNARY_CASE(seqz, IR_CONST_U32(0), IR_CONST_BOOL(true));
    UNARY_CASE(seqz, IR_CONST_U32(5), IR_CONST_BOOL(false));
    UNARY_CASE(snez, IR_CONST_U32(0), IR_CONST_BOOL(false));
    UNARY_CASE(snez, IR_CONST_U32(5), IR_CONST_BOOL(true));
    UNARY_CASE(seqz, IR_CONST_F32(0.0f), IR_CONST_BOOL(true));
    UNARY_CASE(seqz, IR_CONST_F32(1.5f), IR_CONST_BOOL(false));
    UNARY_CASE(snez, IR_CONST_F64(0.0), IR_CONST_BOOL(false));
    UNARY_CASE(snez, IR_CONST_F64(-2.25), IR_CONST_BOOL(true));
    UNARY_CASE(neg, IR_CONST_S32(7), IR_CONST_S32(-7));
    UNARY_CASE(neg, IR_CONST_S32(-9), IR_CONST_S32(9));
    UNARY_CASE(neg, IR_CONST_F32(1.5f), IR_CONST_F32(-1.5f));
    UNARY_CASE(neg, IR_CONST_F32(-2.25f), IR_CONST_F32(2.25f));
    UNARY_CASE(bneg, IR_CONST_U32(0), IR_CONST_U32(UINT32_MAX));
    UNARY_CASE(bneg, IR_CONST_U32(UINT32_MAX), IR_CONST_U32(0));
    UNARY_CASE(bneg, IR_CONST_BOOL(false), IR_CONST_BOOL(true));
    UNARY_CASE(bneg, IR_CONST_BOOL(true), IR_CONST_BOOL(false));

    // Ordered and equality comparisons each exercise true and false outcomes.
    BINARY_CASE(sgtu, IR_CONST_U32(5), IR_CONST_U32(3), IR_CONST_BOOL(true));
    BINARY_CASE(sgtu, IR_CONST_U32(3), IR_CONST_U32(5), IR_CONST_BOOL(false));
    BINARY_CASE(sleu, IR_CONST_U32(3), IR_CONST_U32(3), IR_CONST_BOOL(true));
    BINARY_CASE(sleu, IR_CONST_U32(5), IR_CONST_U32(3), IR_CONST_BOOL(false));
    BINARY_CASE(sltu, IR_CONST_U32(3), IR_CONST_U32(5), IR_CONST_BOOL(true));
    BINARY_CASE(sltu, IR_CONST_U32(5), IR_CONST_U32(3), IR_CONST_BOOL(false));
    BINARY_CASE(sgeu, IR_CONST_U32(3), IR_CONST_U32(3), IR_CONST_BOOL(true));
    BINARY_CASE(sgeu, IR_CONST_U32(3), IR_CONST_U32(5), IR_CONST_BOOL(false));
    BINARY_CASE(sgts, IR_CONST_S32(1), IR_CONST_S32(-3), IR_CONST_BOOL(true));
    BINARY_CASE(sgts, IR_CONST_S32(-3), IR_CONST_S32(1), IR_CONST_BOOL(false));
    BINARY_CASE(sles, IR_CONST_S32(-3), IR_CONST_S32(-3), IR_CONST_BOOL(true));
    BINARY_CASE(sles, IR_CONST_S32(1), IR_CONST_S32(-3), IR_CONST_BOOL(false));
    BINARY_CASE(slts, IR_CONST_S32(-3), IR_CONST_S32(1), IR_CONST_BOOL(true));
    BINARY_CASE(slts, IR_CONST_S32(1), IR_CONST_S32(-3), IR_CONST_BOOL(false));
    BINARY_CASE(sges, IR_CONST_S32(-3), IR_CONST_S32(-3), IR_CONST_BOOL(true));
    BINARY_CASE(sges, IR_CONST_S32(-3), IR_CONST_S32(1), IR_CONST_BOOL(false));
    BINARY_CASE(seq, IR_CONST_U32(7), IR_CONST_U32(7), IR_CONST_BOOL(true));
    BINARY_CASE(seq, IR_CONST_U32(7), IR_CONST_U32(8), IR_CONST_BOOL(false));
    BINARY_CASE(sne, IR_CONST_U32(7), IR_CONST_U32(8), IR_CONST_BOOL(true));
    BINARY_CASE(sne, IR_CONST_U32(7), IR_CONST_U32(7), IR_CONST_BOOL(false));
    BINARY_CASE(seq, IR_CONST_BOOL(false), IR_CONST_BOOL(false), IR_CONST_BOOL(true));
    BINARY_CASE(seq, IR_CONST_BOOL(false), IR_CONST_BOOL(true), IR_CONST_BOOL(false));
    BINARY_CASE(sne, IR_CONST_F32(1.5f), IR_CONST_F32(2.25f), IR_CONST_BOOL(true));
    BINARY_CASE(sne, IR_CONST_F32(1.5f), IR_CONST_F32(1.5f), IR_CONST_BOOL(false));

    BINARY_CASE(add, IR_CONST_U32(19), IR_CONST_U32(23), IR_CONST_U32(42));
    BINARY_CASE(add, IR_CONST_U32(0), IR_CONST_U32(0), IR_CONST_U32(0));
    BINARY_CASE(sub, IR_CONST_U32(9), IR_CONST_U32(9), IR_CONST_U32(0));
    BINARY_CASE(sub, IR_CONST_U32(23), IR_CONST_U32(7), IR_CONST_U32(16));
    BINARY_CASE(mul, IR_CONST_U32(6), IR_CONST_U32(7), IR_CONST_U32(42));
    BINARY_CASE(mul, IR_CONST_U32(3), IR_CONST_U32(5), IR_CONST_U32(15));
    BINARY_CASE(divu, IR_CONST_U32(21), IR_CONST_U32(3), IR_CONST_U32(7));
    BINARY_CASE(divu, IR_CONST_U32(29), IR_CONST_U32(4), IR_CONST_U32(7));
    BINARY_CASE(divu, IR_CONST_U32(3), IR_CONST_U32(5), IR_CONST_U32(0));
    BINARY_CASE(remu, IR_CONST_U32(23), IR_CONST_U32(5), IR_CONST_U32(3));
    BINARY_CASE(remu, IR_CONST_U32(20), IR_CONST_U32(5), IR_CONST_U32(0));
    // Exact negative division avoids the documentation's rounding ambiguity.
    BINARY_CASE(divs, IR_CONST_S32(-21), IR_CONST_S32(3), IR_CONST_S32(-7));
    BINARY_CASE(divs, IR_CONST_S32(20), IR_CONST_S32(-4), IR_CONST_S32(-5));
    BINARY_CASE(rems, IR_CONST_S32(23), IR_CONST_S32(5), IR_CONST_S32(3));
    BINARY_CASE(rems, IR_CONST_S32(20), IR_CONST_S32(5), IR_CONST_S32(0));
    BINARY_CASE(shl, IR_CONST_U32(3), IR_CONST_U32(4), IR_CONST_U32(48));
    BINARY_CASE(shl, IR_CONST_U32(1), IR_CONST_U32(0), IR_CONST_U32(1));
    BINARY_CASE(shru, IR_CONST_U32(48), IR_CONST_U32(4), IR_CONST_U32(3));
    BINARY_CASE(shru, IR_CONST_U32(0x80000000), IR_CONST_U32(31), IR_CONST_U32(1));
    BINARY_CASE(shrs, IR_CONST_S32(-16), IR_CONST_U32(2), IR_CONST_S32(-4));
    BINARY_CASE(shrs, IR_CONST_S32(16), IR_CONST_U32(2), IR_CONST_S32(4));
    BINARY_CASE(band, IR_CONST_U32(0x5a), IR_CONST_U32(0x0f), IR_CONST_U32(0x0a));
    BINARY_CASE(band, IR_CONST_U32(0xf0), IR_CONST_U32(0x0f), IR_CONST_U32(0));
    BINARY_CASE(bor, IR_CONST_U32(0x50), IR_CONST_U32(0x0a), IR_CONST_U32(0x5a));
    BINARY_CASE(bor, IR_CONST_U32(0xf0), IR_CONST_U32(0x0f), IR_CONST_U32(0xff));
    BINARY_CASE(bxor, IR_CONST_U32(0x5a), IR_CONST_U32(0x0f), IR_CONST_U32(0x55));
    BINARY_CASE(bxor, IR_CONST_U32(0xff), IR_CONST_U32(0xff), IR_CONST_U32(0));
    BINARY_CASE(add, IR_CONST_F64(1.5), IR_CONST_F64(2.25), IR_CONST_F64(3.75));
    BINARY_CASE(add, IR_CONST_F64(-1.5), IR_CONST_F64(1.5), IR_CONST_F64(0.0));
    BINARY_CASE(sub, IR_CONST_F32(3.75f), IR_CONST_F32(1.5f), IR_CONST_F32(2.25f));
    BINARY_CASE(sub, IR_CONST_F32(1.5f), IR_CONST_F32(3.75f), IR_CONST_F32(-2.25f));
    BINARY_CASE(mul, IR_CONST_F64(1.5), IR_CONST_F64(2.0), IR_CONST_F64(3.0));
    BINARY_CASE(mul, IR_CONST_F64(-2.25), IR_CONST_F64(2.0), IR_CONST_F64(-4.5));

    CHAIN_CASE(23, 46);
    CHAIN_CASE(5, 10);
    RUNTIME_CASE(false, 7, input);
    RUNTIME_CASE(false, 11, other_input);
    RUNTIME_CASE(true, 0, input);
    RUNTIME_CASE(true, 0, other_input);
    PHI_CONSTANT_CASE(7, 7);
    PHI_CONSTANT_CASE(19, 19);
    PHI_CONSTANT_CASE(7, 8);
    PHI_CONSTANT_CASE(19, 23);
    PHI_VARIABLE_CASE(PHI_ALIAS, value, value);
    PHI_VARIABLE_CASE(PHI_ALIAS, other_value, other_value);
    PHI_VARIABLE_CASE(PHI_DIFFERENT_VARS, first, second);
    PHI_VARIABLE_CASE(PHI_DIFFERENT_VARS, left_value, right_value);

    // Bit reinterpretation has explicit IEEE-754 bit patterns, not numeric casts.
    // Keep these last so an unsupported bitcast does not hide other case reports.
    UNARY_CASE(bitcast, IR_CONST_U32(0x3f800000), IR_CONST_F32(1.0f));
    UNARY_CASE(bitcast, IR_CONST_U32(0xc0000000), IR_CONST_F32(-2.0f));
    UNARY_CASE(bitcast, IR_CONST_F32(1.0f), IR_CONST_U32(0x3f800000));
    UNARY_CASE(bitcast, IR_CONST_F32(-2.0f), IR_CONST_U32(0xc0000000));
    return result;
}
