
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "rv_backend.h"

#include "backend.h"
#include "ir.h"
#include "ir_types.h"
#include "lilycc_malloc.h"
#include "rv_abi.h"
#include "rv_instructions.h"
#include "rv_isel.h"
#include "rv_misc.h"
#include "unreachable.h"

#include <stdbool.h>

// Get the default backend.
backend_t const *backend_default() {
    return &rv_backend;
}



// Create a copy of the default profile for this type of backend.
backend_profile_t *rv_create_profile() {
    rv_profile_t *profile           = lilycc_calloc(1, sizeof(rv_profile_t));
    profile->ext_enabled[RV_BASE]   = true;
    profile->ext_enabled[RV_32ONLY] = true;
    profile->abi                    = RV_ABI_ILP32;
    profile->base.backend           = &rv_backend;
    profile->base.reloc_names       = rv_reloc_names;
    return (void *)profile;
}

// Delete a profile for this backend.
void rv_delete_profile(backend_profile_t *profile0) {
    rv_profile_t *profile = (void *)profile0;
    lilycc_free(profile->base.gpr_classes);
    lilycc_free(profile);
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
    profile->base.gpr_classes       = lilycc_calloc(profile->base.gpr_count, sizeof(regclass_t));

    // zero, ra, sp, gp and tp - all unallocatable in the standard ABIs.
    for (regno_t i = 0; i < 5; i++) {
        profile->base.gpr_classes[0].val = 0;
    }
    // Remaining integer registers are usable for general allocations.
    for (regno_t i = 5; i < 32; i++) {
        profile->base.gpr_classes[i] = (regclass_t){.int32 = 1, .int64 = profile->ext_enabled[RV_64]};
    }
    // Enable FPRs iff the ISA supports them.
    // ABI is separate; FPRs are tempregs if the ABI is not float.
    // TODO: Correct support for f32 ABI when f64 extension is enabled.
    if (profile->ext_enabled[RV_EXT_F]) {
        for (regno_t i = 32; i < 64; i++) {
            profile->base.gpr_classes[i] = (regclass_t){.f32 = 1, .f64 = profile->ext_enabled[RV_EXT_D]};
        }
    }

    // ABI names of the registers.
    profile->base.gpr_names = rv_reg_names;
}



// Emit load for register spilling.
void rv_spill_load(backend_profile_t *profile0, ir_insnloc_t loc, ir_var_t *dest, ir_frame_t *frame) {
    rv_profile_t       *profile = (void *)profile0;
    insn_proto_t const *op;
    switch (dest->orig_prim_type) {
        case IR_PRIM_s8: op = &rv_insn_lb; break;
        case IR_PRIM_bool:
        case IR_PRIM_u8: op = &rv_insn_lbu; break;
        case IR_PRIM_s16: op = &rv_insn_lh; break;
        case IR_PRIM_u16: op = &rv_insn_lhu; break;
        case IR_PRIM_s32: op = &rv_insn_lw; break;
        case IR_PRIM_u32:
            if (profile->ext_enabled[RV_64]) {
                UNREACHABLE(); // TODO.
            } else {
                op = &rv_insn_lw;
            }
            break;
        // case IR_PRIM_s64:
        // case IR_PRIM_u64:
        // case IR_PRIM_s128:
        // case IR_PRIM_u128:
        // case IR_PRIM_f32:
        // case IR_PRIM_f64:
        default: UNREACHABLE();
    }
    ir_add_mach_insn(
        loc,
        true,
        IR_RETVAL_VAR(dest),
        op,
        1,
        (ir_operand_t const[]){
            IR_OPERAND_MEM(IR_MEMREF(dest->orig_prim_type, IR_BADDR_FRAME(frame))),
        }
    );
}

// Emit store for register spilling.
void rv_spill_store(backend_profile_t *profile0, ir_insnloc_t loc, ir_var_t *src, ir_frame_t *frame) {
    rv_profile_t       *profile = (void *)profile0;
    insn_proto_t const *op;
    switch (src->orig_prim_type) {
        case IR_PRIM_s8: op = &rv_insn_lb; break;
        case IR_PRIM_bool:
        case IR_PRIM_u8: op = &rv_insn_lbu; break;
        case IR_PRIM_s16: op = &rv_insn_lh; break;
        case IR_PRIM_u16: op = &rv_insn_lhu; break;
        case IR_PRIM_s32: op = &rv_insn_lw; break;
        case IR_PRIM_u32:
            if (profile->ext_enabled[RV_64]) {
                UNREACHABLE(); // TODO.
            } else {
                op = &rv_insn_lw;
            }
            break;
        // case IR_PRIM_s64:
        // case IR_PRIM_u64:
        // case IR_PRIM_s128:
        // case IR_PRIM_u128:
        // case IR_PRIM_f32:
        // case IR_PRIM_f64:
        default: UNREACHABLE();
    }
    ir_add_mach_insn(
        loc,
        false,
        (ir_retval_t){},
        op,
        2,
        (ir_operand_t const[]){
            IR_OPERAND_VAR(src),
            IR_OPERAND_MEM(IR_MEMREF(src->orig_prim_type, IR_BADDR_FRAME(frame))),
        }
    );
}



// The RISC-V backend.
backend_t const rv_backend = {
    .id             = "riscv",
    .create_profile = rv_create_profile,
    .delete_profile = rv_delete_profile,
    .init_codegen   = rv_init_codegen,
    .isel           = rv_isel,
    .xabi_entry     = rv_xabi_entry,
    .xabi_call      = rv_xabi_call,
    .xabi_return    = rv_xabi_return,
    .ra_spill_load  = rv_spill_load,
    .ra_spill_store = rv_spill_store,
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
