
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir.h"

#include "arrays.h"
#include "strong_malloc.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>



// Create a new IR function.
// Function argument types are IR_PRIM_s32 by default.
ir_func_t *ir_func_create(char const *name, char const *entry_name, size_t args_len, char const *const *args_name) {
    ir_func_t *func = strong_calloc(1, sizeof(ir_func_t));
    func->name      = strong_strdup(name);
    func->args      = strong_calloc(1, sizeof(ir_var_t *) * args_len);
    func->args_len  = args_len;
    for (size_t i = 0; i < args_len; i++) {
        func->args[i] = ir_var_create(func, IR_PRIM_s32, args_name ? args_name[i] : NULL);
    }
    func->entry = ir_code_create(func, entry_name);
    return func;
}

// Delete an IR function.
void ir_func_destroy(ir_func_t *func) {
    while (func->code_list.len) {
        ir_code_t *code = (ir_code_t *)func->code_list.head;
        ir_code_delete(code);
    }

    while (func->vars_list.len) {
        ir_var_t *var = (ir_var_t *)func->vars_list.head;
        ir_var_delete(var);
    }

    while (func->frames_list.len) {
        ir_frame_t *frame = (ir_frame_t *)dlist_pop_front(&func->frames_list);
        free(frame->name);
        free(frame);
    }

    free(func->args);
    free(func->name);
    free(func);
}


// Extra temporary data used while building dominance tree.
typedef struct {
    // Pointer to the code that this concerns.
    ir_code_t *code;
    // Parent in the depth-first search tree.
    size_t     parent;
    // ?
    size_t     ancestor;
    // Semidominator.
    size_t     semi;
    // Immediate dominator.
    size_t     idom;
    // Best link.
    size_t     best;
    // Set of nodes whose semidominator this is.
    set_t      bucket;
    // Frontier where this node's dominance ends.
    set_t      frontier;
    // Whether this node uses the variable being analyzed.
    bool       uses_var;
} dom_node_t;

// Depth-first search function.
static void dom_node_dfs(ir_code_t *code, dom_node_t *nodes, ir_func_t *func, size_t *ctr, size_t parent) {
    if (code->visited) {
        return;
    }
    code->visited      = true;
    code->dfs_index    = *ctr;
    nodes[*ctr].code   = code;
    nodes[*ctr].parent = parent;
    parent             = *ctr;
    ++*ctr;
    set_foreach(ir_code_t, succ, &code->succ) {
        dom_node_dfs(succ, nodes, func, ctr, parent);
    }
}

// Compression function for the algorithm of Lengauer and Tarjan.
static void dom_node_compress(dom_node_t *nodes, size_t v) {
    size_t a = nodes[v].ancestor;

    if (a == (size_t)-1) {
        return;
    }

    dom_node_compress(nodes, a);

    if (nodes[nodes[v].best].semi > nodes[nodes[a].best].semi) {
        nodes[v].best = nodes[a].best;
    }

    nodes[v].ancestor = nodes[a].ancestor;
}

// Evaluation function for the algorithm of Lengauer and Tarjan.
static size_t dom_node_eval(dom_node_t *nodes, size_t v) {
    if (nodes[v].ancestor == (size_t)-1) {
        return v;
    } else {
        dom_node_compress(nodes, v);
        return nodes[v].best;
    }
}

// Compute the dominance relations between nodes.
static void compute_dominance(ir_func_t *func, size_t nodes_len, dom_node_t *nodes) {
    // Clear visited flag.
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        code->visited = false;
    }

    // Do a depth-first search.
    for (size_t i = 0; i < nodes_len; i++) {
        nodes[i].semi     = i;
        nodes[i].best     = i;
        nodes[i].bucket   = PTR_SET_EMPTY;
        nodes[i].ancestor = -1;
        nodes[i].frontier = PTR_SET_EMPTY;
    }
    {
        size_t ctr = 0;
        dom_node_dfs(container_of(func->code_list.head, ir_code_t, node), nodes, func, &ctr, -1);
    }

    for (size_t w = nodes_len - 1; w >= 1; w--) {
        size_t p = nodes[w].parent;

        set_foreach(ir_code_t, pred, &nodes[w].code->pred) {
            size_t v = pred->dfs_index;
            size_t u = dom_node_eval(nodes, v);
            if (nodes[w].semi > nodes[u].semi) {
                nodes[w].semi = nodes[u].semi;
            }
            set_add(&nodes[nodes[w].semi].bucket, (void *)w);
            // Called link in the algorithm:
            nodes[w].ancestor = p;
        }

        set_foreach(void, _v, &nodes[p].bucket) {
            size_t v      = (size_t)_v;
            size_t u      = dom_node_eval(nodes, v);
            nodes[v].idom = nodes[u].semi < nodes[v].semi ? u : nodes[w].parent;
        }
    }

    for (size_t w = 1; w < nodes_len; w++) {
        if (nodes[w].idom != nodes[w].semi) {
            nodes[w].idom = nodes[nodes[w].idom].idom;
        }
    }
    nodes[0].idom = -1;

    // Compute dominance frontiers.
    for (size_t i = 1; i < nodes_len; i++) {
        if (nodes[i].code->pred.len < 2) {
            continue;
        }
        set_foreach(ir_code_t, code, &nodes[i].code->pred) {
            size_t runner = code->dfs_index;
            while (runner != nodes[i].idom) {
                set_add(&nodes[runner].frontier, (void *)i);
                runner = nodes[runner].idom;
            }
        }
    }
}

// Insert a combinator function for `var` into the beginning of `code`.
static void create_combinator(ir_code_t *code, ir_var_t *dest) {
    ir_expr_t *expr             = calloc(1, sizeof(ir_expr_t));
    expr->base.type             = IR_INSN_EXPR;
    expr->base.parent           = code;
    expr->type                  = IR_EXPR_COMBINATOR;
    expr->e_combinator.from_len = code->pred.len;
    expr->e_combinator.from     = strong_calloc(1, code->pred.len * sizeof(ir_combinator_t));
    expr->dest                  = dest;
    size_t i                    = 0;
    set_foreach(ir_code_t, pred, &code->pred) {
        // TODO: Some way to mark as undefined?
        expr->e_combinator.from[i++] = (ir_combinator_t){
            .bind = {
                .is_const = true,
                .iconst   = {
                    .prim_type = dest->prim_type,
                    .constl    = 0,
                    .consth    = 0,
                },
            },
            .prev = pred,
        };
    }
    dlist_append(&dest->assigned_at, &expr->dest_node);
    dlist_prepend(&code->insns, &expr->base.node);
}

// Search successor nodes depth-first looking for variable usage.
static bool var_usage_dfs(ir_code_t *code, dom_node_t *nodes) {
    if (code->visited) {
        return nodes[code->dfs_index].uses_var;
    }
    code->visited = true;

    bool uses_var = nodes[code->dfs_index].uses_var;
    set_foreach(ir_code_t, succ, &code->succ) {
        uses_var |= var_usage_dfs(succ, nodes);
    }

    nodes[code->dfs_index].uses_var = uses_var;
    return uses_var;
}

// Insert combinator functions.
static void insert_combinators(ir_func_t *func, ir_var_t *var, size_t nodes_len, dom_node_t *nodes) {
    // Nodes at which a phi function is to be inserted.
    set_t frontier = PTR_SET_EMPTY;

    // Mark as not checked for variable usage.
    for (size_t i = 0; i < nodes_len; i++) {
        nodes[i].uses_var = false;
    }
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        code->visited = false;
    }
    set_foreach(ir_insn_t, insn, &var->used_at) {
        nodes[insn->parent->dfs_index].uses_var = true;
    }
    dlist_foreach(ir_expr_t, expr, dest_node, &var->assigned_at) {
        // The search starts at each definition so that anything before won't be marked as using it.
        // Even though it can be assigned both by memory insns and exprs, this is okay to do.
        nodes[expr->base.parent->dfs_index].uses_var = true;
        var_usage_dfs(expr->base.parent, nodes);
    }

    // Mark as not having a combinator function.
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        code->visited = false;
    }
    dlist_foreach(ir_expr_t, expr, dest_node, &var->assigned_at) {
        // Same thing; could actually be a mem insn but this works for that too.
        set_addall(&frontier, &nodes[expr->base.parent->dfs_index].frontier);
    }

    bool changed = true;
    while (changed) {
        changed = false;
        set_foreach(void, _index, &frontier) {
            size_t     index = (size_t)_index;
            ir_code_t *code  = nodes[index].code;
            if (code->visited || !nodes[index].uses_var) {
                continue;
            }
            code->visited = true;
            create_combinator(code, var);
            set_addall(&frontier, &nodes[index].frontier);
        }
    }

    set_clear(&frontier);
}

// Replace variables in an instruction unless it is a phi instruction.
static void replace_insn_var(ir_insn_t *insn, ir_var_t *from, ir_var_t *to) {
    set_remove(&from->used_at, insn);
    if (insn->type == IR_INSN_EXPR) {
        ir_expr_t *expr = (void *)insn;
        if (expr->type == IR_EXPR_BINARY) {
            if (!expr->e_binary.lhs.is_const && expr->e_binary.lhs.var == from) {
                set_add(&to->used_at, insn);
                expr->e_binary.lhs.var = to;
            }
            if (!expr->e_binary.rhs.is_const && expr->e_binary.rhs.var == from) {
                set_add(&to->used_at, insn);
                expr->e_binary.rhs.var = to;
            }
        } else if (expr->type == IR_EXPR_UNARY) {
            if (!expr->e_unary.value.is_const && expr->e_unary.value.var == from) {
                set_add(&to->used_at, insn);
                expr->e_unary.value.var = to;
            }
        } else if (expr->type == IR_EXPR_COMBINATOR || expr->type == IR_EXPR_UNDEFINED) {
            // Nothing to do.
        } else {
            abort();
        }
    } else if (insn->type == IR_INSN_MEM) {
        ir_mem_t *mem = (void *)insn;
        if (mem->type == IR_MEM_LOAD) {
            if (!mem->m_load.addr.is_const && mem->m_load.addr.var == from) {
                set_add(&to->used_at, insn);
                mem->m_load.addr.var = to;
            }
        } else if (mem->type == IR_MEM_STORE) {
            if (!mem->m_store.addr.is_const && mem->m_store.addr.var == from) {
                set_add(&to->used_at, insn);
                mem->m_store.addr.var = to;
            }
            if (!mem->m_store.src.is_const && mem->m_store.src.var == from) {
                set_add(&to->used_at, insn);
                mem->m_store.src.var = to;
            }
        } else if (mem->type == IR_MEM_LEA_STACK || mem->type == IR_MEM_LEA_SYMBOL) {
            // Nothing to do.
        } else {
            abort();
        }
    } else if (insn->type == IR_INSN_FLOW) {
        ir_flow_t *flow = (void *)insn;
        if (flow->type == IR_FLOW_BRANCH) {
            if (!flow->f_branch.cond.is_const && flow->f_branch.cond.var == from) {
                set_add(&to->used_at, insn);
                flow->f_branch.cond.var = to;
            }
        } else if (flow->type == IR_FLOW_RETURN) {
            if (flow->f_return.has_value && !flow->f_return.value.is_const && flow->f_return.value.var == from) {
                set_add(&to->used_at, insn);
                flow->f_return.value.var = to;
            }
        } else if (flow->type == IR_FLOW_CALL_DIRECT || flow->type == IR_FLOW_CALL_PTR) {
            if (flow->type == IR_FLOW_CALL_PTR) {
                if (!flow->f_call_ptr.addr.is_const && flow->f_call_ptr.addr.var == from) {
                    set_add(&to->used_at, insn);
                    flow->f_call_ptr.addr.var = to;
                }
            }
            for (size_t i = 0; i < flow->f_call_direct.args_len; i++) {
                if (!flow->f_call_direct.args[i].is_const && flow->f_call_direct.args[i].var == from) {
                    set_add(&to->used_at, insn);
                    flow->f_call_direct.args[i].var = to;
                }
            }
        } else if (flow->type == IR_FLOW_JUMP) {
            // Nothing to do.
        } else {
            abort();
        }
    } else {
        abort();
    }
}

// Replace variables in the phi instructions.
static void replace_phi_vars(ir_code_t *pred, ir_code_t *code, set_t *from, ir_var_t *to) {
    dlist_foreach_node(ir_insn_t, insn, &code->insns) {
        if (insn->type != IR_INSN_EXPR) {
            return;
        }
        ir_expr_t *expr = (void *)insn;
        if (expr->type != IR_EXPR_COMBINATOR) {
            return;
        } else if (set_contains(from, expr->dest)) {
            for (size_t i = 0; i < expr->e_combinator.from_len; i++) {
                if (expr->e_combinator.from[i].prev == pred && !expr->e_combinator.from[i].bind.is_const) {
                    set_add(&to->used_at, expr);
                    expr->e_combinator.from[i].bind.is_const = false;
                    expr->e_combinator.from[i].bind.var      = to;
                }
            }
            return;
        }
    }
}

// Rename variables assigned more than once.
static void rename_assignments(ir_func_t *func, ir_code_t *code, ir_var_t *from, ir_var_t *to, set_t *phi_from) {
    if (code->visited) {
        return;
    }
    code->visited = true;

    dlist_foreach_node(ir_insn_t, insn, &code->insns) {
        if (to) {
            replace_insn_var(insn, from, to);
        }
        if (insn->type == IR_INSN_EXPR) {
            ir_expr_t *expr = (void *)insn;
            if (expr->dest == from) {
                // If the instruction assigns the variable, rename the assignment.
                dlist_remove(&from->assigned_at, &expr->dest_node);
                to         = ir_var_create(func, from->prim_type, NULL);
                expr->dest = to;
                dlist_append(&to->assigned_at, &expr->dest_node);
                if (expr->type == IR_EXPR_COMBINATOR) {
                    set_add(phi_from, to);
                }
            }
        } else if (insn->type == IR_INSN_MEM) {
            ir_mem_t *mem = (void *)insn;
            // Note: The `dest` for `lea` and `load` are in the same place.
            if (mem->type != IR_MEM_STORE && mem->m_load.dest == from) {
                // If the instruction assigns the variable, rename the assignment.
                dlist_remove(&from->assigned_at, &mem->dest_node);
                to               = ir_var_create(func, from->prim_type, NULL);
                mem->m_load.dest = to;
                dlist_append(&to->assigned_at, &mem->dest_node);
            }
        }
    }
    if (to) {
        set_foreach(ir_code_t, succ, &code->succ) {
            replace_phi_vars(code, succ, phi_from, to);
        }
    }
    set_foreach(ir_code_t, succ, &code->succ) {
        rename_assignments(func, succ, from, to, phi_from);
    }
}

// Convert non-SSA to SSA form.
void ir_func_to_ssa(ir_func_t *func) {
    if (func->enforce_ssa) {
        return;
    }
    size_t      nodes_len = func->code_list.len;
    dom_node_t *nodes     = calloc(sizeof(dom_node_t), nodes_len);

    compute_dominance(func, nodes_len, nodes);

    size_t    limit = func->vars_list.len;
    ir_var_t *var   = (ir_var_t *)func->vars_list.head;
    for (size_t i = 0; i < limit; i++) {
        // Insert phi functions.
        insert_combinators(func, var, nodes_len, nodes);

        // Rename variable definitions.
        dlist_foreach_node(ir_code_t, code, &func->code_list) {
            code->visited = false;
        }
        set_t phi_set = PTR_SET_EMPTY;
        set_add(&phi_set, var);
        rename_assignments(func, (ir_code_t *)func->code_list.head, var, NULL, &phi_set);
        set_clear(&phi_set);

        var = (ir_var_t *)var->node.next;
    }

    for (size_t i = 0; i < nodes_len; i++) {
        set_clear(&nodes[i].bucket);
        set_clear(&nodes[i].frontier);
    }
    free(nodes);
    func->enforce_ssa = true;
}



// Recalculate the predecessors and successors for code blocks.
void ir_func_recalc_flow(ir_func_t *func) {
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        set_clear(&code->pred);
        set_clear(&code->succ);
    }
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        dlist_foreach_node(ir_insn_t, insn, &code->insns) {
            if (insn->type != IR_INSN_FLOW) {
                continue;
            }
            ir_flow_t *flow = (void *)insn;
            if (flow->type == IR_FLOW_JUMP || flow->type == IR_FLOW_BRANCH) {
                set_add(&flow->f_jump.target->pred, code);
                set_add(&code->succ, flow->f_jump.target);
            }
        }
    }
}



// Create a new stack frame.
// If `name` is `NULL`, its name will be `frame%zu` where `%zu` is a number.
ir_frame_t *ir_frame_create(ir_func_t *func, uint64_t size, uint64_t align, char const *name) {
    if (!align || align & (align - 1)) {
        fprintf(stderr, "[BUG] Stack frame does not have power-of-2 alignment\n");
        abort();
    }
    if (size & (align - 1)) {
        fprintf(stderr, "[BUG] Stack frame size is not an integer multiple of alignment\n");
        abort();
    }
    ir_frame_t *frame = calloc(1, sizeof(ir_frame_t));
    if (name) {
        frame->name = strong_strdup(name);
    } else {
        char const *fmt = "frame%zu";
        size_t      len = snprintf(NULL, 0, fmt, func->frames_list.len);
        frame->name     = calloc(1, len + 1);
        snprintf(frame->name, len + 1, fmt, func->frames_list.len);
    }
    frame->func  = func;
    frame->size  = size;
    frame->align = align;
    frame->node  = DLIST_NODE_EMPTY;
    dlist_append(&func->frames_list, &frame->node);
    return frame;
}



// Create a new variable.
// If `name` is `NULL`, its name will be `var%zu` where `%zu` is a number.
ir_var_t *ir_var_create(ir_func_t *func, ir_prim_t type, char const *name) {
    ir_var_t *var = calloc(1, sizeof(ir_var_t));
    if (name) {
        var->name = strong_strdup(name);
    } else {
        char const *fmt = "var%zu";
        size_t      len = snprintf(NULL, 0, fmt, func->vars_list.len);
        var->name       = calloc(1, len + 1);
        snprintf(var->name, len + 1, fmt, func->vars_list.len);
    }
    var->prim_type = type;
    var->func      = func;
    var->used_at   = PTR_SET_EMPTY;
    var->node      = DLIST_NODE_EMPTY;
    dlist_append(&func->vars_list, &var->node);
    return var;
}

// Delete an IR variable, removing all assignments and references in the process.
void ir_var_delete(ir_var_t *var) {
    set_t to_delete = PTR_SET_EMPTY;
    set_addall(&to_delete, &var->used_at);
    dlist_foreach(ir_expr_t, expr, dest_node, &var->assigned_at) {
        set_add(&to_delete, expr);
    }
    set_foreach(ir_insn_t, insn, &to_delete) {
        ir_insn_delete(insn);
    }
    set_clear(&to_delete);
    dlist_remove(&var->func->vars_list, &var->node);
    set_clear(&var->used_at);
    free(var->name);
    free(var);
}

// Replace all references to a variable with a constant.
// Does not replace assignments, nor does it delete the variable.
void ir_var_replace(ir_var_t *var, ir_operand_t value) {
    if (!value.is_const && value.var == var) {
        fprintf(stderr, "[BUG] IR variable %%%s asked to be replaced with itself\n", var->name);
        abort();
    }
    set_foreach(ir_insn_t, insn, &var->used_at) {
        if (insn->type == IR_INSN_EXPR) {
            ir_expr_t *expr = (void *)insn;
            if (expr->type == IR_EXPR_UNARY) {
                expr->e_unary.value = value;
                if (!value.is_const) {
                    set_add(&value.var->used_at, insn);
                }
            } else if (expr->type == IR_EXPR_BINARY) {
                if (!expr->e_binary.lhs.is_const && expr->e_binary.lhs.var == var) {
                    expr->e_binary.lhs = value;
                    if (!value.is_const) {
                        set_add(&value.var->used_at, insn);
                    }
                }
                if (!expr->e_binary.rhs.is_const && expr->e_binary.rhs.var == var) {
                    expr->e_binary.rhs = value;
                    if (!value.is_const) {
                        set_add(&value.var->used_at, insn);
                    }
                }
            } else if (expr->type == IR_EXPR_COMBINATOR) {
                for (size_t i = 0; i < expr->e_combinator.from_len; i++) {
                    if (!expr->e_combinator.from[i].bind.is_const && expr->e_combinator.from[i].bind.var == var) {
                        expr->e_combinator.from[i].bind = value;
                        if (!value.is_const) {
                            set_add(&value.var->used_at, insn);
                        }
                    }
                }
            } else if (expr->type == IR_EXPR_UNDEFINED) {
                // Nothing to do.
            } else {
                abort();
            }
        } else if (insn->type == IR_INSN_MEM) {
            ir_mem_t *mem = (void *)insn;
            if (mem->type == IR_MEM_LOAD) {
                if (!mem->m_load.addr.is_const && mem->m_load.addr.var == var) {
                    mem->m_load.addr = value;
                    if (!value.is_const) {
                        set_add(&value.var->used_at, insn);
                    }
                }
            } else if (mem->type == IR_MEM_STORE) {
                if (!mem->m_store.src.is_const && mem->m_store.src.var == var) {
                    mem->m_store.src = value;
                    if (!value.is_const) {
                        set_add(&value.var->used_at, insn);
                    }
                }
                if (!mem->m_store.addr.is_const && mem->m_store.addr.var == var) {
                    mem->m_store.addr = value;
                    if (!value.is_const) {
                        set_add(&value.var->used_at, insn);
                    }
                }
            } else if (mem->type == IR_MEM_LEA_STACK || mem->type == IR_MEM_LEA_SYMBOL) {
                // Nothing to do.
            } else {
                abort();
            }
        } else if (insn->type == IR_INSN_FLOW) {
            ir_flow_t *flow = (void *)insn;
            if (flow->type == IR_FLOW_BRANCH) {
                if (!flow->f_branch.cond.is_const && flow->f_branch.cond.var == var) {
                    flow->f_branch.cond = value;
                    if (!value.is_const) {
                        set_add(&value.var->used_at, insn);
                    }
                }
            } else if (flow->type == IR_FLOW_CALL_PTR || flow->type == IR_FLOW_CALL_DIRECT) {
                if (flow->type == IR_FLOW_CALL_PTR && !flow->f_call_ptr.addr.is_const
                    && flow->f_call_ptr.addr.var == var) {
                    flow->f_call_ptr.addr = value;
                    if (!value.is_const) {
                        set_add(&value.var->used_at, insn);
                    }
                }
                for (size_t i = 0; i < flow->f_call_direct.args_len; i++) {
                    if (!flow->f_call_direct.args[i].is_const) {
                        flow->f_call_direct.args[i] = value;
                        if (!value.is_const) {
                            set_add(&value.var->used_at, insn);
                        }
                    }
                }
            } else if (flow->type == IR_FLOW_RETURN) {
                if (flow->f_return.has_value && !flow->f_return.value.is_const) {
                    flow->f_return.value = value;
                    if (!value.is_const) {
                        set_add(&value.var->used_at, insn);
                    }
                }
            } else {
                abort();
            }
        } else {
            abort();
        }
    }
    set_clear(&var->used_at);
}

// Create a new IR code block.
// If `name` is `NULL`, its name will be `code%zu` where `%zu` is a number.
ir_code_t *ir_code_create(ir_func_t *func, char const *name) {
    ir_code_t *code = strong_calloc(1, sizeof(ir_code_t));
    code->func      = func;
    code->pred      = PTR_SET_EMPTY;
    code->succ      = PTR_SET_EMPTY;
    code->node      = DLIST_NODE_EMPTY;
    if (name) {
        code->name = strong_strdup(name);
    } else {
        char const *fmt = "label%zu";
        size_t      len = snprintf(NULL, 0, fmt, func->code_list.len);
        code->name      = calloc(1, len + 1);
        snprintf(code->name, len + 1, fmt, func->code_list.len);
    }
    dlist_append(&func->code_list, &code->node);
    return code;
}

// Remove a predecessor from a combinator.
static void remove_combinator_path(ir_expr_t *expr, ir_code_t *code) {
    for (size_t i = 0; i < expr->e_combinator.from_len; i++) {
        if (expr->e_combinator.from[i].prev == code) {
            if (!expr->e_combinator.from[i].bind.is_const) {
                set_remove(&expr->e_combinator.from[i].bind.var->used_at, expr);
            }
            array_remove(expr->e_combinator.from, sizeof(ir_combinator_t), expr->e_combinator.from_len, NULL, i);
            expr->e_combinator.from_len--;
            break;
        }
    }
    if (expr->e_combinator.from_len == 1) {
        ir_var_replace(expr->dest, expr->e_combinator.from[0].bind);
        ir_insn_delete(&expr->base);
    }
}

// Delete an IR code block and all contained instructions.
void ir_code_delete(ir_code_t *code) {
    // Remove this code as predecessor/successor.
    set_foreach(ir_code_t, pred, &code->pred) {
        set_remove(&pred->succ, code);
        // Delete jump instructions to this code.
        ir_insn_t *insn = container_of(pred->insns.head, ir_insn_t, node);
        while (insn) {
            ir_insn_t *next = container_of(insn->node.next, ir_insn_t, node);
            if (insn->type == IR_INSN_FLOW) {
                ir_flow_t *flow = (void *)insn;
                if ((flow->type == IR_FLOW_JUMP || flow->type == IR_FLOW_BRANCH) && flow->f_jump.target == code) {
                    ir_insn_delete(insn);
                }
            }
            insn = next;
        }
    }
    set_foreach(ir_code_t, succ, &code->succ) {
        set_remove(&succ->pred, code);
        // Update phi nodes in successors.
        ir_insn_t *insn = container_of(succ->insns.head, ir_insn_t, node);
        while (insn) {
            ir_insn_t *next = container_of(insn->node.next, ir_insn_t, node);
            if (insn->type == IR_INSN_EXPR) {
                ir_expr_t *expr = (void *)insn;
                if (expr->type == IR_EXPR_COMBINATOR) {
                    remove_combinator_path(expr, code);
                }
            }
            insn = next;
        }
    }
    // Delete all instructions.
    while (code->insns.len) {
        ir_insn_delete((void *)code->insns.head);
    }
    // Release memory.
    dlist_remove(&code->func->code_list, &code->node);
    set_clear(&code->pred);
    set_clear(&code->succ);
    free(code->name);
    free(code);
}

// Delete an instruction from the code.
void ir_insn_delete(ir_insn_t *insn) {
    if (insn->type == IR_INSN_EXPR) {
        ir_expr_t *expr = (void *)insn;
        dlist_remove(&expr->dest->assigned_at, &expr->dest_node);
        if (expr->type == IR_EXPR_UNARY) {
            if (!expr->e_unary.value.is_const) {
                set_remove(&expr->e_unary.value.var->used_at, insn);
            }
        } else if (expr->type == IR_EXPR_BINARY) {
            if (!expr->e_binary.lhs.is_const) {
                set_remove(&expr->e_binary.lhs.var->used_at, insn);
            }
            if (!expr->e_binary.rhs.is_const) {
                set_remove(&expr->e_binary.rhs.var->used_at, insn);
            }
        } else if (expr->type == IR_EXPR_COMBINATOR) {
            for (size_t i = 0; i < expr->e_combinator.from_len; i++) {
                if (!expr->e_combinator.from[i].bind.is_const) {
                    set_remove(&expr->e_combinator.from[i].bind.var->used_at, insn);
                }
            }
            free(expr->e_combinator.from);
        } else {
            abort();
        }
    } else if (insn->type == IR_INSN_MEM) {
        ir_mem_t *mem = (void *)insn;
        if (mem->type == IR_MEM_LEA_STACK) {
            dlist_remove(&mem->m_lea_stack.dest->assigned_at, &mem->dest_node);
        } else if (mem->type == IR_MEM_LEA_SYMBOL) {
            dlist_remove(&mem->m_lea_symbol.dest->assigned_at, &mem->dest_node);
            free(mem->m_lea_symbol.symbol);
        } else if (mem->type == IR_MEM_LOAD) {
            dlist_remove(&mem->m_load.dest->assigned_at, &mem->dest_node);
            if (!mem->m_load.addr.is_const) {
                set_remove(&mem->m_load.addr.var->used_at, insn);
            }
        } else if (mem->type == IR_MEM_STORE) {
            if (!mem->m_store.src.is_const) {
                set_remove(&mem->m_store.src.var->used_at, insn);
            }
            if (!mem->m_store.addr.is_const) {
                set_remove(&mem->m_store.addr.var->used_at, insn);
            }
        } else {
            abort();
        }
    } else if (insn->type == IR_INSN_FLOW) {
        ir_flow_t *flow = (void *)insn;
        if (flow->type == IR_FLOW_BRANCH) {
            if (!flow->f_branch.cond.is_const) {
                set_remove(&flow->f_branch.cond.var->used_at, insn);
            }
        } else if (flow->type == IR_FLOW_RETURN) {
            if (flow->f_return.has_value && !flow->f_return.value.is_const) {
                set_remove(&flow->f_return.value.var->used_at, insn);
            }
        } else if (flow->type == IR_FLOW_CALL_DIRECT || flow->type == IR_FLOW_CALL_PTR) {
            if (flow->type == IR_FLOW_CALL_DIRECT) {
                free(flow->f_call_direct.label);
            } else if (flow->f_call_ptr.addr.is_const) {
                set_remove(&flow->f_call_ptr.addr.var->used_at, insn);
            }
            for (size_t i = 0; i < flow->f_call_direct.args_len; i++) {
                if (!flow->f_call_direct.args[i].is_const) {
                    set_remove(&flow->f_call_direct.args[i].var->used_at, insn);
                }
            }
        } else if (flow->type == IR_FLOW_JUMP) {
            // Nothing to do.
        } else {
            abort();
        }
    } else {
        abort();
    }
    dlist_remove(&insn->parent->insns, &insn->node);
    free(insn);
}



// Add a combinator function to a code block.
// Takes ownership of the `from` array.
void ir_add_combinator(ir_code_t *code, ir_var_t *dest, size_t from_len, ir_combinator_t *from) {
    ir_expr_t *expr             = calloc(1, sizeof(ir_expr_t));
    expr->base.type             = IR_INSN_EXPR;
    expr->base.parent           = code;
    expr->type                  = IR_EXPR_COMBINATOR;
    expr->e_combinator.from_len = from_len;
    expr->e_combinator.from     = from;
    expr->dest                  = dest;
    if (code->insns.len) {
        ir_insn_t *last = container_of(code->insns.tail, ir_insn_t, node);
        if (last->type == IR_INSN_FLOW) {
            ir_flow_t *flow = (void *)last;
            if (flow->type == IR_FLOW_BRANCH || flow->type == IR_FLOW_JUMP) {
                fprintf(stderr, "[BUG] Cannot have expr after jump or branch\n");
                abort();
            }
        }
    }
    if (dest->assigned_at.len && code->func->enforce_ssa) {
        fprintf(stderr, "[BUG] SSA IR variable %%%s assigned twice\n", dest->name);
        abort();
    }
    for (size_t i = 0; i < from_len; i++) {
        ir_prim_t bind_prim = from[i].bind.is_const ? from[i].bind.iconst.prim_type : from[i].bind.var->prim_type;
        if (bind_prim != dest->prim_type) {
            fprintf(stderr, "IR phi has conflicting bind and return types\n");
            abort();
        }
        if (!from[i].bind.is_const) {
            set_add(&from[i].bind.var->used_at, expr);
        }
    }
    dlist_append(&dest->assigned_at, &expr->dest_node);
    dlist_append(&code->insns, &expr->base.node);
}

// Add an expression to a code block.
void ir_add_expr1(ir_code_t *code, ir_var_t *dest, ir_op1_type_t oper, ir_operand_t operand) {
    ir_expr_t *expr     = calloc(1, sizeof(ir_expr_t));
    expr->base.type     = IR_INSN_EXPR;
    expr->base.parent   = code;
    expr->type          = IR_EXPR_UNARY;
    expr->e_unary.oper  = oper;
    expr->e_unary.value = operand;
    expr->dest          = dest;
    if (code->insns.len) {
        ir_insn_t *last = container_of(code->insns.tail, ir_insn_t, node);
        if (last->type == IR_INSN_FLOW) {
            ir_flow_t *flow = (void *)last;
            if (flow->type == IR_FLOW_BRANCH || flow->type == IR_FLOW_JUMP) {
                fprintf(stderr, "[BUG] Cannot have expr after jump or branch\n");
                abort();
            }
        }
    }
    if (!operand.is_const) {
        set_add(&operand.var->used_at, expr);
    }
    if (oper == IR_OP1_snez || oper == IR_OP1_seqz) {
        if (dest->prim_type != IR_PRIM_bool) {
            fprintf(stderr, "[BUG] IR %s must return a boolean\n", ir_op1_names[oper]);
            abort();
        }
    } else if (oper != IR_OP1_mov) {
        ir_prim_t operand_prim = operand.is_const ? operand.iconst.prim_type : operand.var->prim_type;
        if (operand_prim != dest->prim_type) {
            fprintf(stderr, "[BUG] IR expr1 has conflicting operand and return types\n");
            abort();
        }
    }
    if (dest->assigned_at.len && code->func->enforce_ssa) {
        fprintf(stderr, "[BUG] SSA IR variable %%%s assigned twice\n", dest->name);
        abort();
    }
    dlist_append(&dest->assigned_at, &expr->dest_node);
    dlist_append(&code->insns, &expr->base.node);
}

// Add an expression to a code block.
void ir_add_expr2(ir_code_t *code, ir_var_t *dest, ir_op2_type_t oper, ir_operand_t lhs, ir_operand_t rhs) {
    ir_expr_t *expr     = calloc(1, sizeof(ir_expr_t));
    expr->base.type     = IR_INSN_EXPR;
    expr->base.parent   = code;
    expr->type          = IR_EXPR_BINARY;
    expr->e_binary.oper = oper;
    expr->e_binary.lhs  = lhs;
    expr->e_binary.rhs  = rhs;
    expr->dest          = dest;
    if (code->insns.len) {
        ir_insn_t *last = container_of(code->insns.tail, ir_insn_t, node);
        if (last->type == IR_INSN_FLOW) {
            ir_flow_t *flow = (void *)last;
            if (flow->type == IR_FLOW_BRANCH || flow->type == IR_FLOW_JUMP) {
                fprintf(stderr, "[BUG] Cannot have expr after jump or branch\n");
                abort();
            }
        }
    }
    ir_prim_t lhs_prim = lhs.is_const ? lhs.iconst.prim_type : lhs.var->prim_type;
    if (lhs_prim != dest->prim_type) {
        fprintf(stderr, "[BUG] IR expr2 has conflicting operand and return types\n");
        abort();
    }
    ir_prim_t rhs_prim = rhs.is_const ? rhs.iconst.prim_type : rhs.var->prim_type;
    if (rhs_prim != dest->prim_type) {
        fprintf(stderr, "[BUG] IR expr2 has conflicting operand and return types\n");
        abort();
    }
    if (!lhs.is_const) {
        set_add(&lhs.var->used_at, expr);
    }
    if (!rhs.is_const) {
        set_add(&rhs.var->used_at, expr);
    }
    if (dest->assigned_at.len && code->func->enforce_ssa) {
        fprintf(stderr, "[BUG] SSA IR variable %%%s assigned twice\n", dest->name);
        abort();
    }
    dlist_append(&dest->assigned_at, &expr->dest_node);
    dlist_append(&code->insns, &expr->base.node);
}

// Add an undefined variable.
void ir_add_undefined(ir_code_t *code, ir_var_t *dest) {
    ir_expr_t *expr   = calloc(1, sizeof(ir_expr_t));
    expr->base.type   = IR_INSN_EXPR;
    expr->base.parent = code;
    expr->type        = IR_EXPR_UNDEFINED;
    expr->dest        = dest;
    if (code->insns.len) {
        ir_insn_t *last = container_of(code->insns.tail, ir_insn_t, node);
        if (last->type == IR_INSN_FLOW) {
            ir_flow_t *flow = (void *)last;
            if (flow->type == IR_FLOW_BRANCH || flow->type == IR_FLOW_JUMP) {
                fprintf(stderr, "[BUG] Cannot have expr after jump or branch\n");
                abort();
            }
        }
    }
    if (dest->assigned_at.len && code->func->enforce_ssa) {
        fprintf(stderr, "[BUG] SSA IR variable %%%s assigned twice\n", dest->name);
        abort();
    }
    dlist_append(&dest->assigned_at, &expr->dest_node);
    dlist_append(&code->insns, &expr->base.node);
}

// Add a direct (by label) function call.
// Takes ownership of `params`.
void ir_add_call_direct(ir_code_t *from, char const *label, size_t params_len, ir_operand_t *params) {
    ir_flow_t *flow              = calloc(1, sizeof(ir_flow_t));
    flow->base.parent            = from;
    flow->type                   = IR_FLOW_CALL_DIRECT;
    flow->f_call_direct.label    = strong_strdup(label);
    flow->f_call_direct.args_len = params_len;
    flow->f_call_direct.args     = params;
    if (from->insns.len) {
        ir_insn_t *last = container_of(from->insns.tail, ir_insn_t, node);
        if (last->type == IR_INSN_FLOW) {
            ir_flow_t *flow = (void *)last;
            if (flow->type == IR_FLOW_BRANCH || flow->type == IR_FLOW_JUMP) {
                fprintf(stderr, "[BUG] Cannot have call after jump or branch\n");
                abort();
            }
        }
    }
    for (size_t i = 0; i < params_len; i++) {
        if (!params[i].is_const) {
            set_add(&params[i].var->used_at, flow);
        }
    }
    dlist_append(&from->insns, &flow->base.node);
}

// Add an indirect (by pointer) function call.
// Takes ownership of `params`.
void ir_add_call_ptr(ir_code_t *from, ir_operand_t funcptr, size_t params_len, ir_operand_t *params) {
    ir_flow_t *flow           = calloc(1, sizeof(ir_flow_t));
    flow->base.parent         = from;
    flow->type                = IR_FLOW_CALL_PTR;
    flow->f_call_ptr.addr     = funcptr;
    flow->f_call_ptr.args_len = params_len;
    flow->f_call_ptr.args     = params;
    if (from->insns.len) {
        ir_insn_t *last = container_of(from->insns.tail, ir_insn_t, node);
        if (last->type == IR_INSN_FLOW) {
            ir_flow_t *flow = (void *)last;
            if (flow->type == IR_FLOW_BRANCH || flow->type == IR_FLOW_JUMP) {
                fprintf(stderr, "[BUG] Cannot have call after jump or branch\n");
                abort();
            }
        }
    }
    if (!funcptr.is_const) {
        set_add(&funcptr.var->used_at, flow);
    }
    for (size_t i = 0; i < params_len; i++) {
        if (!params[i].is_const) {
            set_add(&params[i].var->used_at, flow);
        }
    }
    dlist_append(&from->insns, &flow->base.node);
}


// Add a load effective address of a stack frame to a code block.
void ir_add_lea_stack(ir_code_t *code, ir_var_t *dest, ir_frame_t *frame, uint64_t offset) {
    ir_mem_t *mem           = calloc(1, sizeof(ir_mem_t));
    mem->base.type          = IR_INSN_MEM;
    mem->base.parent        = code;
    mem->type               = IR_MEM_LEA_STACK;
    mem->m_lea_stack.dest   = dest;
    mem->m_lea_stack.frame  = frame;
    mem->m_lea_stack.offset = offset;
    if (dest->assigned_at.len && code->func->enforce_ssa) {
        fprintf(stderr, "[BUG] SSA IR variable %%%s assigned twice\n", dest->name);
        abort();
    }
    if (code->insns.len) {
        ir_insn_t *last = container_of(code->insns.tail, ir_insn_t, node);
        if (last->type == IR_INSN_FLOW) {
            ir_flow_t *flow = (void *)last;
            if (flow->type == IR_FLOW_BRANCH || flow->type == IR_FLOW_JUMP) {
                fprintf(stderr, "[BUG] Cannot have lea after jump or branch\n");
                abort();
            }
        }
    }
    dlist_append(&dest->assigned_at, &mem->dest_node);
    dlist_append(&code->insns, &mem->base.node);
}

// Add a load effective address of a symbol to a code block.
void ir_add_lea_symbol(ir_code_t *code, ir_var_t *dest, char const *symbol, uint64_t offset) {
    ir_mem_t *mem            = calloc(1, sizeof(ir_mem_t));
    mem->base.type           = IR_INSN_MEM;
    mem->base.parent         = code;
    mem->type                = IR_MEM_LEA_SYMBOL;
    mem->m_lea_symbol.dest   = dest;
    mem->m_lea_symbol.symbol = strong_strdup(symbol);
    mem->m_lea_symbol.offset = offset;
    if (dest->assigned_at.len && code->func->enforce_ssa) {
        fprintf(stderr, "[BUG] SSA IR variable %%%s assigned twice\n", dest->name);
        abort();
    }
    if (code->insns.len) {
        ir_insn_t *last = container_of(code->insns.tail, ir_insn_t, node);
        if (last->type == IR_INSN_FLOW) {
            ir_flow_t *flow = (void *)last;
            if (flow->type == IR_FLOW_BRANCH || flow->type == IR_FLOW_JUMP) {
                fprintf(stderr, "[BUG] Cannot have lea after jump or branch\n");
                abort();
            }
        }
    }
    dlist_append(&dest->assigned_at, &mem->dest_node);
    dlist_append(&code->insns, &mem->base.node);
}

// Add a memory load to a code block.
void ir_add_load(ir_code_t *code, ir_var_t *dest, ir_operand_t addr) {
    ir_mem_t *mem    = calloc(1, sizeof(ir_mem_t));
    mem->base.type   = IR_INSN_MEM;
    mem->base.parent = code;
    mem->type        = IR_MEM_LOAD;
    mem->m_load.dest = dest;
    mem->m_load.addr = addr;
    if (!addr.is_const) {
        set_add(&addr.var->used_at, mem);
    }
    if (dest->assigned_at.len && code->func->enforce_ssa) {
        fprintf(stderr, "[BUG] SSA IR variable %%%s assigned twice\n", dest->name);
        abort();
    }
    if (code->insns.len) {
        ir_insn_t *last = container_of(code->insns.tail, ir_insn_t, node);
        if (last->type == IR_INSN_FLOW) {
            ir_flow_t *flow = (void *)last;
            if (flow->type == IR_FLOW_BRANCH || flow->type == IR_FLOW_JUMP) {
                fprintf(stderr, "[BUG] Cannot have load after jump or branch\n");
                abort();
            }
        }
    }
    dlist_append(&dest->assigned_at, &mem->dest_node);
    dlist_append(&code->insns, &mem->base.node);
}

// Add a memory store to a code block.
void ir_add_store(ir_code_t *code, ir_operand_t src, ir_operand_t addr) {
    ir_mem_t *mem     = calloc(1, sizeof(ir_mem_t));
    mem->base.type    = IR_INSN_MEM;
    mem->base.parent  = code;
    mem->type         = IR_MEM_STORE;
    mem->m_store.src  = src;
    mem->m_store.addr = addr;
    if (!src.is_const) {
        set_add(&src.var->used_at, mem);
    }
    if (!addr.is_const) {
        set_add(&addr.var->used_at, mem);
    }
    if (code->insns.len) {
        ir_insn_t *last = container_of(code->insns.tail, ir_insn_t, node);
        if (last->type == IR_INSN_FLOW) {
            ir_flow_t *flow = (void *)last;
            if (flow->type == IR_FLOW_BRANCH || flow->type == IR_FLOW_JUMP) {
                fprintf(stderr, "[BUG] Cannot have store after jump or branch\n");
                abort();
            }
        }
    }
    dlist_append(&code->insns, &mem->base.node);
}


// Add an unconditional jump.
void ir_add_jump(ir_code_t *from, ir_code_t *to) {
    ir_flow_t *flow     = calloc(1, sizeof(ir_flow_t));
    flow->base.type     = IR_INSN_FLOW;
    flow->base.parent   = from;
    flow->type          = IR_FLOW_JUMP;
    flow->f_jump.target = to;
    set_add(&from->succ, to);
    set_add(&to->pred, from);
    dlist_append(&from->insns, &flow->base.node);
}

// Add a conditional branch.
void ir_add_branch(ir_code_t *from, ir_operand_t cond, ir_code_t *to) {
    ir_flow_t *flow       = calloc(1, sizeof(ir_flow_t));
    flow->base.type       = IR_INSN_FLOW;
    flow->base.parent     = from;
    flow->type            = IR_FLOW_BRANCH;
    flow->f_branch.cond   = cond;
    flow->f_branch.target = to;
    ir_prim_t cond_prim   = cond.is_const ? cond.iconst.prim_type : cond.var->prim_type;
    if (cond_prim != IR_PRIM_bool) {
        fprintf(stderr, "[BUG] IR branch requires a boolean condition\n");
        abort();
    }
    if (!cond.is_const) {
        set_add(&cond.var->used_at, flow);
    }
    set_add(&from->succ, to);
    set_add(&to->pred, from);
    dlist_append(&from->insns, &flow->base.node);
}

// Add a return without value.
void ir_add_return0(ir_code_t *from) {
    ir_flow_t *flow   = calloc(1, sizeof(ir_flow_t));
    flow->base.type   = IR_INSN_FLOW;
    flow->base.parent = from;
    flow->type        = IR_FLOW_RETURN;
    if (from->insns.len) {
        ir_insn_t *last = container_of(from->insns.tail, ir_insn_t, node);
        if (last->type == IR_INSN_FLOW) {
            ir_flow_t *flow = (void *)last;
            if (flow->type == IR_FLOW_BRANCH || flow->type == IR_FLOW_JUMP) {
                fprintf(stderr, "[BUG] Cannot have return after jump or branch\n");
                abort();
            }
        }
    }
    dlist_append(&from->insns, &flow->base.node);
}

// Add a return with value.
void ir_add_return1(ir_code_t *from, ir_operand_t value) {
    ir_flow_t *flow          = calloc(1, sizeof(ir_flow_t));
    flow->base.type          = IR_INSN_FLOW;
    flow->base.parent        = from;
    flow->type               = IR_FLOW_RETURN;
    flow->f_return.has_value = true;
    flow->f_return.value     = value;
    if (from->insns.len) {
        ir_insn_t *last = container_of(from->insns.tail, ir_insn_t, node);
        if (last->type == IR_INSN_FLOW) {
            ir_flow_t *flow = (void *)last;
            if (flow->type == IR_FLOW_BRANCH || flow->type == IR_FLOW_JUMP) {
                fprintf(stderr, "[BUG] Cannot have return after jump or branch\n");
                abort();
            }
        }
    }
    if (!value.is_const) {
        set_add(&value.var->used_at, flow);
    }
    dlist_append(&from->insns, &flow->base.node);
}
