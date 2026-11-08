
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir.h"
#include "ir_optimizer.h"
#include "ir_serialization.h"
#include "testcase.h"

#include <stdio.h>



// Follow register copies/conversions, retaining the type checks at each step.
static int follow_copies(ir_func_t *func, ir_var_t **value, size_t *count) {
    while ((*value)->assigned_at.len == 1) {
        ir_insn_t *insn = set_next(&(*value)->assigned_at, NULL)->value;
        if (insn->type != IR_INSN_EXPR1) {
            break;
        }
        TEST_EXPECT_UINT(insn->operands_len, 1u, "%s", func->name);
        TEST_EXPECT_UINT(insn->returns_len, 1u, "%s", func->name);
        TEST_EXPECT_INT(insn->returns[0].type, IR_RETVAL_TYPE_VAR, "%s", func->name);
        TEST_ASSERT(insn->returns[0].dest_var == *value, "%s", func->name);
        TEST_EXPECT_INT(insn->operands[0].type, IR_OPERAND_TYPE_VAR, "%s", func->name);
        ir_var_t *src = insn->operands[0].var;
        if (insn->op1 == IR_OP1_mov) {
            TEST_EXPECT_INT(src->prim_type, (*value)->prim_type, "%s", func->name);
        } else if (insn->op1 == IR_OP1_signext || insn->op1 == IR_OP1_zeroext) {
            TEST_ASSERT(ir_prim_is_integer_or_bool(src->prim_type), "%s", func->name);
            TEST_ASSERT(ir_prim_is_integer_or_bool((*value)->prim_type), "%s", func->name);
        } else {
            TEST_EXPECT_INT(insn->op1, IR_OP1_bitcast, "%s", func->name);
            TEST_ASSERT(ir_prim_is_float(src->prim_type) || ir_prim_is_float((*value)->prim_type), "%s", func->name);
        }
        *value = src;
        (*count)++;
    }
    TEST_OK();
}

static int check_copy(ir_func_t *func, ir_var_t *src, ir_prim_t dest_type) {
    ir_insn_t *ret         = NULL;
    size_t     expressions = 0;
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        dlist_foreach_node(ir_insn_t, insn, &code->insns) {
            TEST_ASSERT(insn->type != IR_INSN_LOAD, "%s: load was not promoted", func->name);
            TEST_ASSERT(insn->type != IR_INSN_STORE, "%s: store was not promoted", func->name);
            if (insn->type == IR_INSN_EXPR1 || insn->type == IR_INSN_EXPR2) {
                expressions++;
            } else {
                TEST_EXPECT_INT(insn->type, IR_INSN_RETURN, "%s", func->name);
                TEST_ASSERT(!ret, "%s: unexpected extra return", func->name);
                ret = insn;
            }
        }
    }

    TEST_ASSERT(ret, "%s: missing return", func->name);
    TEST_EXPECT_UINT(ret->operands_len, 1u, "%s", func->name);
    TEST_EXPECT_INT(ret->operands[0].type, IR_OPERAND_TYPE_VAR, "%s", func->name);
    TEST_EXPECT_INT(ir_operand_prim(ret->operands[0]), dest_type, "%s", func->name);

    size_t    count = 0;
    ir_var_t *value = ret->operands[0].var;
    TEST_EXPECT_INT(follow_copies(func, &value, &count), 0, "%s", func->name);
    if (value != src) {
        // A partial store preserves the other bytes of the replacement variable:
        // (old & ~(data_mask << shift)) | (converted_source << shift).
        TEST_EXPECT_UINT(value->assigned_at.len, 1u, "%s: missing store merge", func->name);
        ir_insn_t *merge = set_next(&value->assigned_at, NULL)->value;
        TEST_EXPECT_INT(merge->type, IR_INSN_EXPR2, "%s", func->name);
        TEST_EXPECT_INT(merge->op2, IR_OP2_bor, "%s", func->name);
        TEST_EXPECT_UINT(merge->operands_len, 2u, "%s", func->name);
        TEST_EXPECT_INT(merge->operands[0].type, IR_OPERAND_TYPE_VAR, "%s", func->name);
        TEST_EXPECT_INT(merge->operands[1].type, IR_OPERAND_TYPE_VAR, "%s", func->name);
        ir_var_t *masked  = merge->operands[0].var;
        ir_var_t *shifted = merge->operands[1].var;
        TEST_EXPECT_INT(masked->prim_type, value->prim_type, "%s", func->name);
        TEST_EXPECT_INT(shifted->prim_type, value->prim_type, "%s", func->name);
        count++;

        unsigned int shift = 0;
        TEST_EXPECT_UINT(shifted->assigned_at.len, 1u, "%s", func->name);
        ir_insn_t *shl = set_next(&shifted->assigned_at, NULL)->value;
        if (shl->type == IR_INSN_EXPR2) {
            TEST_EXPECT_INT(shl->op2, IR_OP2_shl, "%s", func->name);
            TEST_EXPECT_UINT(shl->operands_len, 2u, "%s", func->name);
            TEST_EXPECT_INT(shl->operands[0].type, IR_OPERAND_TYPE_VAR, "%s", func->name);
            TEST_EXPECT_INT(shl->operands[1].type, IR_OPERAND_TYPE_CONST, "%s", func->name);
            TEST_EXPECT_UINT(shl->operands[1].iconst.consth, 0u, "%s", func->name);
            TEST_COMPARE_UINT(shl->operands[1].iconst.constl, <, 128u, "%s", func->name);
            shift = shl->operands[1].iconst.constl;
            TEST_EXPECT_UINT(shift % 8, 0u, "%s", func->name);
            shifted = shl->operands[0].var;
            count++;
        }
        TEST_COMPARE_UINT(
            shift + ir_prim_size(src->prim_type) * 8,
            <=,
            ir_prim_size(value->prim_type) * 8,
            "%s",
            func->name
        );
        TEST_EXPECT_INT(follow_copies(func, &shifted, &count), 0, "%s", func->name);
        TEST_ASSERT(shifted == src, "%s: wrong store source", func->name);

        TEST_EXPECT_UINT(masked->assigned_at.len, 1u, "%s", func->name);
        ir_insn_t *band = set_next(&masked->assigned_at, NULL)->value;
        TEST_EXPECT_INT(band->type, IR_INSN_EXPR2, "%s", func->name);
        TEST_EXPECT_INT(band->op2, IR_OP2_band, "%s", func->name);
        TEST_EXPECT_UINT(band->operands_len, 2u, "%s", func->name);
        TEST_EXPECT_INT(band->operands[0].type, IR_OPERAND_TYPE_VAR, "%s", func->name);
        TEST_EXPECT_INT(band->operands[0].var->prim_type, value->prim_type, "%s", func->name);
        TEST_EXPECT_INT(band->operands[1].type, IR_OPERAND_TYPE_CONST, "%s", func->name);
        TEST_EXPECT_INT(band->operands[1].iconst.prim_type, value->prim_type, "%s", func->name);
        i128_t data_mask = shr128u(UI128_MAX, 128 - ir_prim_size(src->prim_type) * 8);
        i128_t type_mask = shr128u(UI128_MAX, 128 - ir_prim_size(value->prim_type) * 8);
        i128_t cutout    = bneg128(shl128(data_mask, shift));
        TEST_ASSERT(
            eq128(and128(band->operands[1].iconst.const128, type_mask), and128(cutout, type_mask)),
            "%s: wrong cutout mask",
            func->name
        );
        count++;
    }
    TEST_ASSERT(count > 0, "%s: missing copy/conversion", func->name);
    TEST_EXPECT_UINT(count, expressions, "%s: unexpected instructions outside copy sequence", func->name);
    TEST_OK();
}

static int test_copy(char const *name, ir_prim_t src_type, ir_prim_t dest_type) {
    ir_func_t  *func  = ir_func_create(name, NULL, 1);
    ir_var_t   *src   = ir_var_create(func, src_type, "src");
    ir_var_t   *dest  = ir_var_create(func, dest_type, "dest");
    ir_frame_t *frame = ir_frame_create(func, ir_prim_size(src_type), ir_prim_size(src_type), "slot");

    src->arg_index          = 0;
    func->args[0].arg_type  = IR_FUNCARG_TYPE_VAR;
    func->args[0].var       = src;
    func->rettype.type      = IR_FUNCRET_PRIM;
    func->rettype.prim_type = dest_type;

    // Only direct accesses at offset zero. Each access matches its memory type.
    ir_add_store(IR_APPEND(func->entry), IR_OPERAND_VAR(src), IR_MEMREF(src_type, IR_BADDR_FRAME(frame)));
    ir_add_load(IR_APPEND(func->entry), IR_RETVAL_VAR(dest), IR_MEMREF(dest_type, IR_BADDR_FRAME(frame)));
    ir_add_return1(IR_APPEND(func->entry), IR_OPERAND_VAR(dest));

    // Inspect the pass directly, before copy propagation can remove mov instructions.
    ir_func_to_ssa(func);
    bool changed = opt_mem2reg(func);
    ir_func_serialize(func, NULL, stdout);
    int result = check_copy(func, src, dest_type);
    ir_func_delete(func);
    TEST_ASSERT(changed, "%s: pass reported no change", name);
    return result;
}

int main() {
    int result = 0;

    // Same-type accesses become mov.
    result |= test_copy("mov_i8", IR_PRIM_i8, IR_PRIM_i8);
    result |= test_copy("mov_i16", IR_PRIM_i16, IR_PRIM_i16);
    result |= test_copy("mov_i32", IR_PRIM_i32, IR_PRIM_i32);
    result |= test_copy("mov_i64", IR_PRIM_i64, IR_PRIM_i64);
    result |= test_copy("mov_i128", IR_PRIM_i128, IR_PRIM_i128);
    result |= test_copy("mov_bool", IR_PRIM_bool, IR_PRIM_bool);
    result |= test_copy("mov_f32", IR_PRIM_f32, IR_PRIM_f32);
    result |= test_copy("mov_f64", IR_PRIM_f64, IR_PRIM_f64);

    // Narrow loads read only initialized bytes; no widening loads are used.
    result |= test_copy("narrow_i16_i8", IR_PRIM_i16, IR_PRIM_i8);
    result |= test_copy("narrow_i32_i16", IR_PRIM_i32, IR_PRIM_i16);
    result |= test_copy("narrow_i64_i32", IR_PRIM_i64, IR_PRIM_i32);
    result |= test_copy("narrow_i128_i64", IR_PRIM_i128, IR_PRIM_i64);

    // Equal-width loads reinterpret the stored representation without numeric conversion.
    result |= test_copy("bitcast_i32_f32", IR_PRIM_i32, IR_PRIM_f32);
    result |= test_copy("bitcast_f32_i32", IR_PRIM_f32, IR_PRIM_i32);
    result |= test_copy("bitcast_i64_f64", IR_PRIM_i64, IR_PRIM_f64);
    result |= test_copy("bitcast_f64_i64", IR_PRIM_f64, IR_PRIM_i64);

    return result;
}
