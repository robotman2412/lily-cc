
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir.h"
#include "ir_optimizer.h"
#include "lilycc_malloc.h"
#include "optimizer_test.h"
#include "testcase.h"

#include <string.h>



static ir_var_t *argument(ir_func_t *func, size_t index, ir_prim_t type, char const *name) {
    ir_var_t *var              = ir_var_create(func, type, name);
    var->arg_index             = index;
    func->args[index].arg_type = IR_FUNCARG_TYPE_VAR;
    func->args[index].var      = var;
    return var;
}

static bool has_named_var(ir_func_t *func, char const *name) {
    dlist_foreach_node(ir_var_t, var, &func->vars_list) {
        if (!strcmp(var->name, name)) {
            return true;
        }
    }
    return false;
}

static size_t instruction_count(ir_func_t *func, ir_insn_type_t type) {
    size_t count = 0;
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        dlist_foreach_node(ir_insn_t, insn, &code->insns) {
            count += insn->type == type;
        }
    }
    return count;
}

// Resolve copies using instruction scans, so a legal copy elimination does not
// make a test depend on the names or identities of intermediate variables.
static bool is_copy_of_argument(ir_func_t *func, ir_operand_t value, size_t arg_index) {
    if (func->args[arg_index].arg_type != IR_FUNCARG_TYPE_VAR) {
        return false;
    }
    for (size_t depth = 0; depth <= func->vars_list.len; depth++) {
        if (value.type != IR_OPERAND_TYPE_VAR) {
            return false;
        }
        if (value.var == func->args[arg_index].var) {
            return true;
        }
        ir_insn_t *definition = NULL;
        dlist_foreach_node(ir_code_t, code, &func->code_list) {
            dlist_foreach_node(ir_insn_t, insn, &code->insns) {
                if (opt_test_insn_defines(insn, value.var)) {
                    if (definition) {
                        return false;
                    }
                    definition = insn;
                }
            }
        }
        if (!definition || definition->type != IR_INSN_EXPR1 || definition->op1 != IR_OP1_mov
            || definition->operands_len != 1u) {
            return false;
        }
        if (ir_operand_prim(definition->operands[0]) != value.var->prim_type) {
            return false;
        }
        value = definition->operands[0];
    }
    return false;
}

static int dead_pure_chain(void) {
    ir_func_t *func = ir_func_create("unused_pure_chain", "entry", 0);
    ir_var_t  *bare = ir_var_create(func, IR_PRIM_i32, "bare");
    ir_var_t  *a    = ir_var_create(func, IR_PRIM_i32, "a");
    ir_var_t  *b    = ir_var_create(func, IR_PRIM_i32, "b");
    ir_var_t  *c    = ir_var_create(func, IR_PRIM_i32, "c");
    ir_add_expr1(IR_APPEND(func->entry), IR_RETVAL_VAR(a), IR_OP1_mov, IR_OPERAND_CONST(IR_CONST_U32(11)));
    ir_add_expr2(
        IR_APPEND(func->entry),
        IR_RETVAL_VAR(b),
        IR_OP2_add,
        IR_OPERAND_VAR(a),
        IR_OPERAND_CONST(IR_CONST_U32(7))
    );
    ir_add_expr1(IR_APPEND(func->entry), IR_RETVAL_VAR(c), IR_OP1_neg, IR_OPERAND_VAR(b));
    ir_add_return0(IR_APPEND(func->entry));
    func->enforce_ssa = true;

    // The first two definitions are consumed, but only by other dead computations.
    TEST_EXPECT_UINT(opt_test_reads(func, bare), 0u);
    TEST_EXPECT_UINT(opt_test_defs(func, bare), 0u);
    TEST_EXPECT_UINT(opt_test_reads(func, a), 1u);
    TEST_EXPECT_UINT(opt_test_reads(func, b), 1u);
    TEST_EXPECT_UINT(opt_test_reads(func, c), 0u);
    TEST_EXPECT_UINT(opt_test_defs(func, a), 1u);
    TEST_EXPECT_UINT(opt_test_defs(func, b), 1u);
    TEST_EXPECT_UINT(opt_test_defs(func, c), 1u);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_ASSERT(opt_unused_vars(func));
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_EXPECT_UINT(func->vars_list.len, 0u);
    TEST_EXPECT_UINT(instruction_count(func, IR_INSN_EXPR1), 0u);
    TEST_EXPECT_UINT(instruction_count(func, IR_INSN_EXPR2), 0u);
    TEST_EXPECT_UINT(instruction_count(func, IR_INSN_RETURN), 1u);
    TEST_ASSERT(!opt_unused_vars(func));
    ir_func_delete(func);
    TEST_OK();
}

static int observable_uses(void) {
    ir_func_t *func        = ir_func_create("unused_observable_uses", "entry", 4);
    ir_var_t  *ret_arg     = argument(func, 0, IR_PRIM_i32, "ret_arg");
    ir_var_t  *store_arg   = argument(func, 1, IR_PRIM_i32, "store_arg");
    ir_var_t  *call_arg    = argument(func, 2, IR_PRIM_i32, "call_arg");
    ir_var_t  *addr_arg    = argument(func, 3, IR_PRIM_i64, "addr_arg");
    ir_var_t  *ret_value   = ir_var_create(func, IR_PRIM_i32, "ret_value");
    ir_var_t  *stored      = ir_var_create(func, IR_PRIM_i32, "stored");
    ir_var_t  *passed      = ir_var_create(func, IR_PRIM_i32, "passed");
    ir_var_t  *address     = ir_var_create(func, IR_PRIM_i64, "address");
    ir_var_t  *call_result = ir_var_create(func, IR_PRIM_i32, "call_result");
    ir_var_t  *load_result = ir_var_create(func, IR_PRIM_i32, "load_result");
    ir_var_t  *dead        = ir_var_create(func, IR_PRIM_i32, "dead");
    func->rettype          = (ir_funcret_t){.type = IR_FUNCRET_PRIM, .prim_type = IR_PRIM_i32};

    ir_add_expr1(IR_APPEND(func->entry), IR_RETVAL_VAR(ret_value), IR_OP1_mov, IR_OPERAND_VAR(ret_arg));
    ir_add_expr1(IR_APPEND(func->entry), IR_RETVAL_VAR(stored), IR_OP1_mov, IR_OPERAND_VAR(store_arg));
    ir_add_expr1(IR_APPEND(func->entry), IR_RETVAL_VAR(passed), IR_OP1_mov, IR_OPERAND_VAR(call_arg));
    ir_add_expr1(IR_APPEND(func->entry), IR_RETVAL_VAR(address), IR_OP1_mov, IR_OPERAND_VAR(addr_arg));
    ir_add_expr1(IR_APPEND(func->entry), IR_RETVAL_VAR(dead), IR_OP1_neg, IR_OPERAND_VAR(ret_arg));
    ir_memref_t memory = IR_MEMREF(IR_PRIM_i32, IR_BADDR_VAR(address));
    ir_add_store(IR_APPEND(func->entry), IR_OPERAND_VAR(stored), memory);
    ir_operand_t param = IR_OPERAND_VAR(passed);
    ir_insn_t   *call  = ir_add_call(
        IR_APPEND(func->entry),
        IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM("observe")),
        true,
        IR_RETVAL_VAR(call_result),
        1,
        &param
    );
    call->flags     |= IR_INSN_FLAG_SIDE_EFFECTS | IR_INSN_FLAG_IMPURE;
    ir_insn_t *load  = ir_add_load(IR_APPEND(func->entry), IR_RETVAL_VAR(load_result), memory);
    load->flags     |= IR_INSN_FLAG_VOLATILE | IR_INSN_FLAG_SIDE_EFFECTS;
    ir_add_return1(IR_APPEND(func->entry), IR_OPERAND_VAR(ret_value));
    func->enforce_ssa = true;

    TEST_EXPECT_UINT(opt_test_reads(func, ret_value), 1u);
    TEST_EXPECT_UINT(opt_test_reads(func, stored), 1u);
    TEST_EXPECT_UINT(opt_test_reads(func, passed), 1u);
    TEST_EXPECT_UINT(opt_test_reads(func, address), 2u);
    TEST_EXPECT_UINT(opt_test_reads(func, call_result), 0u);
    TEST_EXPECT_UINT(opt_test_reads(func, load_result), 0u);
    TEST_EXPECT_UINT(opt_test_reads(func, dead), 0u);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_ASSERT(opt_unused_vars(func));
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_ASSERT(!has_named_var(func, "dead"));
    TEST_EXPECT_UINT(instruction_count(func, IR_INSN_CALL), 1u);
    TEST_EXPECT_UINT(instruction_count(func, IR_INSN_STORE), 1u);
    TEST_EXPECT_UINT(instruction_count(func, IR_INSN_LOAD), 1u);
    TEST_EXPECT_UINT(instruction_count(func, IR_INSN_RETURN), 1u);
    // Verify that preserving the opcode also preserved the observable operands/flags.
    dlist_foreach_node(ir_insn_t, insn, &func->entry->insns) {
        if (insn->type == IR_INSN_CALL) {
            TEST_EXPECT_UINT(insn->operands_len, 2u);
            TEST_EXPECT_INT(insn->operands[1].type, IR_OPERAND_TYPE_VAR);
            TEST_ASSERT(is_copy_of_argument(func, insn->operands[1], 2));
            TEST_ASSERT(insn->flags & IR_INSN_FLAG_SIDE_EFFECTS);
        } else if (insn->type == IR_INSN_LOAD) {
            TEST_ASSERT(insn->flags & IR_INSN_FLAG_VOLATILE);
            TEST_EXPECT_INT(insn->operands[0].mem.base_type, IR_MEMBASE_VAR);
            TEST_ASSERT(is_copy_of_argument(func, IR_OPERAND_VAR(insn->operands[0].mem.base_var), 3));
        } else if (insn->type == IR_INSN_STORE) {
            TEST_ASSERT(is_copy_of_argument(func, IR_OPERAND_VAR(insn->operands[0].mem.base_var), 3));
            TEST_ASSERT(is_copy_of_argument(func, insn->operands[1], 1));
        } else if (insn->type == IR_INSN_RETURN) {
            TEST_EXPECT_UINT(insn->operands_len, 1u);
            TEST_ASSERT(is_copy_of_argument(func, insn->operands[0], 0));
        }
    }
    ir_func_delete(func);
    TEST_OK();
}

static int branch_condition(void) {
    ir_func_t *func  = ir_func_create("unused_branch_condition", "entry", 1);
    ir_var_t  *input = argument(func, 0, IR_PRIM_i32, "input");
    ir_var_t  *cond  = ir_var_create(func, IR_PRIM_bool, "cond");
    ir_code_t *yes   = ir_code_create(func, "yes");
    ir_code_t *no    = ir_code_create(func, "no");
    ir_add_expr1(IR_APPEND(func->entry), IR_RETVAL_VAR(cond), IR_OP1_snez, IR_OPERAND_VAR(input));
    ir_add_branch(IR_APPEND(func->entry), IR_OPERAND_VAR(cond), yes);
    ir_add_jump(IR_APPEND(func->entry), no);
    ir_add_return0(IR_APPEND(yes));
    ir_add_return0(IR_APPEND(no));
    func->enforce_ssa = true;

    TEST_EXPECT_UINT(opt_test_reads(func, input), 1u);
    TEST_EXPECT_UINT(opt_test_reads(func, cond), 1u);
    TEST_EXPECT_UINT(opt_test_defs(func, cond), 1u);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_ASSERT(!opt_unused_vars(func));
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_ASSERT(has_named_var(func, "input"));
    TEST_ASSERT(has_named_var(func, "cond"));
    TEST_EXPECT_UINT(instruction_count(func, IR_INSN_BRANCH), 1u);
    ir_func_delete(func);
    TEST_OK();
}

static int combinator_cycle(bool live) {
    ir_func_t       *func   = ir_func_create(live ? "live_combinator_cycle" : "dead_combinator_cycle", "entry", 1);
    ir_var_t        *cond   = argument(func, 0, IR_PRIM_bool, "cond");
    ir_var_t        *a      = ir_var_create(func, IR_PRIM_i32, "a");
    ir_var_t        *b      = ir_var_create(func, IR_PRIM_i32, "b");
    ir_code_t       *loop   = ir_code_create(func, "loop");
    ir_code_t       *exit   = ir_code_create(func, "exit");
    ir_combinator_t *from_a = lilycc_malloc(2 * sizeof(*from_a));
    ir_combinator_t *from_b = lilycc_malloc(2 * sizeof(*from_b));
    from_a[0]               = (ir_combinator_t){.pred = func->entry, .bind = IR_OPERAND_CONST(IR_CONST_U32(1))};
    from_a[1]               = (ir_combinator_t){.pred = loop, .bind = IR_OPERAND_VAR(b)};
    from_b[0]               = (ir_combinator_t){.pred = func->entry, .bind = IR_OPERAND_CONST(IR_CONST_U32(2))};
    from_b[1]               = (ir_combinator_t){.pred = loop, .bind = IR_OPERAND_VAR(a)};
    ir_add_jump(IR_APPEND(func->entry), loop);
    ir_add_combinator(IR_APPEND(loop), a, 2, from_a);
    ir_add_combinator(IR_APPEND(loop), b, 2, from_b);
    ir_add_branch(IR_APPEND(loop), IR_OPERAND_VAR(cond), loop);
    ir_add_jump(IR_APPEND(loop), exit);
    if (live) {
        func->rettype = (ir_funcret_t){.type = IR_FUNCRET_PRIM, .prim_type = IR_PRIM_i32};
        ir_add_return1(IR_APPEND(exit), IR_OPERAND_VAR(a));
    } else {
        ir_add_return0(IR_APPEND(exit));
    }
    func->enforce_ssa = true;

    // Only a live return makes the mutually dependent phi values observable.
    TEST_EXPECT_UINT(opt_test_reads(func, a), live ? 2u : 1u);
    TEST_EXPECT_UINT(opt_test_reads(func, b), 1u);
    TEST_EXPECT_UINT(opt_test_defs(func, a), 1u);
    TEST_EXPECT_UINT(opt_test_defs(func, b), 1u);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_EXPECT_INT(opt_unused_vars(func), !live);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_EXPECT_INT(has_named_var(func, "a"), live);
    TEST_EXPECT_INT(has_named_var(func, "b"), live);
    TEST_EXPECT_UINT(instruction_count(func, IR_INSN_COMBINATOR), live ? 2u : 0u);
    TEST_ASSERT(has_named_var(func, "cond"));
    TEST_EXPECT_UINT(instruction_count(func, IR_INSN_BRANCH), 1u);
    // Break the fixture's phi cycle explicitly before generic block teardown.
    // Removing predecessors during teardown can otherwise form self-bindings.
    if (live) {
        ir_insn_t *insn = (ir_insn_t *)loop->insns.head;
        while (insn) {
            ir_insn_t *next = (ir_insn_t *)insn->node.next;
            if (insn->type == IR_INSN_COMBINATOR) {
                ir_insn_delete(insn);
            }
            insn = next;
        }
    }
    ir_func_delete(func);
    TEST_OK();
}

static int unused_argument(void) {
    ir_func_t *func   = ir_func_create("unused_argument", "entry", 2);
    ir_var_t  *unused = argument(func, 0, IR_PRIM_i16, "unused");
    ir_var_t  *used   = argument(func, 1, IR_PRIM_i32, "used");
    func->rettype     = (ir_funcret_t){.type = IR_FUNCRET_PRIM, .prim_type = IR_PRIM_i32};
    ir_add_return1(IR_APPEND(func->entry), IR_OPERAND_VAR(used));
    func->enforce_ssa = true;

    TEST_EXPECT_UINT(opt_test_reads(func, unused), 0u);
    TEST_EXPECT_UINT(opt_test_reads(func, used), 1u);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    opt_unused_vars(func);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_EXPECT_UINT(func->args_len, 2u);
    TEST_EXPECT_INT(func->args[1].arg_type, IR_FUNCARG_TYPE_VAR);
    TEST_ASSERT(func->args[1].var == used);
    TEST_EXPECT_INT(used->arg_index, 1);
    // Either retain the argument variable or retain its position as an ignored argument.
    if (func->args[0].arg_type == IR_FUNCARG_TYPE_IGNORED) {
        TEST_EXPECT_INT(func->args[0].ignored_prim, IR_PRIM_i16);
        TEST_ASSERT(!has_named_var(func, "unused"));
    } else {
        TEST_EXPECT_INT(func->args[0].arg_type, IR_FUNCARG_TYPE_VAR);
        TEST_ASSERT(has_named_var(func, "unused"));
        TEST_EXPECT_INT(func->args[0].var->prim_type, IR_PRIM_i16);
    }
    ir_func_delete(func);
    TEST_OK();
}

int main(void) {
    int result  = 0;
    result     |= dead_pure_chain();
    result     |= observable_uses();
    result     |= branch_condition();
    result     |= combinator_cycle(false);
    result     |= combinator_cycle(true);
    result     |= unused_argument();
    return result;
}
