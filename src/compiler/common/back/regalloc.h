
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "backend.h"
#include "ir_types.h"
#include "set.h"



// A graph-coloring problem node as used for register selection.
typedef struct ra_node ra_node_t;



// A graph-coloring problem node as used for register selection.
struct ra_node {
    // IR variable linked to this node, if any.
    // The `clobber` instruction generates nodes without variables, one each per register clobbered.
    ir_var_t *var;
    // Physical register assigned to this node.
    // Set to `SIZE_MAX` if not yet allocated.
    size_t    regno;
    // Links to nodes that are alive at the same time.
    set_t     links;
};



// Perform liveness analisys for all variables in a function.
// Assumes at least trivial dead-code elimination has been done.
void ra_liveness(ir_func_t const *func, ra_node_t **nodes_out, size_t *nodes_len_out);

// Perform resource allocation for the given function.
// Allocates registers to IR variables and frame offsets ir IR stack frames.
void regalloc(backend_profile_t *profile, ir_func_t *func);
