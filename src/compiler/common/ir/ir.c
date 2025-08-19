
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir.h"

#include "arrays.h"
#include "insn_proto.h"
#include "ir/ir_optimizer.h"
#include "ir_types.h"
#include "list.h"
#include "map.h"
#include "set.h"
#include "strong_malloc.h"

#include <assert.h>
#include <complex.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



// Helper macro for performing an operation on all variables in an IR operand.
#define FOR_OPERAND_VARS(operand, name, action)                                                                        \
    ({                                                                                                                 \
        if ((operand).type == IR_OPERAND_TYPE_VAR) {                                                                   \
            ir_var_t *name = (operand).var;                                                                            \
            action                                                                                                     \
        } else if ((operand).type == IR_OPERAND_TYPE_MEM) {                                                            \
            if ((operand).mem.rel_type == IR_MEMREL_VAR) {                                                             \
                ir_var_t *name = (operand).mem.base_var;                                                               \
                action                                                                                                 \
            }                                                                                                          \
        }                                                                                                              \
    })

// Helper function that marks an operand as not used by an instruction.
static void ir_unmark_used(ir_operand_t operand, ir_insn_t *insn) {
    FOR_OPERAND_VARS(operand, var, set_remove(&var->used_at, insn););
}

// Helper function that marks an operand as used by an instruction.
void ir_mark_used(ir_operand_t operand, ir_insn_t *insn) {
    FOR_OPERAND_VARS(operand, var, set_add(&var->used_at, insn););
}

// Create a new IR function.
// Function argument types are IR_PRIM_s32 by default.
ir_func_t *ir_func_create(char const *name, char const *entry_name, size_t args_len, char const *const *args_name) {
    ir_func_t *func = ir_func_create_empty(name);
    func->args      = strong_calloc(args_len, sizeof(ir_arg_t));
    func->args_len  = args_len;
    for (size_t i = 0; i < args_len; i++) {
        func->args[i].has_var        = true;
        func->args[i].var            = ir_var_create(func, IR_PRIM_s32, args_name ? args_name[i] : NULL);
        func->args[i].var->arg_index = (ptrdiff_t)i;
    }
    func->entry = ir_code_create(func, entry_name);
    return func;
}

// Create an IR function without operands nor code.
ir_func_t *ir_func_create_empty(char const *name) {
    ir_func_t *func     = strong_calloc(1, sizeof(ir_func_t));
    func->name          = strong_strdup(name);
    func->code_by_name  = STR_MAP_EMPTY;
    func->var_by_name   = STR_MAP_EMPTY;
    func->frame_by_name = STR_MAP_EMPTY;
    return func;
}

// Delete an IR function.
void ir_func_delete(ir_func_t *func) {
    while (func->code_list.len) {
        ir_code_t *code = (ir_code_t *)func->code_list.head;
        ir_code_delete(code);
    }

    while (func->vars_list.len) {
        ir_var_t *var = (ir_var_t *)func->vars_list.head;
        assert(var->used_at.len == 0);
        assert(var->assigned_at.len == 0);
        ir_var_delete(var);
    }

    while (func->frames_list.len) {
        // TODO: This should be its own function.
        ir_frame_t *frame = (ir_frame_t *)dlist_pop_front(&func->frames_list);
        map_remove(&func->frame_by_name, frame->name);
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

        set_foreach(void, v0, &nodes[p].bucket) {
            size_t v      = (size_t)v0;
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
    ir_insn_t *expr       = calloc(1, sizeof(ir_insn_t));
    expr->type            = IR_INSN_COMBINATOR;
    expr->code            = code;
    expr->combinators_len = code->pred.len;
    expr->combinators     = strong_calloc(1, code->pred.len * sizeof(ir_combinator_t));
    expr->returns         = strong_malloc(sizeof(void *));
    expr->returns[0]      = dest;
    expr->returns_len     = 1;
    size_t i              = 0;
    set_foreach(ir_code_t, pred, &code->pred) {
        expr->combinators[i++] = (ir_combinator_t){
            .bind = IR_OPERAND_UNDEF(dest->prim_type),
            .prev = pred,
        };
    }
    set_add(&dest->assigned_at, expr);
    dlist_prepend(&code->insns, &expr->node);
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
        nodes[insn->code->dfs_index].uses_var = true;
    }
    set_foreach(ir_insn_t, expr, &var->assigned_at) {
        // The search starts at each definition so that anything before won't be marked as using it.
        // Even though it can be assigned both by memory insns and exprs, this is okay to do.
        nodes[expr->code->dfs_index].uses_var = true;
        var_usage_dfs(expr->code, nodes);
    }

    // Mark as not having a combinator function.
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        code->visited = false;
    }
    set_foreach(ir_insn_t, expr, &var->assigned_at) {
        // Same thing; could actually be a mem insn but this works for that too.
        set_addall(&frontier, &nodes[expr->code->dfs_index].frontier);
    }

    bool changed = true;
    while (changed) {
        changed = false;
        set_foreach(void, index0, &frontier) {
            size_t     index = (size_t)index0;
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
    if (insn->type == IR_INSN_COMBINATOR) {
        return;
    }
    for (size_t i = 0; i < insn->operands_len; i++) {
        if (insn->operands[i].type == IR_OPERAND_TYPE_VAR && insn->operands[i].var == from) {
            insn->operands[i].var = to;
            set_add(&to->used_at, insn);
        } else if (insn->operands[i].type == IR_OPERAND_TYPE_MEM) {
            if (insn->operands[i].mem.rel_type == IR_MEMREL_VAR && insn->operands[i].mem.base_var == from) {
                insn->operands[i].mem.base_var = to;
                set_add(&to->used_at, insn);
            }
        }
    }
    set_remove(&from->used_at, insn);
}

// Replace variables in the phi instructions.
static void replace_phi_vars(ir_code_t *pred, ir_code_t *code, set_t *from, ir_var_t *to) {
    dlist_foreach_node(ir_insn_t, insn, &code->insns) {
        if (insn->type != IR_INSN_COMBINATOR) {
            continue;
        } else if (set_contains(from, insn->returns[0])) {
            for (size_t i = 0; i < insn->combinators_len; i++) {
                if (insn->combinators[i].prev == pred) {
                    ir_unmark_used(insn->combinators[i].bind, insn);
                    set_add(&to->used_at, insn);
                    insn->combinators[i].bind = IR_OPERAND_VAR(to);
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
        for (size_t i = 0; i < insn->returns_len; i++) {
            if (insn->returns[i] != from) {
                continue;
            }
            to = ir_var_create(func, from->prim_type, NULL);
            set_remove(&insn->returns[i]->assigned_at, insn);
            set_add(&to->assigned_at, insn);
            set_add(phi_from, to);
            insn->returns[i] = to;
        }
    }
    set_foreach(ir_code_t, succ, &code->succ) {
        replace_phi_vars(code, succ, phi_from, to ?: from);
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

    // Converting to SSA form requires deleting trivially unreachable code.
    opt_dead_code(func);

    size_t      nodes_len = func->code_list.len;
    dom_node_t *nodes     = calloc(nodes_len, sizeof(dom_node_t));

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
            if (insn->type == IR_INSN_JUMP || insn->type == IR_INSN_BRANCH) {
                ir_code_t *target = insn->operands[0].mem.base_code;
                set_add(&target->pred, code);
                set_add(&code->succ, target);
            }
        }
    }
}



// Check that a name isn't already in use.
static void name_free_assert(ir_func_t *func, char *name) {
    if (map_get(&func->code_by_name, name) || map_get(&func->var_by_name, name)
        || map_get(&func->frame_by_name, name)) {
        fprintf(stderr, "[BUG] The name %%%s is already in use\n", name);
        abort();
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
    name_free_assert(func, frame->name);
    map_set(&func->frame_by_name, frame->name, frame);
    dlist_append(&func->frames_list, &frame->node);
    return frame;
}



// Create a new variable.
// If `name` is `NULL`, its name will be `var%zu` where `%zu` is a number.
ir_var_t *ir_var_create(ir_func_t *func, ir_prim_t type, char const *name) {
    assert(type < IR_N_PRIM);
    ir_var_t *var = calloc(1, sizeof(ir_var_t));
    if (name) {
        var->name = strong_strdup(name);
    } else {
        char const *fmt = "var%zu";
        size_t      len = snprintf(NULL, 0, fmt, func->vars_list.len);
        var->name       = calloc(1, len + 1);
        snprintf(var->name, len + 1, fmt, func->vars_list.len);
    }
    name_free_assert(func, var->name);
    var->prim_type   = type;
    var->func        = func;
    var->assigned_at = PTR_SET_EMPTY;
    var->used_at     = PTR_SET_EMPTY;
    var->node        = DLIST_NODE_EMPTY;
    var->arg_index   = -1;
    dlist_append(&func->vars_list, &var->node);
    map_set(&func->var_by_name, var->name, var);
    return var;
}

// Delete an IR variable, removing all assignments and references in the process.
void ir_var_delete(ir_var_t *var) {
    // Delete variable's usages.
    set_t to_delete = PTR_SET_EMPTY;
    set_addall(&to_delete, &var->used_at);
    set_addall(&to_delete, &var->assigned_at);
    set_foreach(ir_insn_t, insn, &to_delete) {
        ir_insn_delete(insn);
    }
    set_clear(&to_delete);

    if (var->arg_index >= 0) {
        // Turn function arg into variable-less.
        var->func->args[var->arg_index].has_var = false;
        var->func->args[var->arg_index].type    = var->prim_type;
    }

    // Delete the variable itself.
    map_remove(&var->func->var_by_name, var->name);
    dlist_remove(&var->func->vars_list, &var->node);
    free(var->name);
    free(var);
}

// Replace all references to a variable with a constant.
// Does not replace assignments, nor does it delete the variable.
void ir_var_replace(ir_var_t *var, ir_operand_t value) {
    if (value.type == IR_OPERAND_TYPE_VAR && value.var == var) {
        fprintf(stderr, "[BUG] IR variable %%%s asked to be replaced with itself\n", var->name);
        abort();
    }
    for (set_ent_t const *ent; (ent = set_next(&var->used_at, NULL));) {
        ir_insn_t *insn = ent->value;
        if (insn->type == IR_INSN_COMBINATOR) {
            for (size_t i = 0; i < insn->combinators_len; i++) {
                if (insn->combinators[i].bind.type == IR_OPERAND_TYPE_VAR && insn->combinators[i].bind.var == var) {
                    ir_insn_set_operand(insn, i, value);
                }
            }
        } else {
            for (size_t i = 0; i < insn->operands_len; i++) {
                if (insn->operands[i].type == IR_OPERAND_TYPE_VAR && insn->operands[i].var == var) {
                    ir_insn_set_operand(insn, i, value);
                }
            }
        }
    }
    assert(var->used_at.len == 0);
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
    name_free_assert(func, code->name);
    map_set(&func->code_by_name, code->name, code);
    dlist_append(&func->code_list, &code->node);
    return code;
}

// Remove a predecessor from a combinator.
static void remove_combinator_path(ir_insn_t *expr, ir_code_t *code) {
    for (size_t i = 0; i < expr->combinators_len; i++) {
        if (expr->combinators[i].prev == code) {
            ir_unmark_used(expr->combinators[i].bind, expr);
            array_remove(expr->combinators, sizeof(ir_combinator_t), expr->combinators_len, NULL, i);
            expr->combinators_len--;
            break;
        }
    }
    if (expr->combinators_len == 1) {
        ir_var_replace(expr->returns[0], expr->combinators[0].bind);
        ir_insn_delete(expr);
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
            if ((insn->type == IR_INSN_JUMP || insn->type == IR_INSN_BRANCH)
                && insn->operands[0].mem.base_code == code) {
                ir_insn_delete(insn);
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
            if (insn->type == IR_INSN_COMBINATOR) {
                remove_combinator_path(insn, code);
            }
            insn = next;
        }
    }
    // Delete all instructions.
    while (code->insns.len) {
        ir_insn_delete((void *)code->insns.head);
    }
    // Release memory.
    map_remove(&code->func->code_by_name, code->name);
    dlist_remove(&code->func->code_list, &code->node);
    set_clear(&code->pred);
    set_clear(&code->succ);
    free(code->name);
    free(code);
}

// Delete an instruction from the code.
void ir_insn_delete(ir_insn_t *insn) {
    // Debug-assert return lengths.
    switch (insn->type) {
        case IR_INSN_EXPR2:
        case IR_INSN_EXPR1:
        case IR_INSN_LEA:
        case IR_INSN_LOAD:
        case IR_INSN_COMBINATOR: assert(insn->returns_len == 1); break;
        case IR_INSN_JUMP:
        case IR_INSN_BRANCH:
        case IR_INSN_STORE:
        case IR_INSN_CALL:
        case IR_INSN_RETURN: assert(insn->returns_len == 0); break;
        case IR_INSN_MACHINE: break;
    }

    // Debug-assert parameter lengths.
    switch (insn->type) {
        case IR_INSN_EXPR2: assert(insn->operands_len == 2); break;
        case IR_INSN_EXPR1: assert(insn->operands_len == 1); break;
        case IR_INSN_JUMP: assert(insn->operands_len == 1); break;
        case IR_INSN_BRANCH: assert(insn->operands_len == 2); break;
        case IR_INSN_LEA: assert(insn->operands_len == 1); break;
        case IR_INSN_LOAD: assert(insn->operands_len == 1); break;
        case IR_INSN_STORE: assert(insn->operands_len == 2); break;
        case IR_INSN_COMBINATOR: assert(insn->operands_len > 0); break;
        case IR_INSN_CALL: break;
        case IR_INSN_RETURN: assert(insn->operands_len <= 1); break;
        case IR_INSN_MACHINE: break;
    }

    if (insn->type == IR_INSN_COMBINATOR) {
        for (size_t i = 0; i < insn->combinators_len; i++) {
            ir_unmark_used(insn->combinators[i].bind, insn);
            if (insn->combinators[i].bind.type == IR_OPERAND_TYPE_MEM
                && insn->combinators[i].bind.mem.rel_type == IR_MEMREL_SYM) {
                free(insn->combinators[i].bind.mem.base_sym);
            }
        }
        free(insn->combinators);
    } else {
        for (size_t i = 0; i < insn->operands_len; i++) {
            ir_unmark_used(insn->operands[i], insn);
            if (insn->operands[i].type == IR_OPERAND_TYPE_MEM && insn->operands[i].mem.rel_type == IR_MEMREL_SYM) {
                free(insn->operands[i].mem.base_sym);
            }
        }
        free(insn->operands);
    }
    for (size_t i = 0; i < insn->returns_len; i++) {
        set_remove(&insn->returns[i]->assigned_at, insn);
    }
    free(insn->returns);
    dlist_remove(&insn->code->insns, &insn->node);
    free(insn);
}

// Set an IR instruction's operand by index.
void ir_insn_set_operand(ir_insn_t *insn, size_t index, ir_operand_t operand) {
    assert(index < insn->operands_len);

    // Clean up old operand.
    ir_operand_t old = insn->type == IR_INSN_COMBINATOR ? insn->combinators[index].bind : insn->operands[index];
    if (old.type == IR_OPERAND_TYPE_MEM && old.mem.rel_type == IR_MEMREL_SYM) {
        free(old.mem.base_sym);
    }
    FOR_OPERAND_VARS(old, var, set_remove(&var->used_at, insn););

    // Install new operand.
    if (operand.type == IR_OPERAND_TYPE_MEM && operand.mem.rel_type == IR_MEMREL_SYM) {
        operand.mem.base_sym = strong_strdup(operand.mem.base_sym);
    }
    FOR_OPERAND_VARS(operand, var, set_add(&var->used_at, insn););
    if (insn->type == IR_INSN_COMBINATOR) {
        insn->combinators[index].bind = operand;
    } else {
        insn->operands[index] = operand;
    }

    // Re-add other operands' vars to this insn (in case a var is used in two operands, one of which was just replaced).
    if (insn->type == IR_INSN_COMBINATOR) {
        for (size_t i = 0; i < insn->combinators_len; i++) {
            if (i != index) {
                FOR_OPERAND_VARS(insn->combinators[i].bind, var, set_add(&var->used_at, insn););
            }
        }
    } else {
        for (size_t i = 0; i < insn->operands_len; i++) {
            if (i != index) {
                FOR_OPERAND_VARS(insn->operands[i], var, set_add(&var->used_at, insn););
            }
        }
    }
}

// Set an IR instruction's return variable by index.
void ir_insn_set_return(ir_insn_t *insn, size_t index, ir_var_t *var) {
    assert(index < insn->returns_len);
    if (insn->returns[index]) {
        set_remove(&insn->returns[index]->assigned_at, insn);
    }
    insn->returns[index] = var;
    if (var) {
        assert(!set_contains(&var->assigned_at, insn));
        set_add(&var->assigned_at, insn);
    }
}



// Is a jump, branch or return?
static bool ir_is_jbr(ir_insn_t const *insn) {
    return insn->type == IR_INSN_BRANCH || insn->type == IR_INSN_JUMP || insn->type == IR_INSN_RETURN;
}

// Is forbidden after a jump, branch or return?
static bool ir_not_after_jbr(ir_insn_t const *next) {
    return next->type != IR_INSN_BRANCH && next->type != IR_INSN_JUMP && next->type != IR_INSN_RETURN
           && next->type != IR_INSN_MACHINE;
}

// Helper function that emplaces a new IR instruction at the desired location.
static void ir_emplace_insn(ir_insnloc_t loc, ir_insn_t *insn) {
    ir_insn_t *prev = NULL;
    ir_insn_t *next = NULL;
    switch (loc.type) {
        case IR_INSNLOC_APPEND_CODE: {
            prev = container_of(loc.code->insns.tail, ir_insn_t, node);
        } break;
        case IR_INSNLOC_AFTER_INSN: {
            prev = loc.insn;
            next = container_of(loc.insn->node.next, ir_insn_t, node);
        } break;
        case IR_INSNLOC_BEFORE_INSN: {
            prev = container_of(loc.insn->node.previous, ir_insn_t, node);
            next = loc.insn;
        } break;
    }

    // IR precondition assertions.
    if ((ir_is_jbr(insn) && next && ir_not_after_jbr(next)) || (ir_not_after_jbr(insn) && prev && ir_is_jbr(prev))) {
        fprintf(stderr, "[BUG] Cannot place IR jump, branch or return before an instruction not in that list\n");
        abort();
    }

    switch (loc.type) {
        case IR_INSNLOC_APPEND_CODE: {
            insn->code = loc.code;
            dlist_append(&loc.code->insns, &insn->node);
        } break;
        case IR_INSNLOC_AFTER_INSN: {
            insn->code = loc.insn->code;
            dlist_insert_after(&loc.insn->code->insns, &loc.insn->node, &insn->node);
        } break;
        case IR_INSNLOC_BEFORE_INSN: {
            insn->code = loc.insn->code;
            dlist_insert_before(&loc.insn->code->insns, &loc.insn->node, &insn->node);
        } break;
    }
}

// Helper function to allocate an `ir_insn_t`.
static ir_insn_t *alloc_ir_insn(size_t operands_len, size_t returns_len) {
    ir_insn_t *insn    = strong_calloc(1, sizeof(ir_insn_t));
    insn->operands     = strong_calloc(operands_len, sizeof(ir_operand_t));
    insn->operands_len = operands_len;
    insn->returns      = strong_calloc(returns_len, sizeof(ir_var_t *));
    insn->returns_len  = returns_len;
    return insn;
}

// Helper function for creating an `ir_insn_t`.
static ir_insn_t *
    ir_create_insn(ir_insnloc_t loc, ir_insn_type_t type, ir_var_t *dest, size_t operands_len, ir_operand_t *operands) {
    ir_insn_t *insn    = strong_calloc(1, sizeof(ir_insn_t));
    insn->type         = type;
    insn->operands     = operands;
    insn->operands_len = operands_len;
    if (dest) {
        insn->returns     = strong_malloc(sizeof(ir_var_t *));
        insn->returns_len = 1;
    }
    if (dest != NULL) {
        if (ir_insnloc_code(loc)->func->enforce_ssa && (dest->assigned_at.len || dest->arg_index >= 0)) {
            fprintf(stderr, "[BUG] SSA IR variable %%%s assigned twice\n", dest->name);
            abort();
        }
        set_add(&dest->assigned_at, insn);
        insn->returns[0] = dest;
    }
    for (size_t i = 0; i < operands_len; i++) {
        ir_mark_used(insn->operands[i], insn);
        if (insn->operands[i].type == IR_OPERAND_TYPE_MEM && insn->operands[i].mem.rel_type == IR_MEMREL_SYM) {
            insn->operands[i].mem.base_sym = strong_strdup(insn->operands[i].mem.base_sym);
        }
    }
    ir_emplace_insn(loc, insn);
    return insn;
}

// Helper function for creating an `ir_insn_t`.
static ir_insn_t *ir_create_insn_va(ir_insnloc_t loc, ir_insn_type_t type, ir_var_t *dest, size_t operands_len, ...) {
    ir_operand_t *operands = strong_calloc(operands_len, sizeof(ir_operand_t));
    va_list       l;
    va_start(l, operands_len);
    for (size_t i = 0; i < operands_len; i++) {
        operands[i] = va_arg(l, ir_operand_t);
    }
    va_end(l);
    return ir_create_insn(loc, type, dest, operands_len, operands);
}

// Add a combinator function to a code block.
// Takes ownership of the `from` array.
ir_insn_t *ir_add_combinator(ir_insnloc_t loc, ir_var_t *dest, size_t from_len, ir_combinator_t *from) {
    ir_insn_t *insn       = alloc_ir_insn(0, 1);
    insn->type            = IR_INSN_COMBINATOR;
    insn->combinators_len = from_len;
    insn->combinators     = from;
    insn->returns[0]      = dest;
    if (ir_insnloc_code(loc)->func->enforce_ssa && (dest->assigned_at.len || dest->arg_index >= 0)) {
        fprintf(stderr, "[BUG] SSA IR variable %%%s assigned twice\n", dest->name);
        abort();
    }
    for (size_t i = 0; i < from_len; i++) {
        if (ir_operand_prim(from[i].bind) != dest->prim_type) {
            fprintf(stderr, "[BUG] IR phi has conflicting bind and return types\n");
            abort();
        }
        ir_mark_used(from[i].bind, insn);
    }
    set_add(&dest->assigned_at, insn);
    ir_emplace_insn(loc, insn);
    return insn;
}

// Add an expression to a code block.
ir_insn_t *ir_add_expr1(ir_insnloc_t loc, ir_var_t *dest, ir_op1_type_t oper, ir_operand_t operand) {
    assert(oper < IR_N_OP1);
    if (oper == IR_OP1_snez || oper == IR_OP1_seqz) {
        if (dest->prim_type != IR_PRIM_bool) {
            fprintf(stderr, "[BUG] IR %s must return a boolean\n", ir_op1_names[oper]);
            abort();
        }
    } else if (oper != IR_OP1_mov) {
        if (ir_operand_prim(operand) != dest->prim_type) {
            fprintf(stderr, "[BUG] IR expr1 has conflicting operand and return types\n");
            abort();
        }
    }
    ir_insn_t *insn = ir_create_insn_va(loc, IR_INSN_EXPR1, dest, 1, operand);
    insn->op1       = oper;
    return insn;
}

// Add an expression to a code block.
ir_insn_t *ir_add_expr2(ir_insnloc_t loc, ir_var_t *dest, ir_op2_type_t oper, ir_operand_t lhs, ir_operand_t rhs) {
    assert(oper < IR_N_OP2);
    ir_prim_t lhs_prim = ir_operand_prim(lhs);
    ir_prim_t rhs_prim = ir_operand_prim(rhs);
    if (oper >= IR_OP2_sgt && oper <= IR_OP2_sne) {
        if (lhs_prim != rhs_prim) {
            fprintf(stderr, "[BUG] IR expr2 has conflicting operand types\n");
            abort();
        }
        if (dest->prim_type != IR_PRIM_bool) {
            fprintf(stderr, "[BUG] IR expr2 should be returning IR_PRIM_bool\n");
            abort();
        }
    } else {
        if (lhs_prim != dest->prim_type) {
            fprintf(stderr, "[BUG] IR expr2 has conflicting operand and return types\n");
            abort();
        }
        if (rhs_prim != dest->prim_type) {
            fprintf(stderr, "[BUG] IR expr2 has conflicting operand and return types\n");
            abort();
        }
    }
    ir_insn_t *insn = ir_create_insn_va(loc, IR_INSN_EXPR2, dest, 2, lhs, rhs);
    insn->op2       = oper;
    return insn;
}


// Add a load effective address of a stack frame to a code block.
ir_insn_t *ir_add_lea_stack(ir_insnloc_t loc, ir_var_t *dest, ir_frame_t *frame, uint64_t offset) {
    return ir_create_insn_va(
        loc,
        IR_INSN_LEA,
        dest,
        1,
        IR_OPERAND_MEM(IR_MEMREF(IR_PRIM_u8, IR_BADDR_FRAME(frame), .offset = offset))
    );
}

// Add a load effective address of a symbol to a code block.
ir_insn_t *ir_add_lea_symbol(ir_insnloc_t loc, ir_var_t *dest, char const *symbol, uint64_t offset) {
    return ir_create_insn_va(
        loc,
        IR_INSN_LEA,
        dest,
        1,
        IR_OPERAND_MEM(IR_MEMREF(IR_PRIM_u8, IR_BADDR_SYM((char *)symbol), .offset = offset))
    );
}

// Add a load effective address.
ir_insn_t *ir_add_lea(ir_insnloc_t loc, ir_var_t *dest, ir_memref_t memref) {
    return ir_create_insn_va(loc, IR_INSN_LEA, dest, 1, IR_OPERAND_MEM(memref));
}

// Add a memory load to a code block.
ir_insn_t *ir_add_load(ir_insnloc_t loc, ir_var_t *dest, ir_operand_t addr) {
    return ir_create_insn_va(loc, IR_INSN_LOAD, dest, 1, addr);
}

// Add a memory store to a code block.
ir_insn_t *ir_add_store(ir_insnloc_t loc, ir_operand_t src, ir_operand_t addr) {
    return ir_create_insn_va(loc, IR_INSN_STORE, NULL, 2, addr, src);
}


// Add a function call.
ir_insn_t *ir_add_call(
    ir_insnloc_t        loc,
    ir_memref_t         to,
    size_t              returns_len,
    ir_var_t          **returns,
    size_t              operands_len,
    ir_operand_t const *operands
) {
    ir_operand_t *operands_copy = strong_calloc(1 + operands_len, sizeof(ir_operand_t));
    operands_copy[0]            = IR_OPERAND_MEM(to);
    memcpy(operands_copy + 1, operands, operands_len * sizeof(ir_operand_t));

    ir_var_t **returns_copy = strong_calloc(returns_len, sizeof(void *));
    memcpy(returns_copy, returns, returns_len * sizeof(void *));

    return ir_create_insn(loc, IR_INSN_CALL, NULL, 1 + operands_len, operands_copy);
}

// Add an unconditional jump.
ir_insn_t *ir_add_jump(ir_insnloc_t loc, ir_code_t *to) {
    ir_insn_t *insn
        = ir_create_insn_va(loc, IR_INSN_JUMP, NULL, 1, IR_OPERAND_MEM(IR_MEMREF(IR_PRIM_u8, IR_BADDR_CODE(to))));
    set_add(&ir_insnloc_code(loc)->succ, to);
    set_add(&to->pred, ir_insnloc_code(loc));
    return insn;
}

// Add a conditional branch.
ir_insn_t *ir_add_branch(ir_insnloc_t loc, ir_operand_t cond, ir_code_t *to) {
    if (ir_operand_prim(cond) != IR_PRIM_bool) {
        fprintf(stderr, "[BUG] IR branch requires a boolean condition\n");
        abort();
    }
    ir_insn_t *insn = ir_create_insn_va(
        loc,
        IR_INSN_BRANCH,
        NULL,
        2,
        IR_OPERAND_MEM(IR_MEMREF(IR_PRIM_u8, IR_BADDR_CODE(to))),
        cond
    );
    set_add(&ir_insnloc_code(loc)->succ, to);
    set_add(&to->pred, ir_insnloc_code(loc));
    return insn;
}

// Add a return without value.
ir_insn_t *ir_add_return0(ir_insnloc_t loc) {
    return ir_create_insn_va(loc, IR_INSN_RETURN, NULL, 0);
}

// Add a return with value.
ir_insn_t *ir_add_return1(ir_insnloc_t loc, ir_operand_t value) {
    // TODO: Again, the IR supports multiple-return but this API does not.
    return ir_create_insn_va(loc, IR_INSN_RETURN, NULL, 1, value);
}

// Add a machine instruction.
ir_insn_t *ir_add_mach_insn(
    ir_insnloc_t loc, ir_var_t *dest, insn_proto_t const *proto, size_t operands_len, ir_operand_t const *operands
) {
    ir_operand_t *operands_copy = strong_calloc(operands_len, sizeof(ir_operand_t));
    memcpy(operands_copy, operands, operands_len * sizeof(ir_operand_t));
    ir_insn_t *insn = ir_create_insn(loc, IR_INSN_MACHINE, dest, operands_len, operands_copy);
    insn->prototype = proto;
    return insn;
}
