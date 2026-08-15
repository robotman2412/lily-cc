
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "rv_backend.h"

#include "backend.h"
#include "ir.h"
#include "ir_types.h"
#include "lilycc_malloc.h"
#include "list.h"
#include "rv_abi.h"
#include "rv_instructions.h"
#include "rv_isel.h"
#include "rv_misc.h"
#include "unreachable.h"

#include <assert.h>
#include <stdbool.h>

// Get the default backend.
backend_t const *backend_default() {
    return &rv_backend;
}



// Create a copy of the default profile for this type of backend.
backend_profile_t *rv_create_profile() {
    rv_profile_t *profile          = lilycc_calloc(1, sizeof(rv_profile_t));
    profile->ext_enabled[RV_BASE]  = true;
    profile->ext_enabled[RV_64]    = true;
    profile->ext_enabled[RV_EXT_M] = true;
    profile->abi                   = RV_ABI_LP64;
    profile->base.backend          = &rv_backend;
    profile->base.reloc_names      = rv_reloc_names;
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
    profile->base.has_mul           = profile->ext_enabled[RV_EXT_M];
    profile->base.has_div           = profile->ext_enabled[RV_EXT_M];
    profile->base.has_rem           = profile->ext_enabled[RV_EXT_M];
    profile->base.has_f32           = profile->ext_enabled[RV_EXT_F];
    profile->base.has_f64           = profile->ext_enabled[RV_EXT_D];
    profile->base.gpr_count         = profile->ext_enabled[RV_EXT_F] ? 64 : 32;
    profile->base.gpr_classes       = lilycc_calloc(profile->base.gpr_count, sizeof(regclass_t));
    profile->base.frame_ptr_reg     = RV_REG_S(0);
    profile->base.stack_align       = 16;

    // zero, ra, sp, gp and tp - all unallocatable in the standard ABIs.
    for (int i = 0; i < 5; i++) {
        profile->base.gpr_classes[0].val = 0;
    }
    // Remaining integer registers are usable for general allocations.
    for (int i = 5; i < (profile->is_rve ? 16 : 32); i++) {
        profile->base.gpr_classes[i] = (regclass_t){.int32 = 1, .int64 = profile->ext_enabled[RV_64]};
    }
    // For large offsets (especially from the frame pointer), arbitrarily reserve `t0`.
    // Inline assembly may still use it as it wishes; `t0` will only be used for stores and large stack offsets.
    profile->base.gpr_classes[RV_REG_T(0)].val = 0;
    // Callee-save registers s0 and s1.
    profile->base.gpr_classes[8].callee_save   = 1;
    profile->base.gpr_classes[9].callee_save   = 1;
    if (!profile->is_rve) {
        // Callee-save registers s2 through s11.
        for (int i = 18; i < 28; i++) {
            profile->base.gpr_classes[i].callee_save = 1;
        }
    }
    // Enable FPRs iff the ISA supports them.
    // ABI is separate; FPRs are tempregs if the ABI is not float.
    // TODO: Correct support for f32 ABI when f64 extension is enabled.
    if (profile->ext_enabled[RV_EXT_F]) {
        for (int i = 32; i < 64; i++) {
            profile->base.gpr_classes[i] = (regclass_t){.f32 = 1, .f64 = profile->ext_enabled[RV_EXT_D]};
        }
        if (profile->abi == RV_ABI_ILP32E || profile->abi == RV_ABI_ILP32 || profile->abi == RV_ABI_LP64) {
            // Non-float ABI; all float regs are caller-save.
        } else {
            // Float callee-saved registers fs0 and fs1.
            profile->base.gpr_classes[32 + 8].callee_save = 1;
            profile->base.gpr_classes[32 + 9].callee_save = 1;
            // Float callee-saved registers fs2 through fs11.
            for (int i = 18; i < 28; i++) {
                profile->base.gpr_classes[32 + i].callee_save = 1;
            }
        }
    }

    // ABI names of the registers.
    profile->base.gpr_names = rv_reg_names;
}

// Perform target-specific post-regalloc passes.
void rv_postreg_pass(backend_profile_t *profile0, ir_func_t *func) {
    rv_profile_t *profile   = (void *)profile0;
    uint64_t      word_size = profile->ext_enabled[RV_64] ? 8 : 4;

    uint64_t size = 0;
    if (func->use_frame_ptr) {
        size = 2 * word_size;
    } else if (!func->is_leaf) {
        size = word_size;
    }
    if (size) {
        func->link_frame = ir_frame_create(func, size, word_size, NULL);
    }
}

static void rv_expand_la(ir_insn_t *insn) {
    if (insn->type != IR_INSN_MACHINE || insn->prototype != &rv_insn_la) {
        return;
    }
    assert(insn->operands[0].type == IR_OPERAND_TYPE_MEM);
    assert(insn->returns[0].type == IR_RETVAL_TYPE_REG);
    ir_func_t  *func = insn->code->func;
    ir_memref_t mem  = insn->operands[0].mem;

    int64_t offset;
    int64_t reg;
    if (mem.base_type == IR_MEMBASE_ABS) {
        insn->prototype = &rv_insn_li;
        return;
    } else if (mem.base_type == IR_MEMBASE_REG) {
        reg    = mem.base_regno;
        offset = mem.offset;
    } else if (mem.base_type == IR_MEMBASE_FRAME) {
        offset = mem.offset - (int64_t)mem.base_frame->offset;
        reg    = RV_REG_S(0);
        if (!func->use_frame_ptr) {
            offset += (int64_t)func->stack_footprint;
            reg     = RV_REG_SP;
        }
    } else {
        // Symbol-relative; nothing to fix.
        assert(mem.base_type != IR_MEMBASE_VAR);
        return;
    }

    // If we get here, it's relative to a register (or the stack).

    if (offset >= -2048 && offset <= 2047) {
        // Offset can be generated with just an `addi`.
        ir_add_mach_insn(
            IR_BEFORE_INSN(insn),
            true,
            insn->returns[0],
            &rv_insn_addi,
            2,
            (ir_operand_t const[]){
                IR_OPERAND_REG(reg),
                IR_OPERAND_CONST(IR_CONST_S16(offset)),
            }
        );
    } else {
        // Offset is too wide; use `li + add`.
        ir_add_mach_insn(
            IR_BEFORE_INSN(insn),
            true,
            insn->returns[0],
            &rv_insn_li,
            1,
            (ir_operand_t const[]){
                IR_OPERAND_CONST(IR_CONST_S64(offset)),
            }
        );
        ir_add_mach_insn(
            IR_BEFORE_INSN(insn),
            true,
            insn->returns[0],
            &rv_insn_add,
            3,
            (ir_operand_t const[]){
                IR_OPERAND_REG(insn->returns[0].dest_regno),
                IR_OPERAND_REG(reg),
            }
        );
    }

    ir_insn_delete(insn);
}

static void rv_expand_mem(ir_insn_t *insn) {
    if (insn->type != IR_INSN_MACHINE || insn->prototype == &rv_insn_la) {
        return;
    }
    ir_func_t           *func = insn->code->func;
    rv_encoding_t const *enc  = insn->prototype->cookie;
    ir_memref_t         *mem;
    if (enc->opcode == RV_OP_STORE || enc->opcode == RV_OP_STORE_FP) {
        mem = &insn->operands[1].mem;
    } else if (enc->opcode == RV_OP_LOAD || enc->opcode == RV_OP_LOAD_FP) {
        mem = &insn->operands[0].mem;
    } else {
        return;
    }
    if (mem->base_type != IR_MEMBASE_FRAME) {
        return;
    }

    int64_t offset = mem->offset - (int64_t)mem->base_frame->offset;
    regno_t reg    = RV_REG_S(0);
    if (!func->use_frame_ptr) {
        offset += (int64_t)func->stack_footprint;
        reg     = RV_REG_SP;
    }

    if (offset < -2048 || offset > 2047) {
        // Won't fit in the imm12 field.
        int64_t offset_hi = (offset >> 12) + (offset < 0);
        ir_add_mach_insn(
            IR_BEFORE_INSN(insn),
            true,
            IR_RETVAL_REG(RV_REG_T(0)),
            &rv_insn_lui,
            1,
            (ir_operand_t const[]){
                IR_OPERAND_CONST(IR_CONST_S64(offset_hi)),
            }
        );
        offset = offset << 52 >> 52;
        reg    = RV_REG_T(0);
    }

    *mem = IR_MEMREF(IR_N_PRIM, IR_BADDR_REG(reg), .offset = offset);
}

static void rv_write_prologue(rv_profile_t *profile, ir_func_t *func, ir_insnloc_t loc) {
    int64_t             word_size = profile->ext_enabled[RV_64] ? 8 : 4;
    insn_proto_t const *store     = profile->ext_enabled[RV_64] ? &rv_insn_sd : &rv_insn_sw;

    // Decrease `sp`.
    if (func->stack_footprint == 0) {
        // Nothing to do.
    } else if (func->stack_footprint <= 2047) {
        // addi sp, sp, -footprint
        loc = IR_AFTER_INSN(ir_add_mach_insn(
            loc,
            true,
            IR_RETVAL_REG(RV_REG_SP),
            &rv_insn_addi,
            2,
            (ir_operand_t const[]){
                IR_OPERAND_REG(RV_REG_SP),
                IR_OPERAND_CONST(IR_CONST_S64(-(int64_t)func->stack_footprint)),
            }
        ));
    } else {
        // li t0, footprint
        loc = IR_AFTER_INSN(ir_add_mach_insn(
            loc,
            true,
            IR_RETVAL_REG(RV_REG_T(0)),
            &rv_insn_li,
            1,
            (ir_operand_t const[]){
                IR_OPERAND_CONST(IR_CONST_U64(func->stack_footprint)),
            }
        ));
        // sub sp, sp, t0
        loc = IR_AFTER_INSN(ir_add_mach_insn(
            loc,
            true,
            IR_RETVAL_REG(RV_REG_SP),
            &rv_insn_sub,
            2,
            (ir_operand_t const[]){
                IR_OPERAND_REG(RV_REG_SP),
                IR_OPERAND_REG(RV_REG_T(0)),
            }
        ));
    }

    int64_t offset = (int64_t)func->stack_footprint;
    if (!func->is_leaf || func->use_frame_ptr) {
        offset -= word_size;
        loc     = IR_AFTER_INSN(ir_add_mach_insn(
            loc,
            false,
            (ir_retval_t){},
            store,
            2,
            (ir_operand_t const[]){
                IR_OPERAND_REG(RV_REG_RA),
                IR_OPERAND_MEM(IR_MEMREF(IR_N_PRIM, IR_BADDR_REG(RV_REG_SP), .offset = offset)),
            }
        ));
    }
    if (func->use_frame_ptr) {
        offset -= word_size;
        loc     = IR_AFTER_INSN(ir_add_mach_insn(
            loc,
            false,
            (ir_retval_t){},
            store,
            2,
            (ir_operand_t const[]){
                IR_OPERAND_REG(RV_REG_S(0)),
                IR_OPERAND_MEM(IR_MEMREF(IR_N_PRIM, IR_BADDR_REG(RV_REG_SP), .offset = offset)),
            }
        ));
    }

    // Update `fp`.
    if (func->use_frame_ptr) {
        if (func->stack_footprint <= 2047) {
            // addi fp, sp, footprint
            loc = IR_AFTER_INSN(ir_add_mach_insn(
                loc,
                true,
                IR_RETVAL_REG(RV_REG_S(0)),
                &rv_insn_addi,
                2,
                (ir_operand_t const[]){
                    IR_OPERAND_REG(RV_REG_SP),
                    IR_OPERAND_CONST(IR_CONST_S64((int64_t)func->stack_footprint)),
                }
            ));

        } else {
            // add fp, sp, t0
            loc = IR_AFTER_INSN(ir_add_mach_insn(
                loc,
                true,
                IR_RETVAL_REG(RV_REG_S(0)),
                &rv_insn_add,
                2,
                (ir_operand_t const[]){
                    IR_OPERAND_REG(RV_REG_SP),
                    IR_OPERAND_REG(RV_REG_T(0)),
                }
            ));
        }
    }
}

static void rv_write_epilogue(rv_profile_t *profile, ir_func_t *func, ir_insnloc_t loc) {
    int64_t             word_size = profile->ext_enabled[RV_64] ? 8 : 4;
    insn_proto_t const *load      = profile->ext_enabled[RV_64] ? &rv_insn_ld : &rv_insn_lw;

    int64_t footprint;
    if (func->uses_alloca) {
        assert(func->use_frame_ptr);
        // addi sp, fp, -2*word_size
        footprint = 2 * word_size;
        loc       = IR_AFTER_INSN(ir_add_mach_insn(
            loc,
            true,
            IR_RETVAL_REG(RV_REG_SP),
            &rv_insn_addi,
            1,
            (ir_operand_t const[]){
                IR_OPERAND_REG(RV_REG_S(0)),
                IR_OPERAND_CONST(IR_CONST_S16(2 * word_size)),
            }
        ));
    } else {
        footprint = (int64_t)func->stack_footprint;
    }

    int64_t offset = footprint;
    if (!func->is_leaf || func->use_frame_ptr) {
        offset -= word_size;
        loc     = IR_AFTER_INSN(ir_add_mach_insn(
            loc,
            true,
            IR_RETVAL_REG(RV_REG_RA),
            load,
            1,
            (ir_operand_t const[]){
                IR_OPERAND_MEM(IR_MEMREF(IR_N_PRIM, IR_BADDR_REG(RV_REG_SP), .offset = offset)),
            }
        ));
    }
    if (func->use_frame_ptr) {
        offset -= word_size;
        loc     = IR_AFTER_INSN(ir_add_mach_insn(
            loc,
            true,
            IR_RETVAL_REG(RV_REG_S(0)),
            load,
            1,
            (ir_operand_t const[]){
                IR_OPERAND_MEM(IR_MEMREF(IR_N_PRIM, IR_BADDR_REG(RV_REG_SP), .offset = offset)),
            }
        ));
    }

    if (footprint == 0) {
        // Nothing to do.
    } else if (footprint <= 2047) {
        // addi sp, sp, footprint
        loc = IR_AFTER_INSN(ir_add_mach_insn(
            loc,
            true,
            IR_RETVAL_REG(RV_REG_SP),
            &rv_insn_addi,
            2,
            (ir_operand_t const[]){
                IR_OPERAND_REG(RV_REG_SP),
                IR_OPERAND_CONST(IR_CONST_S64(footprint)),
            }
        ));
    } else {
        // li t0, footprint
        loc = IR_AFTER_INSN(ir_add_mach_insn(
            loc,
            true,
            IR_RETVAL_REG(RV_REG_T(0)),
            &rv_insn_li,
            1,
            (ir_operand_t const[]){
                IR_OPERAND_CONST(IR_CONST_U64(func->stack_footprint)),
            }
        ));
        // add sp, sp, t0
        loc = IR_AFTER_INSN(ir_add_mach_insn(
            loc,
            true,
            IR_RETVAL_REG(RV_REG_SP),
            &rv_insn_add,
            2,
            (ir_operand_t const[]){
                IR_OPERAND_REG(RV_REG_SP),
                IR_OPERAND_REG(RV_REG_T(0)),
            }
        ));
    }
}

// Perform target-specific post-processing passes.
void rv_postproc_pass(backend_profile_t *profile, ir_func_t *func) {
    (void)profile;
    rv_write_prologue((rv_profile_t *)profile, func, IR_PREPEND(func->entry));
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        ir_insn_t *cur = (ir_insn_t *)code->insns.head;
        while (cur) {
            ir_insn_t *next = (ir_insn_t *)cur->node.next;
            rv_expand_la(cur);
            rv_expand_mem(cur);
            if (cur->type == IR_INSN_EPILOGUE) {
                rv_write_epilogue((rv_profile_t *)profile, func, IR_AFTER_INSN(cur));
            }
            cur = next;
        }
    }
}



// Emit load for register spilling.
void rv_spill_load(backend_profile_t *profile0, ir_insnloc_t loc, ir_retval_t dest, ir_frame_t *frame, int64_t offset) {
    rv_profile_t       *profile = (void *)profile0;
    insn_proto_t const *op;
    ir_prim_t           prim;
    if (dest.type == IR_RETVAL_TYPE_VAR) {
        prim = dest.dest_var->orig_prim_type;
    } else {
        assert(dest.type == IR_RETVAL_TYPE_REG);
        prim = profile->ext_enabled[RV_64] ? IR_PRIM_s64 : IR_PRIM_s32;
    }
    switch (prim) {
        case IR_PRIM_s8: op = &rv_insn_lb; break;
        case IR_PRIM_bool:
        case IR_PRIM_u8: op = &rv_insn_lbu; break;
        case IR_PRIM_s16: op = &rv_insn_lh; break;
        case IR_PRIM_u16: op = &rv_insn_lhu; break;
        case IR_PRIM_s32:
        case IR_PRIM_u32: op = &rv_insn_lw; break;
        case IR_PRIM_s64:
        case IR_PRIM_u64: op = &rv_insn_ld; break;
        // case IR_PRIM_s128:
        // case IR_PRIM_u128:
        // case IR_PRIM_f32:
        // case IR_PRIM_f64:
        default: UNREACHABLE();
    }
    ir_add_mach_insn(
        loc,
        true,
        dest,
        op,
        1,
        (ir_operand_t const[]){
            IR_OPERAND_MEM(IR_MEMREF(prim, IR_BADDR_FRAME(frame), .offset = offset)),
        }
    );
}

// Emit store for register spilling.
void rv_spill_store(
    backend_profile_t *profile0, ir_insnloc_t loc, ir_operand_t src, ir_frame_t *frame, int64_t offset
) {
    rv_profile_t       *profile = (void *)profile0;
    insn_proto_t const *op;
    ir_prim_t           prim;
    if (src.type == IR_OPERAND_TYPE_VAR) {
        prim = src.var->orig_prim_type;
    } else {
        assert(src.type == IR_OPERAND_TYPE_REG);
        prim = profile->ext_enabled[RV_64] ? IR_PRIM_s64 : IR_PRIM_s32;
    }
    switch (prim) {
        case IR_PRIM_s8:
        case IR_PRIM_bool:
        case IR_PRIM_u8: op = &rv_insn_sb; break;
        case IR_PRIM_s16:
        case IR_PRIM_u16: op = &rv_insn_sh; break;
        case IR_PRIM_s32:
        case IR_PRIM_u32: op = &rv_insn_sw; break;
        case IR_PRIM_s64:
        case IR_PRIM_u64: op = &rv_insn_sd; break;
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
            src,
            IR_OPERAND_MEM(IR_MEMREF(prim, IR_BADDR_FRAME(frame), .offset = offset)),
        }
    );
}

// Print directives before the function label.
void rv_asm_print_prefunc(backend_profile_t *profile, ir_func_t const *func, FILE *to) {
    (void)profile;
    (void)func;
    fputs("    .p2align 1\n", to);
}



// The RISC-V backend.
backend_t const rv_backend = {
    .id                = "riscv",
    .create_profile    = rv_create_profile,
    .delete_profile    = rv_delete_profile,
    .init_codegen      = rv_init_codegen,
    .postreg_pass      = rv_postreg_pass,
    .postproc_pass     = rv_postproc_pass,
    .isel              = rv_isel,
    .xabi_entry        = rv_xabi_entry,
    .xabi_call         = rv_xabi_call,
    .xabi_return       = rv_xabi_return,
    .ra_spill_load     = rv_spill_load,
    .ra_spill_store    = rv_spill_store,
    .asm_print_prefunc = rv_asm_print_prefunc,
    .asm_print_insn    = rv_asm_print_insn,
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
    
    "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7",
    "fs0", "fs1",
    "fa0", "fa1", "fa2", "fa3", "fa4", "fa5", "fa6", "fa7",
    "fs2", "fs3", "fs4", "fs5", "fs6", "fs7", "fs8", "fs9", "fs10", "fs11",
    "ft8", "ft9", "ft10", "ft11",
    // clang-format on
};
