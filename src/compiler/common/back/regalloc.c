
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "regalloc.h"

#include "arrays.h"
#include "backend.h"
#include "bitops.h"
#include "ir.h"
#include "ir_types.h"
#include "lilycc_malloc.h"
#include "list.h"
#include "map.h"
#include "set.h"
#include "unreachable.h"

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>



// Helper for `ra_liveness` that links variables together as being live at the same time.
// `ra_vars` - map of `ir_var_t *` -> `ra_node_t *`.
// `vars` - set of `ir_var_t *`.
static void link_vars(map_t *ra_vars, set_t const *vars) {
    set_foreach(ir_var_t, var1, vars) {
        ra_node_t *node1 = map_get(ra_vars, var1);
        if (!node1) {
            continue;
        }
        assert(node1->var == var1);
        set_foreach(ir_var_t, var2, vars) {
            if (var1 == var2) {
                continue;
            }
            ra_node_t *node2 = map_get(ra_vars, var2);
            if (!node2) {
                continue;
            }
            assert(node2->var == var2);
            set_add(&node1->links, node2);
        }
    }
}

// Helper for `ra_liveness` that links IR variables to physical registers.
static void link_regs(ra_node_t *node, map_t const *ra_vars, set_t const *vars) {
    assert(node->var == NULL);
    set_foreach(ir_var_t, var, vars) {
        ra_node_t *reg_node = map_get(ra_vars, var);
        assert(reg_node != NULL);
        set_add(&node->links, reg_node);
    }
}

// Get or allocate `ra_node_t` for a physical register.
static ra_node_t *alloc_reg_node(set_t *ra_nodes, map_t *ra_regs, regno_t regno) {
    ra_node_t *node = map_get(ra_regs, (void *)(size_t)regno);
    if (node) {
        return node;
    }

    node        = lilycc_malloc(sizeof(ra_node_t));
    node->links = PTR_SET_EMPTY;
    node->regno = regno;
    node->var   = NULL;

    map_set(ra_regs, (void *)(size_t)regno, node);
    set_add(ra_nodes, node);

    return node;
}

// Perform liveness analisys for variables in a function.
// Assumes at least trivial dead-code elimination has been done.
// Returns a map of `ir_var_t *` -> `ra_node_t *`.
ra_nodes_t ra_liveness(ir_func_t const *func) {
    // TODO: This may need to be changed to increase efficiency.

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

        // Variables referenced by this node.
        set_t use;
        // Variables defined by this node.
        set_t def;

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
        lt_nodes = lilycc_calloc(lt_nodes_len, sizeof(lt_node_t));
        for (size_t x = 0; x < lt_nodes_len; x++) {
            lt_nodes[x].in  = PTR_SET_EMPTY;
            lt_nodes[x].out = PTR_SET_EMPTY;
            lt_nodes[x].use = PTR_SET_EMPTY;
            lt_nodes[x].def = PTR_SET_EMPTY;
        }

        size_t i = 0;
        dlist_foreach_node(ir_code_t const, code, &func->code_list) {
            dlist_foreach_node(ir_insn_t const, insn, &code->insns) {
                lt_nodes[i].insn = insn;
                map_set(&insn_to_node, insn, &lt_nodes[i]);

                // Variables referenced.
                assert(insn->type != IR_INSN_COMBINATOR);
                for (size_t x = 0; x < insn->operands_len; x++) {
                    IR_FOR_OPERAND_VARS(
                        insn->operands[x],
                        var,
                        if (var->assigned_at.len) {
                            // Only keep if the var is assigned at least once.
                            // If undefined but used, we will simply accept any arbitrary register chosen.
                            set_add(&lt_nodes[i].use, var);
                        }
                    );
                }

                // Variables defined.
                for (size_t x = 0; x < insn->returns_len; x++) {
                    if (insn->returns[x].type == IR_RETVAL_TYPE_VAR) {
                        set_add(&lt_nodes[i].def, insn->returns[x].dest_var);
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
    lt_node_t **dirty     = lilycc_calloc(lt_nodes_len, sizeof(lt_node_t *));
    for (size_t i = 0; i < lt_nodes_len; i++) {
        dirty[i]          = &lt_nodes[i];
        lt_nodes[i].dirty = true;
    }

    // Iterate until no mode nodes are dirty.
    while (dirty_len) {
        // Pop the first dirty node.
        lt_node_t *node;
        array_remove(dirty, sizeof(lt_node_t *), dirty_len, &node, 0);
        node->dirty = false;
        dirty_len--;

        // Propagate liveness of variables.
        set_foreach(ir_var_t, var, &node->def) {
            node->dirty |= set_add(&node->out, var);
        }
        set_foreach(ir_var_t, var, &node->out) {
            if (!set_contains(&node->def, var)) {
                node->dirty |= set_add(&node->in, var);
            }
        }
        node->dirty |= set_addall(&node->in, &node->use) > 0;

        // Mark predecessors as dirty if needed.
        for (size_t i = 0; i < node->pred_len; i++) {
            lt_node_t *pred       = node->pred[i];
            bool       pred_dirty = set_addall(&pred->out, &node->in) > 0;
            if (pred_dirty && !pred->dirty) {
                dirty[dirty_len++] = pred;
                pred->dirty        = true;
            }
        }

        if (node->dirty) {
            dirty[dirty_len++] = node;
        }
    }

    set_t ra_nodes = PTR_SET_EMPTY;
    map_t ra_vars  = PTR_MAP_EMPTY;

    // Interference information per IR variable (that isn't unused).
    dlist_foreach_node(ir_var_t, var, &func->vars_list) {
        ra_node_t *node = lilycc_calloc(1, sizeof(ra_node_t));
        node->links     = PTR_SET_EMPTY;
        node->regno     = REGNO_NONE;
        node->var       = var;
        map_set(&ra_vars, var, node);
        set_add(&ra_nodes, node);
    }
    for (size_t i = 0; i < lt_nodes_len; i++) {
        link_vars(&ra_vars, &lt_nodes[i].in);
        link_vars(&ra_vars, &lt_nodes[i].out);
    }

    // Pre-colored interference graph nodes.
    map_t ra_regs = PTR_MAP_EMPTY;
    for (size_t i = 0; i < lt_nodes_len; i++) {
        ir_insn_t const *insn = lt_nodes[i].insn;
        assert(insn->type != IR_INSN_COMBINATOR);
        for (size_t x = 0; x < insn->returns_len; x++) {
            if (insn->returns[x].type == IR_RETVAL_TYPE_REG) {
                ra_node_t *node = alloc_reg_node(&ra_nodes, &ra_regs, insn->returns[x].dest_regno);
                link_regs(node, &ra_vars, &lt_nodes[i].out);
            }
        }
        for (size_t x = 0; x < insn->operands_len; x++) {
            IR_FOR_OPERAND_REGS(insn->operands[x], regno, {
                ra_node_t *node = alloc_reg_node(&ra_nodes, &ra_regs, regno);
                link_regs(node, &ra_vars, &lt_nodes[i].in);
            });
        }
    }

    // Cleanup.
    for (size_t i = 0; i < lt_nodes_len; i++) {
        lt_node_t *node = &lt_nodes[i];
        set_clear(&node->in);
        set_clear(&node->out);
        set_clear(&node->use);
        set_clear(&node->def);
        lilycc_free(node->pred);
    }
    map_clear(&ra_regs);
    map_clear(&insn_to_node);
    lilycc_free(lt_nodes);
    lilycc_free(dirty);

    return (ra_nodes_t){
        .nodes  = ra_nodes,
        .by_var = ra_vars,
    };
}

// Delete an `ra_nodes_t`.
void ra_nodes_destroy(ra_nodes_t nodes) {
    set_foreach(ra_node_t, node, &nodes.nodes) {
        set_clear(&node->links);
        lilycc_free(node);
    }
    set_clear(&nodes.nodes);
    map_clear(&nodes.by_var);
}

// Count IR instructions until first use.
static int64_t time_to_use(ir_var_t *var) {
    if (var->used_at.len == 0 || var->assigned_at.len == 0) {
        return INT64_MAX;
    }

    set_t covered = PTR_SET_EMPTY;
    set_t work    = PTR_SET_EMPTY;
    set_addall(&covered, &var->assigned_at);
    set_addall(&work, &var->assigned_at);

    int64_t time = 0;
    while (work.len) {
        set_t next = PTR_SET_EMPTY;
        set_foreach(ir_insn_t, insn, &work) {
            if (set_contains(&var->used_at, insn)) {
                set_clear(&next);
                set_clear(&work);
                set_clear(&covered);
                return time;
            }
            if (insn->node.next) {
                set_add(&next, insn->node.next);
            } else {
                set_foreach(ir_code_t, succ, &insn->code->succ) {
                    set_add(&next, ir_first_in_code(succ));
                }
            }
        }
        set_clear(&work);
        work = next;
        set_removeall(&work, &covered);
        set_addall(&covered, &work);

        time++;
    }
    set_clear(&covered);

    return INT64_MAX;
}

// Estimate node spill cost.
void ra_spill_cost(backend_profile_t *profile, ra_node_t *node) {
    (void)profile;
    if (node->var == NULL) {
        // Can't spill precolored nodes.
        node->spill_cost = INT64_MAX;
        return;
    }
    if (node->var->no_spill) {
        // Was already spilled previously.
        node->spill_cost = INT64_MAX;
        return;
    }

    int64_t ttu = time_to_use(node->var);
    if (ttu == INT64_MAX) {
        // Never used; no need to spill.
        node->spill_cost = INT64_MAX;
        return;
    }
    if (ttu < 1 && node->var->used_at.len <= 1) {
        // Variable used immediately and only once; don't spill.
        node->spill_cost = INT64_MAX;
        return;
    }
    if (ttu < 1) {
        ttu = 1;
    }

    // TODO: Optimizer settings could change the heuristic formula here.
    node->spill_cost = 1024 * (int64_t)node->var->used_at.len / ttu;
}

static void ra_spill(backend_profile_t *profile, ra_nodes_t nodes) {
    int64_t   spill_cost = INT64_MAX;
    ir_var_t *to_spill   = NULL;

    map_foreach_value(ra_node_t, node, &nodes.by_var) {
        ra_spill_cost(profile, node);
        if (node->spill_cost < spill_cost) {
            to_spill   = node->var;
            spill_cost = node->spill_cost;
        }
    }

    if (spill_cost == INT64_MAX) {
        fprintf(stderr, "BUG: Cannot select IR variable to spill\n");

        set_foreach(ra_node_t, node, &nodes.nodes) {
            if (node->var && node->regno == REGNO_NONE) {
                fprintf(stderr, "CULPRIT: %%%s\n", node->var->name);
            }
        }

        set_foreach(ra_node_t, node, &nodes.nodes) {
            if (node->var) {
                fprintf(stderr, "%%%s", node->var->name);
            }
            if (node->regno != REGNO_NONE) {
                fprintf(stderr, ":$%s", profile->gpr_names[node->regno]);
            }
            if (node->links.len > 0) {
                fputs(" <=", stderr);
            }
            set_foreach(ra_node_t, link, &node->links) {
                fprintf(stderr, " ");
                if (link->var) {
                    fprintf(stderr, "%%%s", link->var->name);
                } else {
                    assert(link->regno != REGNO_NONE);
                    fprintf(stderr, ":$%s", profile->gpr_names[link->regno]);
                }
            }
            fprintf(stderr, "\n");
        }

        abort();
    }

    uint64_t    size     = ir_prim_sizes[to_spill->orig_prim_type];
    ir_frame_t *frame    = ir_frame_create(to_spill->func, size, size, NULL);
    set_t       orig_def = PTR_SET_EMPTY;
    set_t       orig_use = PTR_SET_EMPTY;
    set_addall(&orig_def, &to_spill->assigned_at);
    set_addall(&orig_use, &to_spill->used_at);

    set_foreach(ir_insn_t, insn, &orig_def) {
        profile->backend->ra_spill_store(profile, IR_AFTER_INSN(insn), IR_OPERAND_VAR(to_spill), frame, 0);
    }
    set_foreach(ir_insn_t, insn, &orig_use) {
        profile->backend->ra_spill_load(profile, IR_BEFORE_INSN(insn), IR_RETVAL_VAR(to_spill), frame, 0);
    }
    to_spill->no_spill = true;

    set_clear(&orig_def);
    set_clear(&orig_use);

    ra_nodes_destroy(nodes);
}

static void pop_node(ra_nodes_t *nodes, ra_node_t *node) {
    set_foreach(ra_node_t, linked, &node->links) {
        set_remove(&linked->links, node);
    }
    if (node->var) {
        map_remove(&nodes->by_var, node->var);
    }
    set_remove(&nodes->nodes, node);
}

static void push_node(ra_nodes_t *nodes, ra_node_t *node) {
    set_foreach(ra_node_t, linked, &node->links) {
        set_add(&linked->links, node);
    }
    if (node->var) {
        map_set(&nodes->by_var, node->var, node);
    }
    set_add(&nodes->nodes, node);
}

static void choose_one_reg(backend_profile_t *profile, ra_node_t *node, uint32_t *callee_save, bool use_frame_ptr) {
    if (node->regno != REGNO_NONE) {
        return;
    }
    assert(node->var != NULL);

    // TODO: Try registers that would reduce copies first.
    // TODO: Heuristic-based register choice.

    bool any_match  = false;
    bool weak_match = false;
    for (regno_t regno = 0; regno < profile->gpr_count; regno++) {
        if (use_frame_ptr && regno == profile->frame_ptr_reg) {
            continue;
        }

        // Check regclass matches.
        regclass_t regclass = profile->gpr_classes[regno];
        switch (node->var->prim_type) {
            case IR_PRIM_bool:
            case IR_PRIM_s8:
            case IR_PRIM_u8:
                if (!regclass.int8) {
                    continue;
                }
                break;
            case IR_PRIM_s16:
            case IR_PRIM_u16:
                if (!regclass.int16) {
                    continue;
                }
                break;
            case IR_PRIM_s32:
            case IR_PRIM_u32:
                if (!regclass.int32) {
                    continue;
                }
                break;
            case IR_PRIM_s64:
            case IR_PRIM_u64:
                if (!regclass.int64) {
                    continue;
                }
                break;
            case IR_PRIM_s128:
            case IR_PRIM_u128:
                if (!regclass.int128) {
                    continue;
                }
                break;
            case IR_PRIM_f32:
                if (!regclass.f32) {
                    continue;
                }
                break;
            case IR_PRIM_f64:
                if (!regclass.f64) {
                    continue;
                }
                break;
            case IR_N_PRIM: UNREACHABLE();
        }
        weak_match = regclass.callee_save;
        any_match  = true;

        // Check register is free in neighbors.
        set_foreach(ra_node_t, node, &node->links) {
            if (node->regno == regno) {
                goto next;
            }
        }

        // Register successfully chosen.
        node->regno = regno;
        if (!weak_match) {
            return;
        }

    next:;
    }

    if (!any_match) {
        fprintf(stderr, "BUG: No regclass matches primitive type %s\n", ir_prim_names[node->var->prim_type]);
        abort();
    }

    if (profile->gpr_classes[node->regno].callee_save) {
        callee_save[node->regno / 32] |= 1 << (node->regno % 32);
    }
}

static void ra_callee_save_regs_impl(
    backend_profile_t *profile, ir_frame_t *frame, uint32_t const *callee_save, ir_insnloc_t loc, bool is_restore
) {
    int64_t word_size = lily_bits_absolute(profile->gpr_bits) / 8;
    int64_t offset    = 0;
    for (size_t i = 0; i < (profile->gpr_count + 31u) / 32u; i++) {
        uint32_t tmp = callee_save[i];
        while (tmp) {
            regno_t reg  = lilycc_ctz32(tmp);
            tmp         ^= 1 << reg;

            if (is_restore) {
                profile->backend->ra_spill_load(profile, loc, IR_RETVAL_REG(reg), frame, offset);
            } else {
                profile->backend->ra_spill_store(profile, loc, IR_OPERAND_REG(reg), frame, offset);
            }
            offset += word_size;
        }
    }
}

static void
    ra_callee_save_regs(backend_profile_t *profile, ir_func_t *func, ir_frame_t *frame, uint32_t const *callee_save) {
    ra_callee_save_regs_impl(profile, frame, callee_save, IR_PREPEND(func->entry), false);

    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        dlist_foreach_node_rev(ir_insn_t, insn, &code->insns) {
            if (insn->type == IR_INSN_EPILOGUE) {
                ra_callee_save_regs_impl(profile, frame, callee_save, IR_BEFORE_INSN(insn), true);
                goto next;
            }
        }
    next:;
    }
}

// Perform resource allocation for the given function.
// Allocates registers to IR variables and frame offsets ir IR stack frames.
void regalloc(backend_profile_t *profile, ir_func_t *func) {
    uint32_t *callee_save = lilycc_calloc((profile->gpr_count + 31u) / 32u, sizeof(uint32_t));

    // K values per type of IR variable.
    regno_t k_values[IR_N_PRIM] = {0};
    for (regno_t i = 0; i < profile->gpr_count; i++) {
        regclass_t class = profile->gpr_classes[i];
        if (i == profile->frame_ptr_reg && func->use_frame_ptr) {
            continue;
        }
        if (class.f64) {
            k_values[IR_PRIM_f64]++;
        }
        if (class.f32) {
            k_values[IR_PRIM_f32]++;
        }
        if (class.int128) {
            k_values[IR_PRIM_u128]++;
            k_values[IR_PRIM_s128]++;
        }
        if (class.int64) {
            k_values[IR_PRIM_u64]++;
            k_values[IR_PRIM_s64]++;
        }
        if (class.int32) {
            k_values[IR_PRIM_u32]++;
            k_values[IR_PRIM_s32]++;
        }
        if (class.int16) {
            k_values[IR_PRIM_u16]++;
            k_values[IR_PRIM_s16]++;
        }
        if (class.int8) {
            k_values[IR_PRIM_u8]++;
            k_values[IR_PRIM_s8]++;
            k_values[IR_PRIM_bool]++;
        }
    }

    ra_nodes_t nodes;
again:
    nodes = ra_liveness(func);

    vec_ra_node_t stack = {0};

    // Step 1: simplify the graph until empty.
unconstrained: // Unconstrained, uncolored nodes.
    set_foreach(ra_node_t, node, &nodes.nodes) {
        if (!node->var || node->regno != REGNO_NONE) {
            continue;
        }
        if (node->links.len < k_values[node->var->prim_type]) {
            pop_node(&nodes, node);
            vec_push(&stack, node);
            goto unconstrained;
        }
    }
constrained: // Constrained, uncolored nodes.
    set_foreach(ra_node_t, node, &nodes.nodes) {
        if (node->regno != REGNO_NONE) {
            pop_node(&nodes, node);
            vec_push(&stack, node);
            goto constrained;
        }
    }
    // All remaining nodes, if any, are pre-colored.
    while (nodes.nodes.len) {
        ra_node_t *node = set_next(&nodes.nodes, NULL)->value;
        pop_node(&nodes, node);
        vec_push(&stack, node);
    }

    // Step 2: reinsert nodes into the graph and color as we go.
    // This is also why the pre-colored nodes are popped last; no possible color conflict.
    bool must_spill = false;
    while (stack.len) {
        ra_node_t *node = vec_pop(&stack);
        push_node(&nodes, node);
        choose_one_reg(profile, node, callee_save, func->use_frame_ptr);
        must_spill |= node->regno == REGNO_NONE;
    }

    if (must_spill) {
        // Must spill a node.
        ra_spill(profile, nodes);
        memset(callee_save, 0, (profile->gpr_count + 31u) / 32u * sizeof(uint32_t));
        goto again;
    }

    // Replace all IR variables by their register numbers, and delete the variables.
    map_foreach_value(ra_node_t, node, &nodes.by_var) {
        assert(node->regno != REGNO_NONE);
        set_foreach(ir_insn_t, insn, &node->var->assigned_at) {
            for (size_t i = 0; i < insn->returns_len; i++) {
                if (insn->returns[i].type == IR_RETVAL_TYPE_VAR && insn->returns[i].dest_var == node->var) {
                    insn->returns[i].type       = IR_RETVAL_TYPE_REG;
                    insn->returns[i].dest_regno = node->regno;
                }
            }
        }
        set_clear(&node->var->assigned_at);
        ir_var_replace(node->var, IR_OPERAND_REG(node->regno));
    }

    // Create spill frames for callee-saved registers.
    uint64_t count = 0;
    for (size_t i = 0; i < (profile->gpr_count + 31u) / 32u; i++) {
        count += lilycc_popcount32(callee_save[i]);
    }
    if (count > 0) {
        uint64_t    gpr_size    = lily_bits_absolute(profile->gpr_bits) / 8;
        ir_frame_t *spill_frame = ir_frame_create(func, count * gpr_size, gpr_size, NULL);
        ra_callee_save_regs(profile, func, spill_frame, callee_save);
    }

    lilycc_free(callee_save);
    ra_nodes_destroy(nodes);
}
