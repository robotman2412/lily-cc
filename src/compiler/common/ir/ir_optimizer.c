
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir/ir_optimizer.h"

#include "ir/ir_interpreter.h"



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
static bool strength_reduce_expr(ir_expr_t *expr) {
    if (expr->type != IR_EXPR_BINARY) {
        // The strength-reduced expressions are all binary.
        return false;
    }

#define prim expr->dest->prim_type
#define lhs  expr->e_binary.lhs
#define rhs  expr->e_binary.rhs
#define oper expr->e_binary.oper

    if (prim == IR_PRIM_f32 || prim == IR_PRIM_f64) {
        return false;
    }

    if (oper == IR_OP2_mul && lhs.is_const && !rhs.is_const) {
        ir_operand_t tmp = lhs;
        lhs              = rhs;
        rhs              = tmp;
    }
    if (!rhs.is_const) {
        return false;
    }
    rhs.iconst = ir_trim_const(rhs.iconst);

    if (oper == IR_OP2_div && ir_const_popcnt(rhs.iconst) == 1 && !ir_const_is_negative(rhs.iconst)) {
        // Replace a division with a right shift.
        rhs.iconst.constl = ir_const_ctz(rhs.iconst);
        rhs.iconst.consth = 0;
        oper              = IR_OP2_shr;
        return true;

    } else if (oper == IR_OP2_rem && ir_const_popcnt(rhs.iconst) == 1 && !ir_const_is_negative(rhs.iconst)) {
        // Replace a remainder with a bitmask.
        int    bits = ir_const_ctz(rhs.iconst);
        i128_t mask = add128(int128(-1, -1), shl128(int128(0, 1), bits));
        if (!(prim & 1)) {
            // TODO: Signed requires a bit more instructions
            // and Lily-CC doesn't support inserting them in the middle of code yet.
            return false;
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
            loop    |= strength_reduce_expr(container_of(var->assigned_at.head, ir_expr_t, dest_node));
            reduced |= loop;
            var      = next;
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
        } else if (insn->type == IR_INSN_FLOW) {
            ir_flow_t *flow = (void *)insn;
            if (flow->type == IR_FLOW_JUMP) {
                // If this is a jump, all following instructions will be dead.
                dead     = true;
                changed |= dead_code_dfs(flow->f_jump.target);
            } else if (flow->type == IR_FLOW_RETURN) {
                // If this is a return, all following instructions will be dead.
                dead = true;
            } else if (flow->type == IR_FLOW_BRANCH) {
                if (flow->f_branch.cond.is_const) {
                    if (flow->f_branch.cond.iconst.constl & 1) {
                        // If this is a branch with constant condition true, all following instructions will be dead.
                        dead     = true;
                        changed |= dead_code_dfs(flow->f_branch.target);
                    } else {
                        // If this is a branch with constant condition false, delete it.
                        ir_insn_delete(insn);
                    }
                } else {
                    // Check the potential branch target.
                    changed |= dead_code_dfs(flow->f_branch.target);
                }
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
static bool const_prop_expr(ir_expr_t *expr) {
    if (expr->type == IR_EXPR_UNARY && expr->e_unary.value.is_const) {
        // Calculate unary expression at compile time.
        ir_const_t iconst;
        if (expr->e_unary.oper == IR_OP1_mov) {
            iconst = ir_cast(expr->dest->prim_type, expr->e_unary.value.iconst);
        } else {
            iconst = ir_calc1(expr->e_unary.oper, expr->e_unary.value.iconst);
        }
        ir_var_replace(expr->dest, (ir_operand_t){.is_const = true, .iconst = iconst});
        ir_var_delete(expr->dest);
        return true;

    } else if (expr->type == IR_EXPR_BINARY && expr->e_binary.lhs.is_const && expr->e_binary.rhs.is_const) {
        // Calculate binary expression at compile time.
        ir_const_t iconst = ir_calc2(expr->e_binary.oper, expr->e_binary.lhs.iconst, expr->e_binary.rhs.iconst);
        ir_var_replace(expr->dest, (ir_operand_t){.is_const = true, .iconst = iconst});
        ir_var_delete(expr->dest);
        return true;

    } else if (expr->type == IR_EXPR_UNARY && !expr->e_unary.value.is_const
               && expr->dest->prim_type == expr->e_unary.value.var->prim_type && expr->e_unary.oper == IR_OP1_mov) {
        // Move between two variables of the same type; replace the destination.
        ir_var_replace(expr->dest, (ir_operand_t){.is_const = false, .var = expr->e_unary.value.var});
        ir_var_delete(expr->dest);
        return true;

    } else if (expr->type == IR_EXPR_BINARY && expr->e_binary.oper == IR_OP2_mul && expr->e_binary.rhs.is_const
               && expr->e_binary.rhs.iconst.consth == 0 && expr->e_binary.rhs.iconst.constl == 0) {
        // Multiply by zero (rhs version); replace with constant zero.
        ir_var_replace(
            expr->dest,
            (ir_operand_t){.is_const = true, .iconst = {.prim_type = expr->dest->prim_type, .constl = 0, .consth = 0}}
        );
        ir_var_delete(expr->dest);
        return true;

    } else if (expr->type == IR_EXPR_BINARY && expr->e_binary.oper == IR_OP2_mul && expr->e_binary.lhs.is_const
               && expr->e_binary.lhs.iconst.consth == 0 && expr->e_binary.lhs.iconst.constl == 0) {
        // Multiply by zero (lhs version); replace with constant zero.
        ir_var_replace(
            expr->dest,
            (ir_operand_t){.is_const = true, .iconst = {.prim_type = expr->dest->prim_type, .constl = 0, .consth = 0}}
        );
        ir_var_delete(expr->dest);
        return true;

    } else if (expr->type == IR_EXPR_BINARY && (expr->e_binary.oper == IR_OP2_mul || expr->e_binary.oper == IR_OP2_div)
               && expr->e_binary.rhs.is_const && expr->e_binary.rhs.iconst.consth == 0
               && expr->e_binary.rhs.iconst.constl == 1) {
        // Multiply / divide by one (rhs version); replace with variable.
        ir_var_replace(expr->dest, expr->e_binary.lhs);
        ir_var_delete(expr->dest);
        return true;

    } else if (expr->type == IR_EXPR_BINARY && expr->e_binary.oper == IR_OP2_mul && expr->e_binary.lhs.is_const
               && expr->e_binary.lhs.iconst.consth == 0 && expr->e_binary.lhs.iconst.constl == 1) {
        // Multiply by one (lhs version); replace with variable.
        ir_var_replace(expr->dest, expr->e_binary.rhs);
        ir_var_delete(expr->dest);
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
            loop       |= const_prop_expr(container_of(var->assigned_at.head, ir_expr_t, dest_node));
            propagated |= loop;
            var         = next;
        }
    } while (loop);
    return propagated;
}



// Combine two code blocks end-to-end.
static void merge_code(ir_code_t *first, ir_code_t *second) {
    // The very last instruction should be the one and only jump.
    ir_insn_delete(container_of(first->insns.tail, ir_insn_t, node));

    // Transfer all instructions from second to first.
    dlist_foreach_node(ir_insn_t, insn, &second->insns) {
        insn->parent = first;
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
        if (succ->pred.len == 1) {
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
