
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "backend.h"
#include "ir_types.h"
#include "map.h"
#include "set.h"



// Information from liveness analisys that is needed for register allocation.
typedef struct ra_lifetime ra_lifetime_t;
// A graph-coloring problem node as used for register selection.
typedef struct ra_node     ra_node_t;



// Information from liveness analisys that is needed for register allocation.
struct ra_lifetime {
    // IR variables that are alive at the same time as this one.
    set_t siblings;
};

// A graph-coloring problem node as used for register selection.
struct ra_node {
    // IR variable linked to this node, if any.
    // The `clobber` instruction generates nodes without variables, one each per register clobbered.
    ir_var_t *var;
    // Physical register assigned to this node.
    size_t    regno;
    // Links to nodes that are alive at the same time.
    set_t     links;
};



// Perform liveness analisys for all variables in a function.
// Returns a map of `ir_var_t *` -> `ra_lifetime_t`.
// Assumes at least trivial dead-code elimination has been done.
map_t ra_liveness(ir_func_t const *func);

// Perform resource allocation for the given function.
// Allocates registers to IR variables and frame offsets ir IR stack frames.
void regalloc(backend_profile_t *profile, ir_func_t *func);
