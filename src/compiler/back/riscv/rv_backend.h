
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "backend.h"
#include "cand_tree.h"
#include "rv_misc.h"



// RISC-V backend profile.
typedef struct rv_profile rv_profile_t;



// RISC-V backend profile.
struct rv_profile {
    // Common backend profile information.
    backend_profile_t base;
    // Profile is RV*E.
    bool              is_rve;
    // Instruction candidate tree.
    cand_tree_t      *cand_tree;
    // Which extensions are enabled in this profile.
    bool              ext_enabled[RV_N_EXT];
};



// Create a copy of the default profile for this type of backend.
backend_profile_t  *rv_create_profile();
// Delete a profile for this backend.
void                rv_delete_profile(backend_profile_t *profile);
// Prepare backend for codegen stage.
void                rv_init_codegen(backend_profile_t *profile);
// Perform instruction selection.
insn_proto_t const *rv_isel(backend_profile_t *profile, ir_insn_t const *ir_insn, ir_operand_t *operands_out);



// The RISC-V backend.
extern backend_t const rv_backend;
