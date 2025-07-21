
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "rv_backend.h"

#include "arrays.h"
#include "backend.h"
#include "cand_tree.h"
#include "insn_proto.h"
#include "rv_instructions.h"
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
    return (void *)profile;
}

// Delete a profile for this backend.
void rv_delete_profile(backend_profile_t *_profile) {
    rv_profile_t *profile = (void *)_profile;
    cand_tree_delete(profile->cand_tree);
    free(profile->base.regclasses);
    free(profile);
}

// Prepare backend for codegen stage.
void rv_init_codegen(backend_profile_t *_profile) {
    rv_profile_t        *profile    = (void *)_profile;
    insn_proto_t const **protos     = NULL;
    size_t               protos_len = 0;
    size_t               protos_cap = 0;

    // Collect all instruction prototypes enabled in the profile.
    for (size_t i = 0; i < rv_insns_len; i++) {
        rv_encoding_t const *cookie = rv_insns[i]->cookie;
        if (rv_insns[i]->tree && profile->ext_enabled[cookie->ext]) {
            array_lencap_insert_strong(&protos, sizeof(void *), &protos_len, &protos_cap, &rv_insns[i], protos_len);
        }
    }

    // Create candidate tree from instruction set.
    profile->cand_tree = cand_tree_generate(protos_len, protos);
    free(protos);

    // Update other codegen-relevant settings.
    profile->ext_enabled[RV_32ONLY] = !profile->ext_enabled[RV_64];
    profile->base.gpr_bits          = profile->ext_enabled[RV_64] ? LILY_64_BITS : LILY_32_BITS;
    profile->base.arith_min_bits    = LILY_32_BITS;
    profile->base.arith_max_bits    = profile->base.gpr_bits;
    profile->base.has_f32           = profile->ext_enabled[RV_EXT_F];
    profile->base.has_f64           = profile->ext_enabled[RV_EXT_D];
    profile->base.gpr_count         = profile->ext_enabled[RV_EXT_F] ? 63 : 31;
    profile->base.regclasses        = strong_calloc(profile->base.gpr_count, sizeof(regclass_t));

    for (int i = 0; i < 31; i++) {
        profile->base.regclasses[i] = (regclass_t){.int32 = 1, .int64 = profile->ext_enabled[RV_64]};
    }
    if (profile->ext_enabled[RV_EXT_F]) {
        for (int i = 31; i < 63; i++) {
            profile->base.regclasses[i] = (regclass_t){.f32 = 1, .f64 = profile->ext_enabled[RV_EXT_D]};
        }
    }
}

// Perform instruction selection for expressions, memory access and branches.
insn_proto_t const *rv_isel(backend_profile_t *_profile, ir_insn_t const *ir_insn, ir_operand_t *operands_out) {
    rv_profile_t *profile = (void *)_profile;
    return cand_tree_isel(profile->cand_tree, ir_insn, operands_out);
}



// The RISC-V backend.
backend_t const rv_backend = {
    .id             = "riscv",
    .create_profile = rv_create_profile,
    .delete_profile = rv_delete_profile,
    .init_codegen   = rv_init_codegen,
    .isel           = rv_isel,
};
