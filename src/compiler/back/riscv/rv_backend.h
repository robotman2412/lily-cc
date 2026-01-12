
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "backend.h"
#include "rv_misc.h"



// Supported RISC-V ABI types.
typedef enum {
    // 32-bit.
    RV_ABI_ILP32,
    // 32-bit, RVE registers only.
    RV_ABI_ILP32E,
    // 32-bit, float32.
    RV_ABI_ILP32F,
    // 32-bit, float64.
    RV_ABI_ILP32D,
    // 64-bit.
    RV_ABI_LP64,
    // 64-bit, float32.
    RV_ABI_LP64F,
    // 64-bit, float64.
    RV_ABI_LP64D,
} rv_abi_t;



// RISC-V backend profile.
typedef struct rv_profile rv_profile_t;



// RISC-V backend profile.
struct rv_profile {
    // Common backend profile information.
    backend_profile_t base;
    // Selected ABI.
    rv_abi_t          abi;
    // Profile is RV*E.
    bool              is_rve;
    // Which extensions are enabled in this profile.
    bool              ext_enabled[RV_N_EXT];
};



// Create a copy of the default profile for this type of backend.
backend_profile_t *rv_create_profile();
// Delete a profile for this backend.
void               rv_delete_profile(backend_profile_t *profile);
// Prepare backend for codegen stage.
void               rv_init_codegen(backend_profile_t *profile);



// The RISC-V backend.
extern backend_t const   rv_backend;
// Table of RISC-V register names.
extern char const *const rv_reg_names[];
