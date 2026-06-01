
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "regalloc.h"

#include "arrays.h"
#include "ir.h"
#include "ir_types.h"
#include "list.h"
#include "map.h"
#include "set.h"
#include "strong_malloc.h"

#include <assert.h>
#include <stdint.h>



// Helper for `ra_liveness` that links variables together as being live at the same time.
// `ra_vars` - map of `ir_var_t *` -> `ra_node_t *`.
// `vars` - set of `ir_var_t *`.
static void link_vars(map_t *ra_vars, set_t const *vars) {
    set_foreach(ir_var_t, var1, vars) {
        ra_node_t *node1 = map_get(ra_vars, var1);
        if (!node1) {
            continue;
        }
        set_foreach(ir_var_t, var2, vars) {
            if (var1 == var2) {
                continue;
            }
            ra_node_t *node2 = map_get(ra_vars, var2);
            if (!node2) {
                continue;
            }
            set_add(&node1->links, node2);
        }
    }
}

// Perform liveness analisys for variables in a function.
// Assumes at least trivial dead-code elimination has been done.
void ra_liveness(ir_func_t const *func, ra_node_t **ra_nodes_out, size_t *ra_nodes_len_out) {
    // Lifetime analisys graph node.
    typedef struct lt_node lt_node_t;
    struct lt_node {
        // Associated IR instruction.
        ir_insn_t const *insn;

        // Number of predecessor nodes.
        size_t      pred_len;
        // Predecessor nodes.
        lt_node_t **pred;
        // Successor nodes. The most possible here is two (in case of branch instructions).
        lt_node_t  *succ0, *succ1;

        // Number of variables referenced by this node.
        size_t           use_len;
        // Variables referenced by this node.
        ir_var_t const **use;
        // Number of variables defined by this node.
        size_t           def_len;
        // Variables defined by this node.
        ir_var_t const **def;

        // Variables defined by a predecessor.
        set_t in;
        // Variables referenced by a successor.
        set_t out;

        // Is currently in the dirty set.
        bool dirty;
    };

    size_t     lt_nodes_len = 0;
    lt_node_t *lt_nodes;
    map_t      insn_to_node = PTR_MAP_EMPTY;

    // Allocate all of the nodes.
    {
        dlist_foreach_node(ir_code_t const, code, &func->code_list) {
            lt_nodes_len += code->insns.len;
        }
        lt_nodes = strong_calloc(lt_nodes_len, sizeof(lt_node_t));

        size_t i = 0;
        dlist_foreach_node(ir_code_t const, code, &func->code_list) {
            dlist_foreach_node(ir_insn_t const, insn, &code->insns) {
                lt_nodes[i].insn = insn;
                lt_nodes[i].in   = PTR_SET_EMPTY;
                lt_nodes[i].out  = PTR_SET_EMPTY;
                map_set(&insn_to_node, insn, &lt_nodes[i]);

                // Variables referenced.
                assert(insn->type != IR_INSN_COMBINATOR);
                for (size_t x = 0; x < insn->operands_len; x++) {
                    IR_FOR_OPERAND_VARS(insn->operands[x], var, {
                        array_len_insert_strong(
                            &lt_nodes[i].use,
                            sizeof(ir_var_t const *),
                            &lt_nodes[i].use_len,
                            &var,
                            lt_nodes[i].use_len
                        );
                    });
                }

                // Variables defined.
                for (size_t x = 0; x < insn->returns_len; x++) {
                    if (insn->returns[i].type == IR_RETVAL_TYPE_REG) {
                        array_len_insert_strong(
                            &lt_nodes[i].def,
                            sizeof(ir_var_t const *),
                            &lt_nodes[i].def_len,
                            &insn->returns[i].dest_var,
                            lt_nodes[i].def_len
                        );
                    }
                }

                i++;
            }
        }
    }

    // Establish predecessor-successor relationships.
    for (size_t i = 0; i < lt_nodes_len; i++) {
        ir_insn_t const *insn = lt_nodes[i].insn;

        ir_insn_t const *succ = ir_next_after(insn);
        if (succ) {
            lt_node_t *succ_node = map_get(&insn_to_node, succ);
            lt_nodes[i].succ0    = succ_node;
            array_len_insert_strong(
                &succ_node->pred,
                sizeof(lt_node_t *),
                &succ_node->pred_len,
                (lt_node_t *[]){&lt_nodes[i]},
                succ_node->pred_len
            );
        }

        ir_insn_t const *branch = ir_branch_target(insn);
        if (branch) {
            lt_node_t *branch_node = map_get(&insn_to_node, branch);
            lt_nodes[i].succ1      = branch_node;
            array_len_insert_strong(
                &branch_node->pred,
                sizeof(lt_node_t *),
                &branch_node->pred_len,
                (lt_node_t *[]){&lt_nodes[i]},
                branch_node->pred_len
            );
        }
    }

    // Nodes that may need to be updated.
    size_t      dirty_len = lt_nodes_len;
    lt_node_t **dirty     = strong_calloc(lt_nodes_len, sizeof(lt_node_t *));
    for (size_t i = 0; i < lt_nodes_len; i++) {
        dirty[i] = &lt_nodes[i];
    }

    // Iterate until no mode nodes are dirty.
    while (dirty_len) {
        // Pop the first dirty node.
        lt_node_t *node;
        array_remove(&dirty, sizeof(lt_node_t *), dirty_len, &node, 0);
        node->dirty = false;
        dirty_len--;

        // Propagate liveness of variables.
        set_addall(&node->in, &node->out);
        for (size_t i = 0; i < node->def_len; i++) {
            set_remove(&node->in, node->def[i]);
        }
        for (size_t i = 0; i < node->def_len; i++) {
            set_remove(&node->in, node->def[i]);
        }

        // Mark predecessors as dirty if needed.
        for (size_t i = 0; i < node->pred_len; i++) {
            lt_node_t *pred       = node->pred[i];
            bool       pred_dirty = set_addall(&pred->out, &node->in) > 0;
            if (pred_dirty && !pred->dirty) {
                dirty[dirty_len++] = pred;
                pred->dirty        = true;
            }
        }
    }

    ra_node_t *ra_nodes     = NULL;
    size_t     ra_nodes_len = 0;
    // size_t     ra_nodes_cap = 0;

    // Liveness information per IR variable (that isn't unused).
    map_t ra_vars = PTR_MAP_EMPTY;
    dlist_foreach_node(ir_var_t, var, &func->vars_list) {
        if (var->used_at.len) {
            ra_node_t *node = strong_calloc(1, sizeof(ra_node_t));
            node->links     = PTR_SET_EMPTY;
            node->regno     = SIZE_MAX;
            node->var       = var;
            map_set(&ra_vars, var, node);
        }
    }
    for (size_t i = 0; i < lt_nodes_len; i++) {
        for (size_t x = 0; x < 2; x++) {
            set_t const *vars = x ? &lt_nodes[i].in : &lt_nodes[i].out;
            link_vars(&ra_vars, vars);
        }
    }

    // Cleanup.
    for (size_t i = 0; i < lt_nodes_len; i++) {
        lt_node_t *node = &lt_nodes[i];
        set_clear(&node->in);
        set_clear(&node->out);
        free(node->use);
        free(node->def);
        free(node->pred);
    }
    free(dirty);

    *ra_nodes_out     = ra_nodes;
    *ra_nodes_len_out = ra_nodes_len;
}

// Perform resource allocation for the given function.
// Allocates registers to IR variables and frame offsets ir IR stack frames.
void regalloc(backend_profile_t *profile, ir_func_t *func);
