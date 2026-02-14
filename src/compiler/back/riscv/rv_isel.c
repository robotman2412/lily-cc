
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "rv_isel.h"

#include "ir.h"
#include "ir_interpreter.h"
#include "ir_types.h"
#include "list.h"
#include "rv_backend.h"
#include "rv_instructions.h"
#include "rv_misc.h"
#include "unreachable.h"

#include <assert.h>
#include <stdio.h>



// Primitive type corresponding to a certain register class.
static inline __attribute__((pure)) ir_prim_t rv_regclass_prim(rv_profile_t const *profile, size_t regno) {
    if (regno < 32) {
        return profile->ext_enabled[RV_64] ? IR_PRIM_u64 : IR_PRIM_u32;
    } else if (regno < 64 && profile->ext_enabled[RV_EXT_D]) {
        return IR_PRIM_f64;
    } else if (regno < 64 && profile->ext_enabled[RV_EXT_D]) {
        return IR_PRIM_f32;
    } else {
        UNREACHABLE();
    }
}

// `ir_operand_prim` extended to also understand the registers' types.
static inline __attribute__((pure)) ir_prim_t rv_operand_prim(rv_profile_t const *profile, ir_operand_t operand) {
    if (operand.type == IR_OPERAND_TYPE_REG) {
        return rv_regclass_prim(profile, operand.regno);
    } else {
        return ir_operand_prim(operand);
    }
}

// `ir_retval_t` equivalent to `rv_operand_prim`.
static inline __attribute__((pure)) ir_prim_t rv_retval_prim(rv_profile_t const *profile, ir_retval_t retval) {
    switch (retval.type) {
        case IR_RETVAL_TYPE_VAR: return retval.dest_var->prim_type;
        case IR_RETVAL_TYPE_REG: return rv_regclass_prim(profile, retval.dest_regno);
        case IR_RETVAL_TYPE_STRUCT: UNREACHABLE();
    }
    UNREACHABLE();
}

// Emit a register-register copy.
static ir_insn_t *rv_emit_rr_copy(rv_profile_t const *profile, ir_insnloc_t loc, ir_retval_t dest, ir_operand_t src) {
    if (ir_prim_is_float(rv_retval_prim(profile, dest)) || ir_prim_is_float(rv_operand_prim(profile, src))) {
        fprintf(stderr, "TODO: rv_emit_rr_copy with float types\n");
        abort();
    }

    return ir_add_mach_insn(
        loc,
        true,
        dest,
        &rv_insn_addi,
        2,
        (ir_operand_t const[]){
            src,
            IR_OPERAND_CONST(IR_CONST_S16(0)),
        }
    );
}

// Emit an integer casting operation.
static ir_insn_t *rv_emit_int_cast(
    rv_profile_t const *profile, ir_insnloc_t loc, ir_retval_t dest, ir_prim_t dest_prim, ir_operand_t src
) {
    if (src.type == IR_OPERAND_TYPE_REG || dest_prim < ir_operand_prim(src)) {
        return rv_emit_rr_copy(profile, loc, dest, src);
    }
    ir_prim_t src_prim = ir_operand_prim(src);
    uint8_t   ptr_bits = profile->ext_enabled[RV_64] ? 64 : 32;

    if (dest_prim == IR_PRIM_bool) {
        // slt dest, x0, src
        return ir_add_mach_insn(loc, true, dest, &rv_insn_sltu, 2, (ir_operand_t const[]){IR_OPERAND_REG(0), src});
    } else if (ir_prim_is_signed(src_prim)
               || (ir_prim_is_signed(dest_prim) && ir_prim_as_signed(src_prim) == dest_prim)) {
        // Sign-extend.
        uint8_t   shamt = ptr_bits - ir_prim_sizes[dest_prim] * 8;
        ir_var_t *tmp   = ir_var_create(ir_insnloc_code(loc)->func, dest_prim, NULL);
        // slli tmp, src, shamt
        loc             = IR_AFTER_INSN(ir_add_mach_insn(
            loc,
            true,
            IR_RETVAL_VAR(tmp),
            &rv_insn_slli,
            2,
            (ir_operand_t const[]){src, IR_OPERAND_CONST(IR_CONST_S16(shamt))}
        ));
        // srai dest, tmp, shamt
        return ir_add_mach_insn(
            loc,
            true,
            dest,
            &rv_insn_srai,
            2,
            (ir_operand_t const[]){src, IR_OPERAND_CONST(IR_CONST_S16(shamt))}
        );
    } else if (dest_prim == IR_PRIM_u8) {
        // Zero-extend (one byte).
        // andi dest, src, 255
        return ir_add_mach_insn(
            loc,
            true,
            dest,
            &rv_insn_srai,
            2,
            (ir_operand_t const[]){src, IR_OPERAND_CONST(IR_CONST_S16(255))}
        );
    } else {
        // Zero-extend.
        uint8_t   shamt = ptr_bits - ir_prim_sizes[dest_prim] * 8;
        ir_var_t *tmp   = ir_var_create(ir_insnloc_code(loc)->func, dest_prim, NULL);
        // slli tmp, src, shamt
        loc             = IR_AFTER_INSN(ir_add_mach_insn(
            loc,
            true,
            IR_RETVAL_VAR(tmp),
            &rv_insn_slli,
            2,
            (ir_operand_t const[]){src, IR_OPERAND_CONST(IR_CONST_S16(shamt))}
        ));
        // srli dest, tmp, shamt
        return ir_add_mach_insn(
            loc,
            true,
            dest,
            &rv_insn_srli,
            2,
            (ir_operand_t const[]){src, IR_OPERAND_CONST(IR_CONST_S16(shamt))}
        );
    }
}


// Register-register bitcast.
static ir_insn_t *rv_isel_rr_bitcast(rv_profile_t const *profile, ir_insn_t *insn) {
    (void)profile;
    ir_insn_t *new_node = rv_emit_rr_copy(profile, IR_BEFORE_INSN(insn), insn->returns[0], insn->operands[0]);
    ir_insn_delete(insn);
    return new_node;
}

// Register-register mov.
static ir_insn_t *rv_isel_rr_mov(rv_profile_t const *profile, ir_insn_t *insn) {
    ir_operand_t operand = insn->operands[0];
    ir_insn_t   *new_node;
    if (ir_prim_is_float(rv_operand_prim(profile, operand))) {
        fprintf(stderr, "TODO: rv_isel_rr_mov with float types\n");
        return NULL;
    } else {
        new_node = rv_emit_int_cast(
            profile,
            IR_BEFORE_INSN(insn),
            insn->returns[0],
            insn->returns[0].dest_var->prim_type,
            operand
        );
    }

    ir_insn_delete(insn);
    return new_node;
}

// Constant mov.
static ir_insn_t *rv_isel_const_mov(rv_profile_t const *profile, ir_insn_t *insn) {
    (void)profile;
    ir_func_t *const func   = insn->code->func;
    ir_const_t const iconst = ir_trim_const(insn->operands[0].iconst);

    if (iconst.prim_type == IR_PRIM_f32 || iconst.prim_type == IR_PRIM_f64) {
        fprintf(stderr, "TODO: Emit constant for FP\n");
        return NULL;
    } else if (iconst.prim_type >= IR_PRIM_s64 && iconst.prim_type <= IR_PRIM_u128) {
        fprintf(stderr, "TODO: Emit constant for 64-bit or larger\n");
        return NULL;
    }

    // Determine the operands to the `lui` and `addi` instructions.
    int32_t lui  = (int32_t)(iconst.constl >> 12);
    int16_t addi = (int16_t)((iconst.constl << 4) >> 4);
    if (addi < 0) {
        lui++;
    }
    lui &= 0x000fffff;

    // Replace instruction.
    func->enforce_ssa = false;
    ir_insn_t *new_node;
    if (lui && addi) {
        // Use `lui` followed by `addi`.
        ir_var_t *tmp = ir_var_create(func, IR_PRIM_s32, NULL);
        ir_add_mach_insn(
            IR_BEFORE_INSN(insn),
            true,
            IR_RETVAL_VAR(tmp),
            &rv_insn_lui,
            1,
            (ir_operand_t const[]){
                IR_OPERAND_CONST(IR_CONST_S32(lui)),
            }
        );
        new_node = ir_add_mach_insn(
            IR_BEFORE_INSN(insn),
            true,
            insn->returns[0],
            &rv_insn_addi,
            2,
            (ir_operand_t const[]){
                IR_OPERAND_VAR(tmp),
                IR_OPERAND_CONST(IR_CONST_S16(addi)),
            }
        );

    } else if (lui) {
        // Use just `lui`.
        new_node = ir_add_mach_insn(
            IR_BEFORE_INSN(insn),
            true,
            insn->returns[0],
            &rv_insn_lui,
            1,
            (ir_operand_t const[]){
                IR_OPERAND_CONST(IR_CONST_S32(lui)),
            }
        );

    } else {
        // Use just `addi`.
        new_node = ir_add_mach_insn(
            IR_BEFORE_INSN(insn),
            true,
            insn->returns[0],
            &rv_insn_addi,
            2,
            (ir_operand_t const[]){
                IR_OPERAND_REG(0),
                IR_OPERAND_CONST(IR_CONST_S16(addi)),
            }
        );
    }
    ir_insn_delete(insn);
    func->enforce_ssa = true;

    return new_node;
}

// Constant bitcast.
static ir_insn_t *rv_isel_const_bitcast(rv_profile_t const *profile, ir_insn_t *insn) {
    insn->operands[0].iconst = ir_cast(rv_retval_prim(profile, insn->returns[0]), insn->operands[0].iconst);
    return rv_isel_const_mov(profile, insn);
}


// Jump instruction.
static inline ir_insn_t *rv_isel_jump(rv_profile_t const *profile, ir_insn_t *insn) {
    (void)profile;
    insn->type      = IR_INSN_MACHINE;
    insn->prototype = &rv_insn_jal;
    return insn;
}

// Compare-branch instruction patterns.
static inline ir_insn_t *rv_isel_cmp_branch(rv_profile_t const *profile, ir_insn_t *ir_insn) {
    (void)profile;
    // Check that the branch is given a variable...
    if (ir_insn->operands[1].type != IR_OPERAND_TYPE_VAR) {
        return NULL;
    }
    // ...which is initialized...
    set_ent_t const *pred = set_next(&ir_insn->operands[1].var->assigned_at, NULL);
    if (!pred) {
        return NULL;
    }
    ir_insn_t *pred_insn = pred->value;

    if (pred_insn->type == IR_INSN_EXPR1) {
        // ...by comparison expr1...
        insn_proto_t const *proto;
        switch (pred_insn->op1) {
            case IR_OP1_mov:  // Mov here has the same effect as snez.
            case IR_OP1_snez: // bne rs1, x0, dest
                proto = &rv_insn_bne;
                break;
            case IR_OP1_seqz: // beq rs1, x0, dest
                proto = &rv_insn_beq;
                break;
            default: return NULL;
        }
        ir_insn_t *new_node = ir_add_mach_insn(
            IR_BEFORE_INSN(ir_insn),
            false,
            (ir_retval_t){},
            proto,
            3,
            (ir_operand_t const[]){
                pred_insn->operands[0],
                IR_OPERAND_REG(0),
                ir_insn->operands[0],
            }
        );

        if (pred_insn->returns[0].dest_var->used_at.len == 1) {
            ir_insn_delete(pred_insn);
        }
        ir_insn_delete(ir_insn);
        return new_node;

    } else if (pred_insn->type == IR_INSN_EXPR2) {
        // ...or comparison expr2.
        bool                is_signed = ir_prim_is_signed(ir_operand_prim(pred_insn->operands[0]));
        insn_proto_t const *proto;
        bool                swap;
        switch (pred_insn->op2) {
            case IR_OP2_sgt: // blt rs2, rs1, dest
                proto = is_signed ? &rv_insn_blt : &rv_insn_bltu;
                swap  = true;
                break;
            case IR_OP2_sle: // blt rs1, rs2, dest
                proto = is_signed ? &rv_insn_blt : &rv_insn_bltu;
                swap  = false;
                break;
            case IR_OP2_slt: // bge rs2, rs1, dest
                proto = is_signed ? &rv_insn_bge : &rv_insn_bgeu;
                swap  = true;
                break;
            case IR_OP2_sge: // bge rs1, rs2, dest
                proto = is_signed ? &rv_insn_bge : &rv_insn_bgeu;
                swap  = false;
                break;
            case IR_OP2_seq: // beq rs1, rs2, dest
                proto = &rv_insn_beq;
                swap  = false;
                break;
            case IR_OP2_sne: // bne rs1, rs2, dest
                proto = &rv_insn_bne;
                swap  = false;
                break;
            default: return NULL;
        }
        ir_insn_t *new_node = ir_add_mach_insn(
            IR_BEFORE_INSN(ir_insn),
            false,
            (ir_retval_t){},
            proto,
            3,
            (ir_operand_t const[]){pred_insn->operands[swap], pred_insn->operands[!swap], ir_insn->operands[0]}
        );

        if (pred_insn->returns[0].dest_var->used_at.len == 1) {
            ir_insn_delete(pred_insn);
        }
        ir_insn_delete(ir_insn);
        return new_node;

    } else {
        return NULL;
    }
}

// Branch instruction patterns.
static inline ir_insn_t *rv_isel_branch(rv_profile_t const *profile, ir_insn_t *ir_insn) {
    ir_insn_t *new_node = rv_isel_cmp_branch(profile, ir_insn);
    if (new_node) {
        return new_node;
    }

    // bne rs1, x0, dest
    new_node = ir_add_mach_insn(
        IR_BEFORE_INSN(ir_insn),
        false,
        (ir_retval_t){},
        &rv_insn_bne,
        3,
        (ir_operand_t const[]){ir_insn->operands[1], IR_OPERAND_REG(0), ir_insn->operands[0]}
    );

    ir_insn_delete(ir_insn);
    assert(new_node->node.next != &ir_insn->node);
    assert(!dlist_contains(&new_node->code->insns, &ir_insn->node));

    return new_node;
}

// Binary expressions.
static inline ir_insn_t *rv_isel_expr2_ri(rv_profile_t const *profile, ir_insn_t *ir_insn) {
    (void)profile;

    if (ir_insn->operands[1].type != IR_OPERAND_TYPE_CONST) {
        return NULL;
    }
    ir_const_t iconst = ir_insn->operands[1].iconst;
    if (iconst.prim_type == IR_PRIM_f32 || iconst.prim_type == IR_PRIM_f64 || ir_count_bits(iconst, true, false) > 12) {
        return NULL;
    }

    insn_proto_t const *proto;
    switch (ir_insn->op2) {
        // case IR_OP2_sgt:
        // case IR_OP2_sle:
        // case IR_OP2_slt:
        // case IR_OP2_sge:
        // case IR_OP2_seq:
        // case IR_OP2_sne:
        case IR_OP2_add: proto = &rv_insn_addi; break;
        case IR_OP2_sub:
            proto  = &rv_insn_addi;
            iconst = ir_calc1(IR_OP1_neg, iconst);
            break;
        case IR_OP2_shl: proto = &rv_insn_slli; break;
        case IR_OP2_shr:
            proto = ir_prim_is_signed(ir_operand_prim(ir_insn->operands[0])) ? &rv_insn_srai : &rv_insn_srli;
            break;
        case IR_OP2_band: proto = &rv_insn_andi; break;
        case IR_OP2_bor: proto = &rv_insn_ori; break;
        case IR_OP2_bxor: proto = &rv_insn_xori; break;
        default: return NULL;
    }

    ir_insn->type      = IR_INSN_MACHINE;
    ir_insn->prototype = proto;

    return ir_insn;
}

// Binary expressions.
static inline ir_insn_t *rv_isel_expr2_rr(rv_profile_t const *profile, ir_insn_t *ir_insn) {
    (void)profile;
    if (ir_insn->returns[0].dest_var->prim_type == IR_PRIM_f32
        || ir_insn->returns[0].dest_var->prim_type == IR_PRIM_f64) {
        return NULL;
    }

    if (ir_insn->op2 == IR_OP2_sgt || ir_insn->op2 == IR_OP2_sge) {
        // Swap operands as RISC-V only has slt and an emulated sle.
        ir_operand_t tmp     = ir_insn->operands[0];
        ir_insn->operands[0] = ir_insn->operands[1];
        ir_insn->operands[1] = tmp;
    }

    if (ir_insn->op2 == IR_OP2_sle || ir_insn->op2 == IR_OP2_sge) {
        // a <= b written as (b < a) ^ 1
        ir_var_t *tmp = ir_var_create(ir_insn->code->func, ir_operand_prim(ir_insn->operands[0]), NULL);
        // b < a
        ir_add_mach_insn(
            IR_BEFORE_INSN(ir_insn),
            true,
            IR_RETVAL_VAR(tmp),
            &rv_insn_slt,
            2,
            (ir_operand_t const[]){ir_insn->operands[1], ir_insn->operands[0]}
        );
        // (b < a) ^ 1
        ir_insn_t *new_node = ir_add_mach_insn(
            IR_BEFORE_INSN(ir_insn),
            true,
            ir_insn->returns[0],
            &rv_insn_xori,
            2,
            (ir_operand_t const[]){IR_OPERAND_VAR(tmp), IR_OPERAND_CONST(IR_CONST_U16(1))}
        );
        ir_insn_delete(ir_insn);
        return new_node;

    } else if (ir_insn->op2 == IR_OP2_seq || ir_insn->op2 == IR_OP2_sne) {
        // a == b written as (a ^ b) < 1u
        // a != b written as 0u < (a ^ b)
        ir_var_t *tmp = ir_var_create(ir_insn->code->func, ir_operand_prim(ir_insn->operands[0]), NULL);
        // a ^ b
        ir_add_mach_insn(IR_BEFORE_INSN(ir_insn), true, IR_RETVAL_VAR(tmp), &rv_insn_xor, 2, ir_insn->operands);
        ir_insn_t *new_node;
        if (ir_insn->op2 == IR_OP2_seq) {
            // (a ^ b) < 1u
            new_node = ir_add_mach_insn(
                IR_BEFORE_INSN(ir_insn),
                true,
                ir_insn->returns[0],
                &rv_insn_sltiu,
                2,
                (ir_operand_t const[]){IR_OPERAND_VAR(tmp), IR_OPERAND_CONST(IR_CONST_U16(1))}
            );
        } else {
            // 0u < (a ^ b)
            new_node = ir_add_mach_insn(
                IR_BEFORE_INSN(ir_insn),
                true,
                ir_insn->returns[0],
                &rv_insn_sltu,
                2,
                (ir_operand_t const[]){IR_OPERAND_CONST(IR_CONST_U16(1)), IR_OPERAND_VAR(tmp)}
            );
        }
        ir_insn_delete(ir_insn);
        return new_node;
    }

    insn_proto_t const *proto;
    switch (ir_insn->op2) {
        case IR_OP2_sgt: // Operands have been swapped.
        case IR_OP2_slt: proto = &rv_insn_slt; break;
        case IR_OP2_add: proto = &rv_insn_add; break;
        case IR_OP2_sub: proto = &rv_insn_sub; break;
        // case IR_OP2_mul:
        //     if (!profile->ext_enabled[RV_EXT_M]) {
        //         return NULL;
        //     }
        //     proto = &rv_insn_mul;
        //     break;
        // case IR_OP2_div:
        //     if (!profile->ext_enabled[RV_EXT_M]) {
        //         return NULL;
        //     }
        //     proto = &rv_insn_div;
        //     break;
        // case IR_OP2_rem:
        //     if (!profile->ext_enabled[RV_EXT_M]) {
        //         return NULL;
        //     }
        //     proto = &rv_insn_rem;
        //     break;
        case IR_OP2_shl: proto = &rv_insn_sll; break;
        case IR_OP2_shr:
            proto = ir_prim_is_signed(ir_insn->returns[0].dest_var->prim_type) ? &rv_insn_sra : &rv_insn_srl;
            break;
        case IR_OP2_band: proto = &rv_insn_and; break;
        case IR_OP2_bor: proto = &rv_insn_or; break;
        case IR_OP2_bxor: proto = &rv_insn_xor; break;
        default: return NULL;
    }

    ir_insn->type      = IR_INSN_MACHINE;
    ir_insn->prototype = proto;

    return ir_insn;
}

// Unary expressions.
static inline ir_insn_t *rv_isel_expr1(rv_profile_t const *profile, ir_insn_t *ir_insn) {
    (void)profile;
    ir_retval_t  dest = ir_insn->returns[0];
    ir_operand_t src  = ir_insn->operands[0];
    ir_insn_t   *new_node;

    switch (ir_insn->op1) {
        case IR_OP1_seqz: // sltiu rd, rs1, 1
            new_node = ir_add_mach_insn(
                IR_BEFORE_INSN(ir_insn),
                true,
                dest,
                &rv_insn_sltiu,
                2,
                (ir_operand_t const[]){src, IR_OPERAND_CONST(IR_CONST_U16(1))}
            );
            break;
        case IR_OP1_snez: // sltu rd, x0, rs2
            new_node = ir_add_mach_insn(
                IR_BEFORE_INSN(ir_insn),
                true,
                dest,
                &rv_insn_sltu,
                2,
                (ir_operand_t const[]){IR_OPERAND_REG(0)}
            );
            break;
        case IR_OP1_neg: // sub rd, x0, rs2
            new_node = ir_add_mach_insn(
                IR_BEFORE_INSN(ir_insn),
                true,
                dest,
                &rv_insn_sub,
                2,
                (ir_operand_t const[]){IR_OPERAND_REG(0)}
            );
            break;
        case IR_OP1_bneg: // xori rd, rs1, -1
            new_node = ir_add_mach_insn(
                IR_BEFORE_INSN(ir_insn),
                true,
                dest,
                &rv_insn_xori,
                2,
                (ir_operand_t const[]){src, IR_OPERAND_CONST(IR_CONST_S16(-1))}
            );
            break;
        default: return NULL;
    }

    ir_insn_delete(ir_insn);
    return new_node;
}

// Helper function for `rv_isel_mem_abs` that determines whether lui + addi can reach the target address.
static inline bool rv_memoff_fits_lui(rv_profile_t const *profile, uint64_t address) {
    if (profile->ext_enabled[RV_32ONLY]) {
        return true;
    }
    return address <= 0x7ffff7ff || 0xffffffff7ffff800 <= address;
}

// Memory instructions with absolute address.
static inline ir_insn_t *
    rv_isel_mem_abs(rv_profile_t const *profile, ir_insn_t *ir_insn, insn_proto_t const *lo12_proto) {
    ir_memref_t  memref = ir_insn->operands[0].mem;
    ir_operand_t ptr;
    uint16_t     offset;

    if (-0x800 <= memref.offset && memref.offset <= 0x7ff) {
        // Pointer offset fits in 12-bit immediate.
        ptr    = IR_OPERAND_REG(0);
        offset = memref.offset;
    } else if (rv_memoff_fits_lui(profile, (uint64_t)memref.offset)) {
        // Can use lui + addi to create offset.
        int32_t lui = (int32_t)(memref.offset >> 12);
        offset      = memref.offset << 4 >> 4;
        if (offset < 0) {
            lui++;
        }
        ir_prim_t ptr_prim = IR_PRIM_s8 + profile->base.ptr_bits * 2;
        ir_var_t *tmp      = ir_var_create(ir_insn->code->func, ptr_prim, NULL);
        ir_add_mach_insn(
            IR_BEFORE_INSN(ir_insn),
            true,
            IR_RETVAL_VAR(tmp),
            &rv_insn_lui,
            1,
            (ir_operand_t const[]){IR_OPERAND_CONST(IR_CONST_S32(lui))}
        );
        ptr = IR_OPERAND_VAR(tmp);
    } else {
        // Must load entire immediate.
        ptr    = IR_OPERAND_CONST(IR_CONST_S64(memref.offset));
        offset = 0;
    }

    // Emit memory access instruction.
    ir_insn_t *new_node;
    if (ir_insn->type == IR_INSN_STORE) {
        new_node = ir_add_mach_insn(
            IR_BEFORE_INSN(ir_insn),
            false,
            (ir_retval_t){},
            lo12_proto,
            2,
            (ir_operand_t const[]){
                IR_OPERAND_CONST(IR_CONST_S16(offset)),
                ptr,
                ir_insn->operands[1],
            }
        );
    } else {
        new_node = ir_add_mach_insn(
            IR_BEFORE_INSN(ir_insn),
            true,
            ir_insn->returns[0],
            lo12_proto,
            1,
            (ir_operand_t const[]){
                IR_OPERAND_CONST(IR_CONST_S16(offset)),
                ptr,
            }
        );
    }

    ir_insn_delete(ir_insn);
    return new_node;
}

// Memory instructions with pointer and offset.
static inline ir_insn_t *
    rv_isel_mem_var(rv_profile_t const *profile, ir_insn_t *ir_insn, insn_proto_t const *lo12_proto) {
    (void)profile;
    ir_memref_t memref = ir_insn->operands[0].mem;
    // TODO: Optimize add/sub with constant into the offset for memref?
    ir_var_t   *ptr;
    uint16_t    offset;

    if (-0x800 <= memref.offset && memref.offset <= 0x7ff) {
        // Pointer offset fits in 12-bit immediate.
        ptr    = memref.base_var;
        offset = memref.offset;
    } else {
        // Need full-width add to offset from the pointer.
        ptr = ir_var_create(ir_insn->code->func, memref.base_var->prim_type, NULL);
        ir_const_t iconst;
        iconst.prim_type = memref.base_var->prim_type;
        iconst.consth    = memref.offset < 0 ? -1 : 0;
        iconst.constl    = memref.offset;
        ir_add_mach_insn(
            IR_BEFORE_INSN(ir_insn),
            true,
            IR_RETVAL_VAR(ptr),
            &rv_insn_add,
            2,
            (ir_operand_t const[]){
                IR_OPERAND_VAR(memref.base_var),
                IR_OPERAND_CONST(iconst),
            }
        );
        offset = 0;
    }

    // Emit memory access instruction.
    ir_insn_t *new_node;
    if (ir_insn->type == IR_INSN_STORE) {
        new_node = ir_add_mach_insn(
            IR_BEFORE_INSN(ir_insn),
            false,
            (ir_retval_t){},
            lo12_proto,
            2,
            (ir_operand_t const[]){
                IR_OPERAND_MEM(IR_MEMREF(memref.data_type, IR_BADDR_VAR(ptr), .offset = offset)),
                ir_insn->operands[1],
            }
        );
    } else {
        new_node = ir_add_mach_insn(
            IR_BEFORE_INSN(ir_insn),
            true,
            ir_insn->returns[0],
            lo12_proto,
            1,
            (ir_operand_t const[]){
                IR_OPERAND_MEM(IR_MEMREF(memref.data_type, IR_BADDR_VAR(ptr), .offset = offset)),
            }
        );
    }

    ir_insn_delete(ir_insn);
    return new_node;
}

// Memory instructions.
static inline ir_insn_t *rv_isel_mem(rv_profile_t const *profile, ir_insn_t *ir_insn) {
    insn_proto_t const *lo12_proto;
    if (ir_insn->type == IR_INSN_LOAD) {
        switch (ir_insn->operands[0].mem.data_type) {
            case IR_PRIM_s8: lo12_proto = &rv_insn_lb; break;
            case IR_PRIM_bool:
            case IR_PRIM_u8: lo12_proto = &rv_insn_lbu; break;
            case IR_PRIM_s16: lo12_proto = &rv_insn_lh; break;
            case IR_PRIM_u16: lo12_proto = &rv_insn_lhu; break;
            case IR_PRIM_s32:
            case IR_PRIM_u32: lo12_proto = &rv_insn_lw; break;
            // case IR_PRIM_s64:
            // case IR_PRIM_u64: lo12_proto = &rv_insn_ld; break;
            // case IR_PRIM_f32: lo12_proto = &rv_insn_flw; break;
            // case IR_PRIM_f64: lo12_proto = &rv_insn_fld; break;
            default: return false;
        }
    } else if (ir_insn->type == IR_INSN_STORE) {
        switch (ir_insn->operands[0].mem.data_type) {
            case IR_PRIM_s8:
            case IR_PRIM_bool:
            case IR_PRIM_u8: lo12_proto = &rv_insn_sb; break;
            case IR_PRIM_s16:
            case IR_PRIM_u16: lo12_proto = &rv_insn_sh; break;
            case IR_PRIM_s32:
            case IR_PRIM_u32: lo12_proto = &rv_insn_sw; break;
            // case IR_PRIM_s64:
            // case IR_PRIM_u64: lo12_proto = &rv_insn_sd; break;
            // case IR_PRIM_f32: lo12_proto = &rv_insn_fsw; break;
            // case IR_PRIM_f64: lo12_proto = &rv_insn_fsd; break;
            default: return false;
        }
    } else {
        lo12_proto = &rv_insn_addi;
    }

    if (ir_insn->operands[0].mem.base_type == IR_MEMBASE_ABS) {
        // Absolute address memory access.
        return rv_isel_mem_abs(profile, ir_insn, lo12_proto);
    } else if (ir_insn->operands[0].mem.base_type == IR_MEMBASE_VAR) {
        // Pointer+offset memory access.
        return rv_isel_mem_var(profile, ir_insn, lo12_proto);
    }

    // Stack or symbol memory access.
    ir_insn_t *new_node;
    if (ir_insn->type == IR_INSN_STORE) {
        new_node = ir_add_mach_insn(
            IR_BEFORE_INSN(ir_insn),
            false,
            (ir_retval_t){},
            lo12_proto,
            2,
            (ir_operand_t const[]){ir_insn->operands[1], ir_insn->operands[0]}
        );
    } else {
        new_node = ir_add_mach_insn(
            IR_BEFORE_INSN(ir_insn),
            true,
            ir_insn->returns[0],
            lo12_proto,
            1,
            (ir_operand_t const[]){ir_insn->operands[0]}
        );
    }

    ir_insn_delete(ir_insn);
    return new_node;
}

// Perform instruction selection for expressions, memory access and branches.
ir_insn_t *rv_isel(backend_profile_t *base_profile, ir_insn_t *ir_insn) {
    rv_profile_t const *profile = (void *)base_profile;

    if (ir_insn->type == IR_INSN_EXPR1 && (ir_insn->op1 == IR_OP1_mov || ir_insn->op1 == IR_OP1_bitcast)) {
        if (ir_insn->operands[0].type == IR_OPERAND_TYPE_CONST) {
            if (ir_insn->op1 == IR_OP1_bitcast) {
                // Constant bit-cast.
                return rv_isel_const_bitcast(profile, ir_insn);
            } else {
                // Constant mov.
                return rv_isel_const_mov(profile, ir_insn);
            }
        } else {
            if (ir_insn->op1 == IR_OP1_bitcast) {
                // Register-register bitcast.
                return rv_isel_rr_bitcast(profile, ir_insn);
            } else {
                // Register-register mov.
                return rv_isel_rr_mov(profile, ir_insn);
            }
        }
    } else if (ir_insn->type == IR_INSN_JUMP) {
        // Jump instruction.
        return rv_isel_jump(profile, ir_insn);
    } else if (ir_insn->type == IR_INSN_BRANCH) {
        // Branch instruction patterns.
        return rv_isel_branch(profile, ir_insn);
    } else if (ir_insn->type == IR_INSN_EXPR2) {
        // Binary expressions.
        return rv_isel_expr2_ri(profile, ir_insn) ?: rv_isel_expr2_rr(profile, ir_insn);
    } else if (ir_insn->type == IR_INSN_EXPR1) {
        // Unary expressions.
        return rv_isel_expr1(profile, ir_insn);
    } else if (ir_insn->type == IR_INSN_LEA || ir_insn->type == IR_INSN_LOAD || ir_insn->type == IR_INSN_STORE) {
        // Memory instructions.
        return rv_isel_mem(profile, ir_insn);
    }

    // Cannot generate this instruction.
    return NULL;
}

// Post-isel pass that creates instructions for loading immediates where only registers are accepted.
void rv_post_isel(backend_profile_t *profile, ir_func_t *func);
