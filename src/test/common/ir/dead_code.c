
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir.h"
#include "ir_optimizer.h"
#include "lilycc_malloc.h"
#include "optimizer_test.h"
#include "testcase.h"

#include <string.h>



// Determine reachability from instruction targets, without consulting cached CFG sets.
static int check_reachability(ir_func_t *func, char const *const *expected, size_t expected_len, bool all_reachable) {
    ir_code_t *blocks[16];
    bool       reachable[16] = {0};
    size_t     len           = 0;
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        TEST_ASSERT(len < sizeof(blocks) / sizeof(*blocks));
        blocks[len]      = code;
        reachable[len++] = code == func->entry;
    }
    TEST_ASSERT(len > 0);

    bool changed;
    do {
        changed = false;
        for (size_t i = 0; i < len; i++) {
            if (!reachable[i]) {
                continue;
            }
            bool terminated = false;
            dlist_foreach_node(ir_insn_t, insn, &blocks[i]->insns) {
                if (insn->type == IR_INSN_BRANCH || insn->type == IR_INSN_JUMP) {
                    TEST_ASSERT(insn->operands_len >= 1u);
                    TEST_EXPECT_INT(insn->operands[0].type, IR_OPERAND_TYPE_MEM);
                    TEST_EXPECT_INT(insn->operands[0].mem.base_type, IR_MEMBASE_CODE);
                    TEST_EXPECT_INT(insn->operands[0].mem.offset, 0);
                    bool always_taken = insn->type == IR_INSN_JUMP;
                    if (insn->type == IR_INSN_BRANCH) {
                        TEST_EXPECT_UINT(insn->operands_len, 2u);
                        if (insn->operands[1].type == IR_OPERAND_TYPE_CONST) {
                            TEST_EXPECT_INT(insn->operands[1].iconst.prim_type, IR_PRIM_bool);
                            if (!insn->operands[1].iconst.constl) {
                                continue;
                            }
                            always_taken = true;
                        }
                    }
                    bool found = false;
                    for (size_t j = 0; j < len; j++) {
                        if (blocks[j] == insn->operands[0].mem.base_code) {
                            found = true;
                            if (!reachable[j]) {
                                reachable[j] = true;
                                changed      = true;
                            }
                        }
                    }
                    TEST_ASSERT(found, "%s: target is not a live block", func->name);
                    if (always_taken) {
                        terminated = true;
                        break;
                    }
                }
                if (insn->type == IR_INSN_RETURN || insn->type == IR_INSN_JUMP) {
                    terminated = true;
                    break;
                }
            }
            TEST_ASSERT(terminated, "%s: reachable block has no unconditional terminator", func->name);
        }
    } while (changed);

    size_t reached = 0;
    for (size_t i = 0; i < len; i++) {
        bool wanted = false;
        for (size_t j = 0; j < expected_len; j++) {
            wanted |= !strcmp(blocks[i]->name, expected[j]);
        }
        TEST_EXPECT_INT(reachable[i], wanted, "%s: unexpected reachability of %s", func->name, blocks[i]->name);
        reached += reachable[i];
        if (all_reachable) {
            TEST_ASSERT(reachable[i], "%s: unreachable block survived", func->name);
        }
    }
    TEST_EXPECT_UINT(reached, expected_len, "%s: expected block missing", func->name);
    TEST_OK();
}

static bool has_insn(ir_func_t *func, ir_insn_t *wanted) {
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        dlist_foreach_node(ir_insn_t, insn, &code->insns) {
            if (insn == wanted) {
                return true;
            }
        }
    }
    return false;
}

static int run_dead_code(ir_func_t *func, char const *const *expected, size_t expected_len, bool expect_change) {
    ir_func_recalc_flow(func);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_EXPECT_INT(check_reachability(func, expected, expected_len, false), 0);
    bool changed = opt_dead_code(func);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_EXPECT_INT(check_reachability(func, expected, expected_len, true), 0);
    TEST_EXPECT_INT(changed, expect_change, "%s: incorrect change report", func->name);
    TEST_OK();
}

static ir_insn_t *add_effect(ir_code_t *code, unsigned int value) {
    return ir_add_store(
        IR_APPEND(code),
        IR_OPERAND_CONST(IR_CONST_U32(value)),
        IR_MEMREF(IR_PRIM_i32, IR_BADDR_SYM("observable"))
    );
}

static int disconnected_paths(void) {
    ir_func_t *func   = ir_func_create("disconnected_paths", "entry", 0);
    ir_code_t *live   = ir_code_create(func, "live");
    ir_code_t *dead0  = ir_code_create(func, "dead_chain0");
    ir_code_t *dead1  = ir_code_create(func, "dead_chain1");
    ir_code_t *cycle0 = ir_code_create(func, "dead_cycle0");
    ir_code_t *cycle1 = ir_code_create(func, "dead_cycle1");
    ir_add_jump(IR_APPEND(func->entry), live);
    ir_insn_t *effect = add_effect(live, 17);
    ir_insn_t *ret    = ir_add_return0(IR_APPEND(live));
    add_effect(dead0, 23);
    ir_add_jump(IR_APPEND(dead0), dead1);
    add_effect(dead1, 29);
    ir_add_return0(IR_APPEND(dead1));
    ir_add_jump(IR_APPEND(cycle0), cycle1);
    ir_add_jump(IR_APPEND(cycle1), cycle0);
    char const *expected[] = {"entry", "live"};

    TEST_EXPECT_INT(run_dead_code(func, expected, 2u, true), 0);
    TEST_ASSERT(has_insn(func, effect));
    TEST_ASSERT(has_insn(func, ret));
    TEST_EXPECT_UINT(func->code_list.len, 2u);
    ir_func_delete(func);
    TEST_OK();
}

static int unknown_branch_and_loop(void) {
    ir_func_t *func        = ir_func_create("unknown_branch_and_loop", "entry", 1);
    ir_var_t  *cond        = ir_var_create(func, IR_PRIM_bool, "cond");
    cond->arg_index        = 0;
    func->args[0].arg_type = IR_FUNCARG_TYPE_VAR;
    func->args[0].var      = cond;
    ir_code_t *left        = ir_code_create(func, "left");
    ir_code_t *right       = ir_code_create(func, "right");
    ir_code_t *loop        = ir_code_create(func, "loop");
    ir_code_t *exit        = ir_code_create(func, "exit");
    ir_add_branch(IR_APPEND(func->entry), IR_OPERAND_VAR(cond), left);
    ir_add_jump(IR_APPEND(func->entry), right);
    ir_insn_t *left_effect  = add_effect(left, 31);
    ir_insn_t *right_effect = add_effect(right, 37);
    ir_add_jump(IR_APPEND(left), loop);
    ir_add_jump(IR_APPEND(right), loop);
    ir_add_branch(IR_APPEND(loop), IR_OPERAND_VAR(cond), loop);
    ir_add_jump(IR_APPEND(loop), exit);
    ir_add_return0(IR_APPEND(exit));
    char const *expected[] = {"entry", "left", "right", "loop", "exit"};

    // No instruction defines this argument; both branch outcomes remain possible.
    TEST_EXPECT_UINT(opt_test_defs(func, cond), 0u);
    TEST_EXPECT_UINT(opt_test_reads(func, cond), 2u);
    TEST_EXPECT_INT(run_dead_code(func, expected, 5u, false), 0);
    TEST_ASSERT(has_insn(func, left_effect));
    TEST_ASSERT(has_insn(func, right_effect));
    ir_func_delete(func);
    TEST_OK();
}

static int instructions_after_transfers(void) {
    ir_func_t *func       = ir_func_create("instructions_after_transfers", "entry", 0);
    ir_code_t *exit       = ir_code_create(func, "exit");
    ir_code_t *dead       = ir_code_create(func, "dead_tail");
    ir_insn_t *jump       = ir_add_jump(IR_APPEND(func->entry), exit);
    // The builder permits only branches/jumps/returns (or machine instructions)
    // after a transfer. Put unreachable side effects in the dead target block.
    ir_insn_t *dead_jump0 = ir_add_jump(IR_APPEND(func->entry), dead);
    ir_insn_t *dead_store = add_effect(dead, 41);
    ir_insn_t *dead_call  = ir_add_call(
        IR_APPEND(dead),
        IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM("unreachable_call")),
        false,
        (ir_retval_t){0},
        0,
        NULL
    );
    ir_add_return0(IR_APPEND(dead));
    ir_insn_t *live_store = add_effect(exit, 43);
    ir_var_t  *loaded     = ir_var_create(func, IR_PRIM_i32, "volatile_value");
    ir_insn_t *volatile_load
        = ir_add_load(IR_APPEND(exit), IR_RETVAL_VAR(loaded), IR_MEMREF(IR_PRIM_i32, IR_BADDR_SYM("observable")));
    volatile_load->flags |= IR_INSN_FLAG_VOLATILE;
    ir_insn_t *live_call  = ir_add_call(
        IR_APPEND(exit),
        IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM("reachable_call")),
        false,
        (ir_retval_t){0},
        0,
        NULL
    );
    ir_insn_t  *ret        = ir_add_return0(IR_APPEND(exit));
    ir_insn_t  *dead_jump1 = ir_add_jump(IR_APPEND(exit), dead);
    char const *expected[] = {"entry", "exit"};

    TEST_ASSERT(has_insn(func, dead_jump0));
    TEST_ASSERT(has_insn(func, dead_jump1));
    TEST_ASSERT(has_insn(func, dead_store));
    TEST_ASSERT(has_insn(func, dead_call));
    TEST_EXPECT_INT(run_dead_code(func, expected, 2u, true), 0);
    TEST_ASSERT(has_insn(func, jump));
    TEST_ASSERT(has_insn(func, live_store));
    TEST_ASSERT(has_insn(func, volatile_load));
    TEST_ASSERT(has_insn(func, live_call));
    TEST_ASSERT(has_insn(func, ret));
    TEST_ASSERT(!has_insn(func, dead_jump0));
    TEST_ASSERT(!has_insn(func, dead_jump1));
    TEST_ASSERT(!has_insn(func, dead_store));
    TEST_ASSERT(!has_insn(func, dead_call));
    ir_func_delete(func);
    TEST_OK();
}

static int trivially_dead_control_flow(void) {
    ir_func_t *func       = ir_func_create("trivially_dead_control_flow", "entry", 0);
    ir_code_t *dispatch   = ir_code_create(func, "dispatch");
    ir_code_t *exit       = ir_code_create(func, "exit");
    ir_code_t *never0     = ir_code_create(func, "never_taken_entry");
    ir_code_t *never1     = ir_code_create(func, "never_taken_dispatch");
    ir_code_t *after_true = ir_code_create(func, "after_always_taken_branch");
    ir_code_t *after_jump = ir_code_create(func, "after_jump");
    ir_code_t *after_ret  = ir_code_create(func, "after_return");
    ir_code_t *dead_tail  = ir_code_create(func, "dead_tail");

    ir_insn_t *false0 = ir_add_branch(IR_APPEND(func->entry), IR_OPERAND_CONST(IR_CONST_FALSE), never0);
    ir_add_branch(IR_APPEND(func->entry), IR_OPERAND_CONST(IR_CONST_TRUE), dispatch);
    ir_insn_t *shadowed0 = ir_add_branch(IR_APPEND(func->entry), IR_OPERAND_CONST(IR_CONST_TRUE), after_true);
    ir_add_jump(IR_APPEND(func->entry), after_true);

    ir_insn_t *effect0 = add_effect(dispatch, 61);
    ir_insn_t *false1  = ir_add_branch(IR_APPEND(dispatch), IR_OPERAND_CONST(IR_CONST_FALSE), never1);
    ir_add_jump(IR_APPEND(dispatch), exit);
    ir_insn_t *shadowed1 = ir_add_branch(IR_APPEND(dispatch), IR_OPERAND_CONST(IR_CONST_TRUE), after_jump);

    ir_insn_t *effect1   = add_effect(exit, 67);
    ir_insn_t *ret       = ir_add_return0(IR_APPEND(exit));
    ir_insn_t *shadowed2 = ir_add_branch(IR_APPEND(exit), IR_OPERAND_CONST(IR_CONST_TRUE), after_ret);

    ir_code_t *dead_paths[] = {never0, never1, after_true, after_jump, after_ret};
    for (size_t i = 0; i < sizeof(dead_paths) / sizeof(*dead_paths); i++) {
        add_effect(dead_paths[i], 71 + i);
        ir_add_jump(IR_APPEND(dead_paths[i]), dead_tail);
    }
    ir_insn_t *dead_effect = add_effect(dead_tail, 79);
    ir_add_return0(IR_APPEND(dead_tail));
    char const *expected[] = {"entry", "dispatch", "exit"};

    // The independent walker evaluates literal conditions and stops at the
    // first unconditional transfer, regardless of the cached CFG edges.
    TEST_EXPECT_UINT(func->code_list.len, 9u);
    TEST_EXPECT_INT(run_dead_code(func, expected, 3u, true), 0);
    TEST_EXPECT_UINT(func->code_list.len, 3u);
    TEST_ASSERT(has_insn(func, effect0));
    TEST_ASSERT(has_insn(func, effect1));
    TEST_ASSERT(has_insn(func, ret));
    TEST_ASSERT(!has_insn(func, false0));
    TEST_ASSERT(!has_insn(func, false1));
    TEST_ASSERT(!has_insn(func, shadowed0));
    TEST_ASSERT(!has_insn(func, shadowed1));
    TEST_ASSERT(!has_insn(func, shadowed2));
    TEST_ASSERT(!has_insn(func, dead_effect));
    ir_func_delete(func);
    TEST_OK();
}

static int dead_combinator_predecessor(void) {
    ir_func_t *func         = ir_func_create("dead_combinator_predecessor", "entry", 0);
    func->rettype.type      = IR_FUNCRET_PRIM;
    func->rettype.prim_type = IR_PRIM_i32;
    ir_code_t *dead         = ir_code_create(func, "dead");
    ir_code_t *join         = ir_code_create(func, "join");
    ir_var_t  *result       = ir_var_create(func, IR_PRIM_i32, "result");
    ir_add_jump(IR_APPEND(func->entry), join);
    ir_add_jump(IR_APPEND(dead), join);
    ir_combinator_t *from  = lilycc_malloc(2 * sizeof(*from));
    from[0]                = (ir_combinator_t){.pred = func->entry, .bind = IR_OPERAND_CONST(IR_CONST_U32(53))};
    from[1]                = (ir_combinator_t){.pred = dead, .bind = IR_OPERAND_CONST(IR_CONST_U32(59))};
    ir_insn_t  *phi        = ir_add_combinator(IR_APPEND(join), result, 2, from);
    ir_insn_t  *ret        = ir_add_return1(IR_APPEND(join), IR_OPERAND_VAR(result));
    char const *expected[] = {"entry", "join"};

    TEST_EXPECT_UINT(phi->combinators_len, 2u);
    TEST_EXPECT_INT(run_dead_code(func, expected, 2u, true), 0);
    TEST_ASSERT(has_insn(func, ret));
    // Removing the dead predecessor also eliminates the single-binding phi.
    // Its instruction and result variable may have been deleted or replaced.
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        dlist_foreach_node(ir_insn_t, insn, &code->insns) {
            TEST_ASSERT(insn->type != IR_INSN_COMBINATOR);
        }
    }
    TEST_EXPECT_UINT(ret->operands_len, 1u);
    ir_operand_t value  = ret->operands[0];
    size_t       copies = 0;
    while (value.type == IR_OPERAND_TYPE_VAR) {
        TEST_ASSERT(copies++ < func->vars_list.len, "cyclic replacement copies");
        // Find the replacement definition in the instructions, without assigned_at.
        ir_insn_t *definition = NULL;
        dlist_foreach_node(ir_code_t, code, &func->code_list) {
            dlist_foreach_node(ir_insn_t, insn, &code->insns) {
                for (size_t i = 0; i < insn->returns_len; i++) {
                    if (insn->returns[i].type == IR_RETVAL_TYPE_VAR && insn->returns[i].dest_var == value.var) {
                        TEST_ASSERT(!definition, "multiple replacement definitions");
                        definition = insn;
                    }
                }
            }
        }
        TEST_ASSERT(definition, "missing replacement definition");
        TEST_EXPECT_INT(definition->type, IR_INSN_EXPR1);
        TEST_EXPECT_INT(definition->op1, IR_OP1_mov);
        TEST_EXPECT_UINT(definition->operands_len, 1u);
        TEST_EXPECT_INT(ir_operand_prim(value), IR_PRIM_i32);
        value = definition->operands[0];
    }
    TEST_EXPECT_INT(value.type, IR_OPERAND_TYPE_CONST);
    TEST_EXPECT_INT(value.iconst.prim_type, IR_PRIM_i32);
    TEST_EXPECT_UINT(value.iconst.constl, 53u);
    ir_func_delete(func);
    TEST_OK();
}

int main(void) {
    int result  = 0;
    result     |= disconnected_paths();
    result     |= unknown_branch_and_loop();
    result     |= instructions_after_transfers();
    result     |= trivially_dead_control_flow();
    result     |= dead_combinator_predecessor();
    return result;
}
