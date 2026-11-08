
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "rv_isel.h"

#include "arith128.h"
#include "backend.h"
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
        return profile->ext_enabled[RV_64] ? IR_PRIM_i64 : IR_PRIM_i32;
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

    ir_insn_t *new_node  = ir_add_mach_insn(loc, true, dest, &rv_insn_mv, 1, (ir_operand_t const[]){src});
    new_node->flags     |= IR_INSN_FLAG_RR_COPY;
    return new_node;
}

static ir_insn_t *rv_emit_rr_zeroext(
    rv_profile_t const *profile, ir_insnloc_t loc, ir_retval_t dest, ir_operand_t src, unsigned int dest_bit
) {
    unsigned int xlen = profile->ext_enabled[RV_64] ? 64 : 32;
    if (dest_bit == xlen) {
        // mv dest, src
        ir_insn_t *node  = rv_add_mv(loc, dest, src);
        node->flags     |= IR_INSN_FLAG_RR_COPY | IR_INSN_FLAG_CLAMPING | IR_INSN_FLAG_ZEROEXT;
        return node;
    } else if (dest_bit < 12) {
        // andi dest, src, (1 << dest_bit) - 1
        ir_insn_t *node  = rv_add_andi(loc, dest, src, (1 << dest_bit) - 1);
        node->flags     |= IR_INSN_FLAG_RR_COPY | IR_INSN_FLAG_CLAMPING | IR_INSN_FLAG_ZEROEXT;
        return node;
    } else if (dest_bit == 32 && profile->ext_enabled[RV_64]) {
        // addiw dest, src, 0
        ir_insn_t *node  = rv_add_addiw(loc, dest, src, 0);
        node->flags     |= IR_INSN_FLAG_RR_COPY | IR_INSN_FLAG_CLAMPING | IR_INSN_FLAG_ZEROEXT;
        return node;
    } else {
        ir_var_t  *tmpvar  = ir_var_create(ir_insnloc_code(loc)->func, ir_operand_prim(src), NULL);
        // slli tmp, src, XLEN - dest_bit
        ir_insn_t *node1   = rv_add_slli(loc, IR_RETVAL_VAR(tmpvar), src, (int)(xlen - dest_bit));
        loc                = IR_AFTER_INSN(node1);
        ir_insn_t *node2   = rv_add_srli(loc, dest, IR_OPERAND_VAR(tmpvar), (int)(xlen - dest_bit));
        // srli dest, tmp, XLEN - dest_bit
        node1->flags      |= IR_INSN_FLAG_RR_COPY | IR_INSN_FLAG_CLAMPING | IR_INSN_FLAG_ZEROEXT;
        node2->flags      |= IR_INSN_FLAG_RR_COPY | IR_INSN_FLAG_CLAMPING | IR_INSN_FLAG_ZEROEXT;
        return node2;
    }
}

static ir_insn_t *rv_emit_rr_signext(
    rv_profile_t const *profile, ir_insnloc_t loc, ir_retval_t dest, ir_operand_t src, unsigned int dest_bit
) {
    unsigned int xlen = profile->ext_enabled[RV_64] ? 64 : 32;
    if (dest_bit == xlen) {
        // mv dest, src
        ir_insn_t *node  = rv_add_mv(loc, dest, src);
        node->flags     |= IR_INSN_FLAG_RR_COPY | IR_INSN_FLAG_CLAMPING | IR_INSN_FLAG_SIGNEXT;
        return node;
    } else if (dest_bit == 32 && profile->ext_enabled[RV_64]) {
        // addiw dest, src, 0
        ir_insn_t *node  = rv_add_addiw(loc, dest, src, 0);
        node->flags     |= IR_INSN_FLAG_RR_COPY | IR_INSN_FLAG_CLAMPING | IR_INSN_FLAG_SIGNEXT;
        return node;
    } else {
        ir_var_t  *tmpvar  = ir_var_create(ir_insnloc_code(loc)->func, ir_operand_prim(src), NULL);
        // slli tmp, src, XLEN - dest_bit
        ir_insn_t *node1   = rv_add_slli(loc, IR_RETVAL_VAR(tmpvar), src, (int)(xlen - dest_bit));
        loc                = IR_AFTER_INSN(node1);
        ir_insn_t *node2   = rv_add_srai(loc, dest, IR_OPERAND_VAR(tmpvar), (int)(xlen - dest_bit));
        // srai dest, tmp, XLEN - dest_bit
        node1->flags      |= IR_INSN_FLAG_RR_COPY | IR_INSN_FLAG_CLAMPING | IR_INSN_FLAG_SIGNEXT;
        node2->flags      |= IR_INSN_FLAG_RR_COPY | IR_INSN_FLAG_CLAMPING | IR_INSN_FLAG_SIGNEXT;
        return node2;
    }
}

// Move a constant into a register if needed.
static void rv_operand_to_reg(rv_profile_t const *profile, ir_insn_t *insn, size_t i) {
    (void)profile;
    if (insn->operands[i].type != IR_OPERAND_TYPE_CONST) {
        return;
    }
    if (ir_prim_is_integer(insn->operands[i].iconst.prim_type)
        && cmp128u(insn->operands[i].iconst.const128, I128_ZERO) == 0) {
        insn->operands[i] = IR_OPERAND_REG(0);
    } else {
        ir_var_t *tmp = ir_var_create(insn->code->func, insn->operands[i].iconst.prim_type, NULL);
        ir_add_mach_insn(
            IR_BEFORE_INSN(insn),
            true,
            IR_RETVAL_VAR(tmp),
            &rv_insn_li,
            1,
            (ir_operand_t const[]){insn->operands[i]}
        );
        ir_insn_set_operand(insn, i, IR_OPERAND_VAR(tmp));
    }
}


// Register-register copy/bitcast.
static ir_insn_t *rv_isel_rr_copy(rv_profile_t const *profile, ir_insn_t *insn) {
    (void)profile;
    ir_insn_t *new_node = rv_emit_rr_copy(profile, IR_BEFORE_INSN(insn), insn->returns[0], insn->operands[0]);
    ir_insn_delete(insn);
    return new_node;
}

// Register-register sign extend.
static ir_insn_t *rv_isel_rr_signext(rv_profile_t const *profile, ir_insn_t *insn) {
    if (insn->returns[0].type != IR_RETVAL_TYPE_VAR) {
        return NULL;
    }

    rv_operand_to_reg(profile, insn, 0);
    unsigned int ret_bit = ir_prim_bits(insn->returns[0].dest_var->orig_prim_type);
    unsigned int val_bit = ir_prim_bits(insn->operands[0].var->orig_prim_type);

    ir_insn_t *new_node = rv_emit_rr_signext(
        profile,
        IR_BEFORE_INSN(insn),
        insn->returns[0],
        insn->operands[0],
        ret_bit < val_bit ? ret_bit : val_bit
    );

    ir_insn_delete(insn);
    return new_node;
}

// Register-register zero extend.
static ir_insn_t *rv_isel_rr_zeroext(rv_profile_t const *profile, ir_insn_t *insn) {
    if (insn->returns[0].type != IR_RETVAL_TYPE_VAR) {
        return NULL;
    }

    rv_operand_to_reg(profile, insn, 0);
    unsigned int ret_bit = ir_prim_bits(insn->returns[0].dest_var->orig_prim_type);
    unsigned int val_bit = ir_prim_bits(insn->operands[0].var->orig_prim_type);

    ir_insn_t *new_node = rv_emit_rr_zeroext(
        profile,
        IR_BEFORE_INSN(insn),
        insn->returns[0],
        insn->operands[0],
        ret_bit < val_bit ? ret_bit : val_bit
    );

    ir_insn_delete(insn);
    return new_node;
}

// Constant mov.
static ir_insn_t *rv_isel_const_mov(rv_profile_t const *profile, ir_insn_t *insn) {
    (void)profile;
    ir_const_t const iconst = ir_trim_const(insn->operands[0].iconst, true);

    if (iconst.prim_type == IR_PRIM_f32 || iconst.prim_type == IR_PRIM_f64) {
        fprintf(stderr, "TODO: Emit constant for FP\n");
        return NULL;
    }

    // li dest, imm
    ir_insn_t *new_node
        = ir_add_mach_insn(IR_BEFORE_INSN(insn), true, insn->returns[0], &rv_insn_li, 1, insn->operands);
    ir_insn_delete(insn);

    return new_node;
}

// Constant bitcast.
static ir_insn_t *rv_isel_const_bitcast(rv_profile_t const *profile, ir_insn_t *insn) {
    insn->operands[0].iconst = ir_cast(rv_retval_prim(profile, insn->returns[0]), insn->operands[0].iconst, true);
    return rv_isel_const_mov(profile, insn);
}


// Jump instruction.
static inline ir_insn_t *rv_isel_jump(rv_profile_t const *profile, ir_insn_t *insn) {
    (void)profile;
    ir_insn_t *new_node = ir_add_mach_insn(IR_BEFORE_INSN(insn), false, (ir_retval_t){}, &rv_insn_j, 1, insn->operands);
    ir_insn_delete(insn);
    return new_node;
}

// Compare-branch instruction patterns.
static inline ir_insn_t *rv_isel_cmp_branch(rv_profile_t const *profile, ir_insn_t *insn) {
    (void)profile;
    // Check that the branch is given a variable...
    if (insn->operands[1].type != IR_OPERAND_TYPE_VAR) {
        return NULL;
    }
    // ...which is initialized...
    set_ent_t const *pred = set_next(&insn->operands[1].var->assigned_at, NULL);
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
        rv_operand_to_reg(profile, pred_insn, 0);
        ir_insn_t *new_node = ir_add_mach_insn(
            IR_BEFORE_INSN(insn),
            false,
            (ir_retval_t){},
            proto,
            3,
            (ir_operand_t const[]){
                pred_insn->operands[0],
                IR_OPERAND_REG(0),
                insn->operands[0],
            }
        );

        if (pred_insn->returns[0].type == IR_RETVAL_TYPE_VAR && pred_insn->returns[0].dest_var->used_at.len == 1) {
            ir_insn_delete(pred_insn);
        }
        ir_insn_delete(insn);
        return new_node;

    } else if (pred_insn->type == IR_INSN_EXPR2) {
        // ...or comparison expr2.
        insn_proto_t const *proto;
        bool                swap;
        switch (pred_insn->op2) {
            case IR_OP2_sgtu: // bltu rs2, rs1, dest
                proto = &rv_insn_bltu;
                swap  = true;
                break;
            case IR_OP2_sleu: // bltu rs1, rs2, dest
                proto = &rv_insn_bltu;
                swap  = false;
                break;
            case IR_OP2_sltu: // bgeu rs2, rs1, dest
                proto = &rv_insn_bgeu;
                swap  = true;
                break;
            case IR_OP2_sgeu: // bgeu rs1, rs2, dest
                proto = &rv_insn_bgeu;
                swap  = false;
                break;
            case IR_OP2_sgts: // blt rs2, rs1, dest
                proto = &rv_insn_blt;
                swap  = true;
                break;
            case IR_OP2_sles: // blt rs1, rs2, dest
                proto = &rv_insn_blt;
                swap  = false;
                break;
            case IR_OP2_slts: // bge rs2, rs1, dest
                proto = &rv_insn_bge;
                swap  = true;
                break;
            case IR_OP2_sges: // bge rs1, rs2, dest
                proto = &rv_insn_bge;
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
        rv_operand_to_reg(profile, pred_insn, 0);
        rv_operand_to_reg(profile, pred_insn, 1);
        ir_insn_t *new_node = ir_add_mach_insn(
            IR_BEFORE_INSN(insn),
            false,
            (ir_retval_t){},
            proto,
            3,
            (ir_operand_t const[]){pred_insn->operands[swap], pred_insn->operands[!swap], insn->operands[0]}
        );

        if (pred_insn->returns[0].type == IR_RETVAL_TYPE_VAR && pred_insn->returns[0].dest_var->used_at.len == 1) {
            ir_insn_delete(pred_insn);
        }
        ir_insn_delete(insn);
        return new_node;

    } else {
        return NULL;
    }
}

// Branch instruction patterns.
static inline ir_insn_t *rv_isel_branch(rv_profile_t const *profile, ir_insn_t *insn) {
    ir_insn_t *new_node = rv_isel_cmp_branch(profile, insn);
    if (new_node) {
        return new_node;
    }

    rv_operand_to_reg(profile, insn, 0);

    // bne rs1, x0, dest
    new_node = ir_add_mach_insn(
        IR_BEFORE_INSN(insn),
        false,
        (ir_retval_t){},
        &rv_insn_bne,
        3,
        (ir_operand_t const[]){insn->operands[1], IR_OPERAND_REG(0), insn->operands[0]}
    );

    ir_insn_delete(insn);
    assert(new_node->node.next != &insn->node);
    assert(!dlist_contains(&new_node->code->insns, &insn->node));

    return new_node;
}

// Binary expressions.
static inline ir_insn_t *rv_isel_expr2_ri(rv_profile_t const *profile, ir_insn_t *insn) {
    (void)profile;

    if (insn->operands[1].type != IR_OPERAND_TYPE_CONST) {
        return NULL;
    }
    ir_const_t iconst = insn->operands[1].iconst;
    if (insn->op2 == IR_OP2_sub) {
        iconst = ir_calc1(IR_OP1_neg, iconst);
    }
    if (iconst.prim_type == IR_PRIM_f32 || iconst.prim_type == IR_PRIM_f64 || ir_count_bits(iconst, true, false) > 12) {
        return NULL;
    }

    bool op32 = profile->ext_enabled[RV_64] && iconst.prim_type == IR_PRIM_i32;

    insn_proto_t const *proto;
    switch (insn->op2) {
        // case IR_OP2_sgt:
        // case IR_OP2_sle:
        // case IR_OP2_slt:
        // case IR_OP2_sge:
        // case IR_OP2_seq:
        // case IR_OP2_sne:
        case IR_OP2_add:
        case IR_OP2_sub: proto = op32 ? &rv_insn_addiw : &rv_insn_addi; break;
        case IR_OP2_shl: proto = op32 ? &rv_insn_slliw : &rv_insn_slli; break;
        case IR_OP2_shru: proto = op32 ? &rv_insn_srliw : &rv_insn_srli; break;
        case IR_OP2_shrs: proto = op32 ? &rv_insn_sraiw : &rv_insn_srai; break;
        case IR_OP2_band: proto = &rv_insn_andi; break;
        case IR_OP2_bor: proto = &rv_insn_ori; break;
        case IR_OP2_bxor: proto = &rv_insn_xori; break;
        default: return NULL;
    }

    rv_operand_to_reg(profile, insn, 0);

    insn->type               = IR_INSN_MACHINE;
    insn->prototype          = proto;
    insn->operands[1].iconst = iconst;

    return insn;
}

// Binary expressions.
static inline ir_insn_t *rv_isel_expr2_rr(rv_profile_t const *profile, ir_insn_t *insn) {
    (void)profile;
    if (insn->returns[0].type == IR_RETVAL_TYPE_VAR
        && (insn->returns[0].dest_var->prim_type == IR_PRIM_f32
            || insn->returns[0].dest_var->prim_type == IR_PRIM_f64)) {
        return NULL;
    }

    if (insn->op2 == IR_OP2_sgtu || insn->op2 == IR_OP2_sgeu || insn->op2 == IR_OP2_sgts || insn->op2 == IR_OP2_sges) {
        // Swap operands as RISC-V only has slt and an emulated sle.
        ir_operand_t tmp  = insn->operands[0];
        insn->operands[0] = insn->operands[1];
        insn->operands[1] = tmp;
    }

    for (size_t i = 0; i < 2; i++) {
        rv_operand_to_reg(profile, insn, i);
    }
    bool op32 = profile->ext_enabled[RV_64] && ir_operand_prim(insn->operands[0]) == IR_PRIM_i32;

    if (insn->op2 == IR_OP2_sleu || insn->op2 == IR_OP2_sgeu || insn->op2 == IR_OP2_sgts || insn->op2 == IR_OP2_sges) {
        // a <= b written as (b < a) ^ 1
        ir_var_t *tmp = ir_var_create(insn->code->func, ir_operand_prim(insn->operands[0]), NULL);
        // b < a
        ir_add_mach_insn(
            IR_BEFORE_INSN(insn),
            true,
            IR_RETVAL_VAR(tmp),
            ir_op2_get_sign(insn->op2) == IR_SIGN_SIGNEXT ? &rv_insn_slt : &rv_insn_sltu,
            2,
            (ir_operand_t const[]){insn->operands[1], insn->operands[0]}
        );
        // (b < a) ^ 1
        ir_insn_t *new_node = ir_add_mach_insn(
            IR_BEFORE_INSN(insn),
            true,
            insn->returns[0],
            &rv_insn_xori,
            2,
            (ir_operand_t const[]){IR_OPERAND_VAR(tmp), IR_OPERAND_CONST(IR_CONST_U16(1))}
        );
        ir_insn_delete(insn);
        return new_node;

    } else if (insn->op2 == IR_OP2_seq || insn->op2 == IR_OP2_sne) {
        // a == b written as (a ^ b) < 1u
        // a != b written as 0u < (a ^ b)
        ir_var_t *tmp = ir_var_create(insn->code->func, ir_operand_prim(insn->operands[0]), NULL);
        // a ^ b
        ir_add_mach_insn(IR_BEFORE_INSN(insn), true, IR_RETVAL_VAR(tmp), &rv_insn_xor, 2, insn->operands);
        ir_insn_t *new_node;
        if (insn->op2 == IR_OP2_seq) {
            // (a ^ b) < 1u
            new_node = ir_add_mach_insn(
                IR_BEFORE_INSN(insn),
                true,
                insn->returns[0],
                &rv_insn_sltiu,
                2,
                (ir_operand_t const[]){IR_OPERAND_VAR(tmp), IR_OPERAND_CONST(IR_CONST_U16(1))}
            );
        } else {
            // 0u < (a ^ b)
            new_node = ir_add_mach_insn(
                IR_BEFORE_INSN(insn),
                true,
                insn->returns[0],
                &rv_insn_sltu,
                2,
                (ir_operand_t const[]){IR_OPERAND_REG(0), IR_OPERAND_VAR(tmp)}
            );
        }
        ir_insn_delete(insn);
        return new_node;
    }

    insn_proto_t const *proto;
    switch (insn->op2) {
        case IR_OP2_sgtu: // Operands have been swapped.
        case IR_OP2_sltu: proto = &rv_insn_sltu; break;
        case IR_OP2_sgts: // Operands have been swapped.
        case IR_OP2_slts: proto = &rv_insn_slt; break;
        case IR_OP2_add: proto = op32 ? &rv_insn_addw : &rv_insn_add; break;
        case IR_OP2_sub: proto = op32 ? &rv_insn_subw : &rv_insn_sub; break;
        case IR_OP2_mul:
            if (!profile->ext_enabled[RV_EXT_M]) {
                return NULL;
            }
            proto = op32 ? &rv_insn_mulw : &rv_insn_mul;
            break;
        case IR_OP2_divu:
            if (!profile->ext_enabled[RV_EXT_M]) {
                return NULL;
            }
            proto = op32 ? &rv_insn_divuw : &rv_insn_divu;
            break;
        case IR_OP2_divs:
            if (!profile->ext_enabled[RV_EXT_M]) {
                return NULL;
            }
            proto = op32 ? &rv_insn_divw : &rv_insn_div;
            break;
        case IR_OP2_remu:
            if (!profile->ext_enabled[RV_EXT_M]) {
                return NULL;
            }
            proto = op32 ? &rv_insn_remuw : &rv_insn_remu;
            break;
        case IR_OP2_rems:
            if (!profile->ext_enabled[RV_EXT_M]) {
                return NULL;
            }
            proto = op32 ? &rv_insn_remw : &rv_insn_rem;
            break;
        case IR_OP2_shl: proto = op32 ? &rv_insn_sllw : &rv_insn_sll; break;
        case IR_OP2_shru: proto = op32 ? &rv_insn_srlw : &rv_insn_srl; break;
        case IR_OP2_shrs: proto = op32 ? &rv_insn_sraw : &rv_insn_sra; break;
        case IR_OP2_band: proto = &rv_insn_and; break;
        case IR_OP2_bor: proto = &rv_insn_or; break;
        case IR_OP2_bxor: proto = &rv_insn_xor; break;
        default: return NULL;
    }

    insn->type      = IR_INSN_MACHINE;
    insn->prototype = proto;

    return insn;
}

// Unary expressions.
static inline ir_insn_t *rv_isel_expr1(rv_profile_t const *profile, ir_insn_t *insn) {
    (void)profile;
    ir_retval_t  dest = insn->returns[0];
    ir_operand_t src  = insn->operands[0];
    ir_insn_t   *new_node;

    rv_operand_to_reg(profile, insn, 0);

    switch (insn->op1) {
        case IR_OP1_seqz: // sltiu rd, rs1, 1
            new_node = ir_add_mach_insn(
                IR_BEFORE_INSN(insn),
                true,
                dest,
                &rv_insn_sltiu,
                2,
                (ir_operand_t const[]){src, IR_OPERAND_CONST(IR_CONST_U16(1))}
            );
            break;
        case IR_OP1_snez: // sltu rd, x0, rs2
            new_node = ir_add_mach_insn(
                IR_BEFORE_INSN(insn),
                true,
                dest,
                &rv_insn_sltu,
                2,
                (ir_operand_t const[]){IR_OPERAND_REG(0), src}
            );
            break;
        case IR_OP1_neg: // sub rd, x0, rs2
            new_node = ir_add_mach_insn(
                IR_BEFORE_INSN(insn),
                true,
                dest,
                &rv_insn_sub,
                2,
                (ir_operand_t const[]){IR_OPERAND_REG(0), src}
            );
            break;
        case IR_OP1_bneg: // xori rd, rs1, -1
            new_node = ir_add_mach_insn(
                IR_BEFORE_INSN(insn),
                true,
                dest,
                &rv_insn_xori,
                2,
                (ir_operand_t const[]){src, IR_OPERAND_CONST(IR_CONST_S16(-1))}
            );
            break;
        default: return NULL;
    }

    ir_insn_delete(insn);
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
        ir_prim_t ptr_prim = IR_PRIM_i8 + (profile->base.ptr_bits - LILY_8_BITS);
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
                ir_insn->operands[1],
                IR_OPERAND_MEM(IR_MEMREF(memref.data_type, IR_BADDR_VAR(ptr), .offset = offset)),
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
static inline ir_insn_t *rv_isel_mem(rv_profile_t const *profile, ir_insn_t *insn) {
    insn_proto_t const *lo12_proto;
    if (insn->type == IR_INSN_LOAD) {
        bool sign = insn->operands[0].sign != IR_SIGN_ZEROEXT;
        switch (insn->operands[0].mem.data_type) {
            case IR_PRIM_bool:
            case IR_PRIM_i8: lo12_proto = sign ? &rv_insn_lb : &rv_insn_lbu; break;
            case IR_PRIM_i16: lo12_proto = sign ? &rv_insn_lh : &rv_insn_lhu; break;
            case IR_PRIM_i32: lo12_proto = sign || !profile->ext_enabled[RV_64] ? &rv_insn_lw : &rv_insn_lwu; break;
            case IR_PRIM_i64: lo12_proto = &rv_insn_ld; break;
            // case IR_PRIM_f32: lo12_proto = &rv_insn_flw; break;
            // case IR_PRIM_f64: lo12_proto = &rv_insn_fld; break;
            default: return false;
        }
    } else if (insn->type == IR_INSN_STORE) {
        switch (insn->operands[0].mem.data_type) {
            case IR_PRIM_bool:
            case IR_PRIM_i8: lo12_proto = &rv_insn_sb; break;
            case IR_PRIM_i16: lo12_proto = &rv_insn_sh; break;
            case IR_PRIM_i32: lo12_proto = &rv_insn_sw; break;
            case IR_PRIM_i64: lo12_proto = &rv_insn_sd; break;
            // case IR_PRIM_f32: lo12_proto = &rv_insn_fsw; break;
            // case IR_PRIM_f64: lo12_proto = &rv_insn_fsd; break;
            default: return false;
        }
    } else {
        lo12_proto = &rv_insn_la;
    }

    if (insn->operands[0].mem.base_type == IR_MEMBASE_ABS) {
        // Absolute address memory access.
        return rv_isel_mem_abs(profile, insn, lo12_proto);
    } else if (insn->operands[0].mem.base_type == IR_MEMBASE_VAR) {
        // Pointer+offset memory access.
        return rv_isel_mem_var(profile, insn, lo12_proto);
    }

    // Stack or symbol memory access.
    ir_insn_t *new_node;
    if (insn->type == IR_INSN_STORE) {
        new_node = ir_add_mach_insn(
            IR_BEFORE_INSN(insn),
            false,
            (ir_retval_t){},
            lo12_proto,
            2,
            (ir_operand_t const[]){insn->operands[1], insn->operands[0]}
        );
    } else {
        new_node = ir_add_mach_insn(
            IR_BEFORE_INSN(insn),
            true,
            insn->returns[0],
            lo12_proto,
            1,
            (ir_operand_t const[]){insn->operands[0]}
        );
    }

    ir_insn_delete(insn);
    return new_node;
}

// Perform instruction selection for expressions, memory access and branches.
ir_insn_t *rv_isel(backend_profile_t *base_profile, ir_insn_t *insn) {
    rv_profile_t const *profile = (void *)base_profile;

    if (insn->type == IR_INSN_EXPR1) {
        if (insn->operands[0].type == IR_OPERAND_TYPE_CONST) {
            switch (insn->op1) {
                case IR_OP1_signext:
                case IR_OP1_zeroext:
                case IR_OP1_mov: return rv_isel_const_mov(profile, insn);
                case IR_OP1_bitcast: return rv_isel_const_bitcast(profile, insn);
                default: break;
            }
        } else if (insn->operands[0].type == IR_OPERAND_TYPE_REG) {
            return rv_isel_rr_copy(profile, insn);
        } else if (insn->operands[0].type == IR_OPERAND_TYPE_VAR) {
            switch (insn->op1) {
                case IR_OP1_signext: return rv_isel_rr_signext(profile, insn);
                case IR_OP1_zeroext: return rv_isel_rr_zeroext(profile, insn);
                case IR_OP1_mov:
                case IR_OP1_bitcast: return rv_isel_rr_copy(profile, insn);
                default: break;
            }
        }
    }
    if (insn->type == IR_INSN_JUMP) {
        // Jump instruction.
        assert(insn->operands[0].mem.base_type == IR_MEMBASE_CODE);
        return rv_isel_jump(profile, insn);
    } else if (insn->type == IR_INSN_BRANCH) {
        // Branch instruction patterns.
        assert(insn->operands[0].type == IR_OPERAND_TYPE_MEM);
        assert(insn->operands[0].mem.base_type == IR_MEMBASE_CODE);
        return rv_isel_branch(profile, insn);
    } else if (insn->type == IR_INSN_EXPR2) {
        // Binary expressions.
        assert(insn->operands[0].type != IR_OPERAND_TYPE_MEM);
        assert(insn->operands[1].type != IR_OPERAND_TYPE_MEM);
        return rv_isel_expr2_ri(profile, insn) ?: rv_isel_expr2_rr(profile, insn);
    } else if (insn->type == IR_INSN_EXPR1) {
        // Unary expressions.
        assert(insn->operands[0].type != IR_OPERAND_TYPE_MEM);
        return rv_isel_expr1(profile, insn);
    } else if (insn->type == IR_INSN_LEA || insn->type == IR_INSN_LOAD || insn->type == IR_INSN_STORE) {
        // Memory instructions.
        assert(insn->operands[0].type == IR_OPERAND_TYPE_MEM);
        if (insn->type == IR_INSN_STORE) {
            rv_operand_to_reg(profile, insn, 1);
        }
        return rv_isel_mem(profile, insn);
    }

    // Cannot generate this instruction.
    return NULL;
}

// Post-isel pass that creates instructions for loading immediates where only registers are accepted.
void rv_post_isel(backend_profile_t *profile, ir_func_t *func);
