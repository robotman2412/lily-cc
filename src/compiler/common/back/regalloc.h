
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "backend.h"
#include "ir_types.h"
#include "set.h"
#include "vec.h"



// Represents one IR variable and/or physical register.
typedef struct ra_node  ra_node_t;
// A graph-coloring problem node as used for register selection.
typedef struct ra_nodes ra_nodes_t;

VEC_TYPE_DEF(vec_ra_node_t, ra_node_t *);



// Represents one IR variable and/or physical register.
struct ra_node {
    // IR variable linked to this node, if any.
    // The `clobber` instruction generates nodes without variables, one each per register clobbered.
    ir_var_t *var;
    // Physical register assigned to this node.
    regno_t   regno;
    // Links to `ra_node_t` nodes that are alive at the same time.
    set_t     links;
    // Node spill cost heuristic.
    int64_t   spill_cost;
};

// A graph-coloring problem node as used for register selection.
struct ra_nodes {
    // Set of all nodes.
    set_t nodes;
    // Non-owning map of nodes by IR variable.
    map_t by_var;
};



// Perform liveness analisys for all variables in a function.
// Assumes at least trivial dead-code elimination has been done.
ra_nodes_t ra_liveness(ir_func_t const *func);

// Delete an `ra_nodes_t`.
void ra_nodes_destroy(ra_nodes_t nodes);

// Estimate node spill cost.
void ra_spill_cost(backend_profile_t *profile, ra_node_t *node);

// Perform resource allocation for the given function.
// Allocates registers to IR variables and frame offsets ir IR stack frames.
void regalloc(backend_profile_t *profile, ir_func_t *func);
