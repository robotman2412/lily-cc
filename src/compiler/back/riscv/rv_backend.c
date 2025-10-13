
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "rv_backend.h"

#include "backend.h"
#include "rv_isel.h"
#include "rv_misc.h"
#include "strong_malloc.h"

// Get the default backend.
backend_t const *backend_default() {
    return &rv_backend;
}



// Create a copy of the default profile for this type of backend.
backend_profile_t *rv_create_profile() {
    rv_profile_t *profile           = strong_calloc(1, sizeof(rv_profile_t));
    profile->ext_enabled[RV_BASE]   = true;
    profile->ext_enabled[RV_32ONLY] = true;
    profile->base.backend           = &rv_backend;
    profile->base.reloc_names       = rv_reloc_names;
    return (void *)profile;
}

// Delete a profile for this backend.
void rv_delete_profile(backend_profile_t *profile0) {
    rv_profile_t *profile = (void *)profile0;
    free(profile->base.gpr_classes);
    free(profile);
}

// Prepare backend for codegen stage.
void rv_init_codegen(backend_profile_t *profile0) {
    rv_profile_t *profile = (void *)profile0;

    // Update other codegen-relevant settings.
    profile->ext_enabled[RV_32ONLY] = !profile->ext_enabled[RV_64];
    profile->base.gpr_bits          = profile->ext_enabled[RV_64] ? LILY_64_BITS : LILY_32_BITS;
    profile->base.arith_min_bits    = LILY_32_BITS;
    profile->base.arith_max_bits    = profile->base.gpr_bits;
    profile->base.ptr_bits          = profile->base.gpr_bits;
    profile->base.has_f32           = profile->ext_enabled[RV_EXT_F];
    profile->base.has_f64           = profile->ext_enabled[RV_EXT_D];
    profile->base.gpr_count         = profile->ext_enabled[RV_EXT_F] ? 64 : 32;
    profile->base.gpr_classes       = strong_calloc(profile->base.gpr_count, sizeof(regclass_t));

    profile->base.gpr_classes[0].val = 0;
    for (int i = 1; i < 32; i++) {
        profile->base.gpr_classes[i] = (regclass_t){.int32 = 1, .int64 = profile->ext_enabled[RV_64]};
    }
    if (profile->ext_enabled[RV_EXT_F]) {
        for (int i = 32; i < 64; i++) {
            profile->base.gpr_classes[i] = (regclass_t){.f32 = 1, .f64 = profile->ext_enabled[RV_EXT_D]};
        }
    }

    // ABI names of the registers.
    profile->base.gpr_names = rv_reg_names;
}



// The RISC-V backend.
backend_t const rv_backend = {
    .id             = "riscv",
    .create_profile = rv_create_profile,
    .delete_profile = rv_delete_profile,
    .init_codegen   = rv_init_codegen,
    .isel           = rv_isel,
};

// Table of RISC-V register names.
char const *const rv_reg_names[] = {
    // clang-format off
    "zero",
    "ra", "sp", "gp", "tp",
    "t0", "t1", "t2",
    "s0" /* also fp */, "s1",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
    "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
    "t3", "t4", "t5", "t6",
    // clang-format on
};
