
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir/ir_optimizer.h"

#include "assert.h"
#include "ir.h"
#include "ir/ir_interpreter.h"
#include "ir_types.h"
#include "set.h"



// Run optimizations on some IR.
// Returns whether any code was changed.
bool ir_optimize(ir_func_t *func) {
    bool changed = false, loop;

    // Optimization passes that affect each other.
    do {
        loop     = false;
        loop    |= opt_const_prop(func);
        loop    |= opt_unused_vars(func);
        loop    |= opt_dead_code(func);
        loop    |= opt_branches(func);
        changed |= loop;
    } while (loop);

    // Standalone optimization passes.
    changed |= opt_strength_reduce(func);

    return changed;
}



// Try to strength-reduce a single expression.
static bool strength_reduce_expr(ir_insn_t *expr) {
    if (expr->type != IR_INSN_EXPR2) {
        // The strength-reduced expressions are all binary exprs.
        return false;
    }

#define prim expr->returns[0].dest_var->prim_type
#define lhs  expr->operands[0]
#define rhs  expr->operands[1]
#define oper expr->op2

    if (prim == IR_PRIM_f32 || prim == IR_PRIM_f64) {
        return false;
    }

    if (oper == IR_OP2_mul && lhs.type == IR_OPERAND_TYPE_CONST && rhs.type != IR_OPERAND_TYPE_CONST) {
        ir_operand_t tmp = lhs;
        lhs              = rhs;
        rhs              = tmp;
    }
    if (rhs.type != IR_OPERAND_TYPE_CONST) {
        return false;
    }
    rhs.iconst       = ir_trim_const(rhs.iconst);
    ir_const_t p_rhs = ir_const_is_negative(rhs.iconst) ? ir_calc1(IR_OP1_neg, rhs.iconst) : rhs.iconst;

    if (oper == IR_OP2_div && ir_const_popcnt(rhs.iconst) == 1 && !ir_const_is_negative(rhs.iconst)) {
        // Replace a division with a right shift.
        rhs.iconst.constl = ir_const_ctz(rhs.iconst);
        rhs.iconst.consth = 0;
        oper              = IR_OP2_shr;
        return true;

    } else if (oper == IR_OP2_rem && ir_const_popcnt(p_rhs) == 1) {
        // Replace a remainder with a bitmask.
        int    bits = ir_const_ctz(p_rhs);
        i128_t mask = add128(int128(-1, -1), shl128(int128(0, 1), bits));
        if (!(prim & 1)) {
            // Add instructions to extract and copy the sign from the lhs to the output.
            ir_var_t *tmp1 = ir_var_create(expr->code->func, prim, NULL);
            ir_var_t *tmp2 = ir_var_create(expr->code->func, prim, NULL);
            ir_var_t *tmp3 = ir_var_create(expr->code->func, prim, NULL);
            ir_var_t *tmp4 = ir_var_create(expr->code->func, prim, NULL);

            ir_const_t shamt1 = {.prim_type = prim, .constl = ir_prim_sizes[prim] * 8 - 1};
            ir_add_expr2(IR_BEFORE_INSN(expr), tmp1, IR_OP2_shr, lhs, IR_OPERAND_CONST(shamt1));

            ir_const_t shamt2 = {.prim_type = prim, .constl = ir_prim_sizes[prim] * 8 - bits};
            ir_add_expr2(IR_BEFORE_INSN(expr), tmp2, IR_OP2_shr, IR_OPERAND_VAR(tmp1), IR_OPERAND_CONST(shamt2));

            ir_add_expr2(IR_BEFORE_INSN(expr), tmp3, IR_OP2_add, lhs, IR_OPERAND_VAR(tmp2));

            ir_var_t *dest = expr->returns[0].dest_var;
            set_remove(&dest->assigned_at, expr);
            expr->returns[0].dest_var = tmp4;
            set_add(&tmp4->assigned_at, expr);

            ir_add_expr2(IR_AFTER_INSN(expr), dest, IR_OP2_sub, IR_OPERAND_VAR(tmp4), IR_OPERAND_VAR(tmp2));
        }
        rhs.iconst.const128 = mask;
        oper                = IR_OP2_band;
        return true;

    } else if (oper == IR_OP2_mul && ir_const_popcnt(rhs.iconst) == 1 && !ir_const_is_negative(rhs.iconst)) {
        // Replace multiplication with a left shift.
        rhs.iconst.constl = ir_const_ctz(rhs.iconst);
        rhs.iconst.consth = 0;
        oper              = IR_OP2_shl;
        return true;
    }

#undef prim
#undef lhs
#undef rhs
#undef oper

    return false;
}

// Optimization: Strength reduction; replaces expensive with cheaper arithmetic where available.
bool opt_strength_reduce(ir_func_t *func) {
    bool reduced = false, loop;
    do {
        loop          = false;
        ir_var_t *var = container_of(func->vars_list.head, ir_var_t, node);
        while (var) {
            ir_var_t *next = container_of(var->node.next, ir_var_t, node);
            if (var->assigned_at.len != 1) {
                var = next;
                continue;
            }
            set_ent_t const *ent  = set_next(&var->assigned_at, NULL);
            loop                 |= strength_reduce_expr(ent->value);
            reduced              |= loop;
            var                   = next;
        }
    } while (loop);
    return reduced;
}

// Optimization: Delete all variables and assignments to them whose value is never read.
// Returns whether any variables were deleted.
bool opt_unused_vars(ir_func_t *func) {
    bool deleted = false, loop;
    do {
        loop          = false;
        ir_var_t *var = container_of(func->vars_list.head, ir_var_t, node);
        while (var) {
            ir_var_t *next = container_of(var->node.next, ir_var_t, node);
            if (!var->used_at.len) {
                ir_var_delete(var);
                deleted = true;
                loop    = true;
            }
            var = next;
        }
    } while (loop);
    return deleted;
}



// Dead code optimization depth-first search.
// Returns whether any code was changed or removed.
static bool dead_code_dfs(ir_code_t *code) {
    if (code->visited) {
        return false;
    }
    code->visited = true;

    // Walk instructions in this code block.
    bool       dead = false, changed = false;
    ir_insn_t *insn = container_of(code->insns.head, ir_insn_t, node);
    while (insn) {
        ir_insn_t *next = container_of(insn->node.next, ir_insn_t, node);
        if (dead) {
            // If we're in dead code, delete all instructions.
            changed = true;
            ir_insn_delete(insn);
        } else if (insn->type == IR_INSN_JUMP) {
            // If this is a jump, all following instructions will be dead.
            dead     = true;
            changed |= dead_code_dfs(insn->operands[0].mem.base_code);
        } else if (insn->type == IR_INSN_RETURN) {
            // If this is a return, all following instructions will be dead.
            dead = true;
        } else if (insn->type == IR_INSN_BRANCH) {
            if (insn->operands[1].type == IR_OPERAND_TYPE_CONST) {
                if (insn->operands[1].iconst.constl & 1) {
                    // If this is a branch with constant condition true, all following instructions will be dead.
                    dead     = true;
                    changed |= dead_code_dfs(insn->operands[0].mem.base_code);
                } else {
                    // If this is a branch with constant condition false, delete it.
                    ir_insn_delete(insn);
                }
            } else {
                // Check the potential branch target.
                changed |= dead_code_dfs(insn->operands[0].mem.base_code);
            }
        }

        insn = next;
    }

    return changed;
}

// Optimization: Delete code from dead paths.
// Returns whether any code was changed or removed.
bool opt_dead_code(ir_func_t *func) {
    bool changed = false, loop;
    do {
        // Mark code as not reachable.
        dlist_foreach_node(ir_code_t, code, &func->code_list) {
            code->visited = false;
        }

        // Walk the code for reachability.
        loop = dead_code_dfs(container_of(func->code_list.head, ir_code_t, node));

        // Delete all unreachable code blocks.
        ir_code_t *code = container_of(func->code_list.head, ir_code_t, node);
        while (code) {
            ir_code_t *next = container_of(code->node.next, ir_code_t, node);
            if (!code->visited) {
                ir_code_delete(code);
            }
            code = next;
        }

        // Recalculate predecessors and successors of the remaining graph.
        ir_func_recalc_flow(func);
        changed |= loop;
    } while (loop);
    return changed;
}



// Try to constant-propagate a single expression.
static bool const_prop_expr(ir_insn_t *expr) {
    if (expr->type == IR_INSN_COMBINATOR) {
        // Flatten phi-nodes with only a single predecessor or all identical bindings.
        for (size_t i = 1; i < expr->combinators_len; i++) {
            if (!ir_operand_identical(expr->combinators[0].bind, expr->combinators[i].bind)) {
                return false;
            }
        }
        ir_var_replace(expr->returns[0].dest_var, expr->combinators[0].bind);
        ir_var_delete(expr->returns[0].dest_var);
        return true;

    } else if (expr->type == IR_INSN_EXPR1 && expr->operands[0].type == IR_OPERAND_TYPE_CONST) {
        // Calculate unary expression at compile time.
        ir_const_t iconst;
        if (expr->op1 == IR_OP1_mov) {
            iconst = ir_cast(expr->returns[0].dest_var->prim_type, expr->operands[0].iconst);
        } else {
            iconst = ir_calc1(expr->op1, expr->operands[0].iconst);
        }
        ir_var_replace(expr->returns[0].dest_var, IR_OPERAND_CONST(iconst));
        ir_var_delete(expr->returns[0].dest_var);
        return true;

    } else if (expr->type == IR_INSN_EXPR2 && expr->operands[0].type == IR_OPERAND_TYPE_CONST
               && expr->operands[1].type == IR_OPERAND_TYPE_CONST) {
        // Calculate binary expression at compile time.
        ir_const_t iconst = ir_calc2(expr->op2, expr->operands[0].iconst, expr->operands[1].iconst);
        ir_var_replace(expr->returns[0].dest_var, IR_OPERAND_CONST(iconst));
        ir_var_delete(expr->returns[0].dest_var);
        return true;

    } else if (expr->type == IR_INSN_EXPR1 && expr->op1 == IR_OP1_mov && expr->operands[0].type == IR_OPERAND_TYPE_VAR
               && expr->returns[0].dest_var->prim_type == expr->operands[0].var->prim_type) {
        // Move between two variables of the same type; replace the destination.
        ir_var_replace(expr->returns[0].dest_var, IR_OPERAND_VAR(expr->operands[0].var));
        ir_var_delete(expr->returns[0].dest_var);
        return true;

    } else if (expr->type == IR_INSN_EXPR2 && expr->op2 == IR_OP2_mul
               && ((expr->operands[1].type == IR_OPERAND_TYPE_CONST && expr->operands[1].iconst.consth == 0
                    && expr->operands[1].iconst.constl == 0)
                   || (expr->operands[0].type == IR_OPERAND_TYPE_CONST && expr->operands[0].iconst.consth == 0
                       && expr->operands[0].iconst.constl == 0))) {
        // Multiply by zero; replace with constant zero.
        ir_var_replace(
            expr->returns[0].dest_var,
            (ir_operand_t){
                .type   = IR_OPERAND_TYPE_CONST,
                .iconst = {
                    .prim_type = expr->returns[0].dest_var->prim_type,
                    .constl    = 0,
                    .consth    = 0,
                },
            }
        );
        ir_var_delete(expr->returns[0].dest_var);
        return true;

    } else if (expr->type == IR_INSN_EXPR2 && (expr->op2 == IR_OP2_mul || expr->op2 == IR_OP2_div)
               && expr->operands[1].type == IR_OPERAND_TYPE_CONST && expr->operands[1].iconst.consth == 0
               && expr->operands[1].iconst.constl == 1) {
        // Multiply / divide by one (rhs version); replace with variable.
        ir_var_replace(expr->returns[0].dest_var, expr->operands[0]);
        ir_var_delete(expr->returns[0].dest_var);
        return true;

    } else if (expr->type == IR_INSN_EXPR2 && expr->op2 == IR_OP2_mul && expr->operands[0].type == IR_OPERAND_TYPE_CONST
               && expr->operands[0].iconst.consth == 0 && expr->operands[0].iconst.constl == 1) {
        // Multiply by one (lhs version); replace with variable.
        ir_var_replace(expr->returns[0].dest_var, expr->operands[1]);
        ir_var_delete(expr->returns[0].dest_var);
        return true;

    } else {
        return false;
    }
}

// Optimization: Propagate constants and useless copies.
// Returns whether any code was changed.
bool opt_const_prop(ir_func_t *func) {
    bool propagated = false, loop;
    do {
        loop          = false;
        ir_var_t *var = container_of(func->vars_list.head, ir_var_t, node);
        while (var) {
            ir_var_t *next = container_of(var->node.next, ir_var_t, node);
            if (var->assigned_at.len != 1) {
                var = next;
                continue;
            }
            set_ent_t const *ent  = set_next(&var->assigned_at, NULL);
            loop                 |= const_prop_expr(ent->value);
            propagated           |= loop;
            var                   = next;
        }
    } while (loop);
    return propagated;
}



// Combine two code blocks end-to-end.
static void merge_code(ir_code_t *first, ir_code_t *second) {
    // The very last instruction should be the one and only jump.
    ir_insn_t *last_jmp = container_of(first->insns.tail, ir_insn_t, node);
    assert(last_jmp);
    assert(last_jmp->type == IR_INSN_JUMP || last_jmp->type == IR_INSN_BRANCH);
    if (last_jmp->type == IR_INSN_JUMP) {
        assert(last_jmp->operands[0].mem.base_code == second);
    } else {
        assert(last_jmp->operands[0].mem.base_code == second);
        assert(last_jmp->operands[1].type == IR_OPERAND_TYPE_CONST);
        assert(last_jmp->operands[1].iconst.prim_type == IR_PRIM_bool);
        assert(last_jmp->operands[1].iconst.constl & 1);
    }
    ir_insn_delete((ir_insn_t *)last_jmp);

    // Transfer all instructions from second to first.
    dlist_foreach_node(ir_insn_t, insn, &second->insns) {
        insn->code = first;
    }
    dlist_concat(&first->insns, &second->insns);

    // Update predecessor-successor relations.
    set_foreach(ir_code_t, succ, &second->succ) {
        set_remove(&succ->pred, second);
        set_add(&succ->pred, first);
    }
    set_clear(&first->succ);
    set_clear(&second->pred);
    first->succ  = second->succ;
    second->succ = PTR_SET_EMPTY;

    // Delete the now empty second node.
    ir_code_delete(second);
}

// Depth-first search function that optimizes branches.
// Returns whether any code was changed.
static bool branch_opt_dfs(ir_code_t *code) {
    if (code->visited) {
        return false;
    }
    code->visited = true;

    bool changed = false;
    while (code->succ.len == 1) {
        ir_code_t *succ = set_next(&code->succ, NULL)->value;
        if (succ != code && succ->pred.len == 1) {
            // If this is a 1:1 link, combine into one block.
            merge_code(code, succ);
            changed = true;
        } else {
            // Cannot merge here.
            break;
        }
    }

    set_foreach(ir_code_t, succ, &code->succ) {
        // Recursively check successors.
        changed |= branch_opt_dfs(succ);
    }

    return changed;
}

// Optimization: Remove redundant branches.
// Returns whether any code was changed.
bool opt_branches(ir_func_t *func) {
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        code->visited = false;
    }
    return branch_opt_dfs(container_of(func->code_list.head, ir_code_t, node));
}
