
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "opt.h"



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



// Optimization: Delete code from dead paths.
// Returns whether any code was changed or removed.
bool opt_dead_code(ir_func_t *func);



// Try to constant-propagate a single expression.
static bool const_prop_expr(ir_expr_t *expr) {
    if (expr->type == IR_EXPR_UNARY && expr->e_unary.value.is_const) {
        return true;
    } else {
        return false;
    }
}

// Optimization: Propagate constants.
// Returns whether any code was changed.
bool opt_const_prop(ir_func_t *func) {
    bool propagated = false, loop;
    do {
        loop = false;
        dlist_foreach_node(ir_var_t, var, &func->vars_list) {
            if (var->assigned_at.len != 1) {
                continue;
            }
            propagated |= const_prop_expr(container_of(var->assigned_at.head, ir_expr_t, dest_node));
            loop        = true;
        }
    } while (loop);
    return propagated;
}
