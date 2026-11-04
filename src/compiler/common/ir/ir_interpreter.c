
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir/ir_interpreter.h"

#include "arith128.h"
#include "ir_types.h"
#include "list.h"
#include "set.h"
#include "unreachable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



// Get the range of possible values from an IR operand.
bool ir_get_operand_range(
    ir_operand_t operand, i128_t *minu_out, i128_t *maxu_out, i128_t *mins_out, i128_t *maxs_out
) {
    switch (operand.type) {
        case IR_OPERAND_TYPE_CONST:
            if (operand.iconst.prim_type >= IR_PRIM_f32) {
                return false;
            }
            *minu_out = operand.iconst.const128;
            *maxu_out = operand.iconst.const128;
            *mins_out = operand.iconst.const128;
            *maxs_out = operand.iconst.const128;
            return true;
        case IR_OPERAND_TYPE_VAR:
            if (operand.var->prim_type >= IR_PRIM_f32) {
                return false;
            }
            *minu_out = operand.var->range_minu;
            *maxu_out = operand.var->range_maxu;
            *mins_out = operand.var->range_mins;
            *maxs_out = operand.var->range_maxs;
            return true;
        case IR_OPERAND_TYPE_UNDEF:
        case IR_OPERAND_TYPE_MEM:
        case IR_OPERAND_TYPE_STRUCT:
        case IR_OPERAND_TYPE_REG:
        default: return false;
    }
}

// Update the possible range according to an expr2 instruction.
void ir_expr2_range(ir_var_t *var, ir_insn_t const *insn) {
    if (var->prim_type >= IR_PRIM_f32) {
        return;
    }

    switch (insn->op2) {
        case IR_OP2_sgts:
        case IR_OP2_sles:
        case IR_OP2_slts:
        case IR_OP2_sges:
        case IR_OP2_sgtu:
        case IR_OP2_sleu:
        case IR_OP2_sltu:
        case IR_OP2_sgeu:
        case IR_OP2_seq:
        case IR_OP2_sne: ir_expand_range(var, I128_ZERO, ui128(1), I128_ZERO, ui128(1)); return;
        default: break;
    }

    i128_t const prim_minu = UI128_ZERO;
    i128_t const prim_maxu = ir_prim_max(var->prim_type, false);
    i128_t const prim_mins = ir_prim_min(var->prim_type, true);
    i128_t const prim_maxs = ir_prim_max(var->prim_type, true);

    i128_t lhs_minu = prim_minu, lhs_maxu = prim_maxu;
    i128_t rhs_minu = prim_minu, rhs_maxu = prim_maxu;
    i128_t lhs_mins = prim_mins, lhs_maxs = prim_maxs;
    i128_t rhs_mins = prim_mins, rhs_maxs = prim_maxs;
    if (!ir_get_operand_range(insn->operands[0], &lhs_minu, &lhs_maxu, &lhs_mins, &lhs_maxs)
        && !ir_get_operand_range(insn->operands[1], &rhs_minu, &rhs_maxu, &rhs_mins, &rhs_maxs)) {
        var->range_minu = prim_minu;
        var->range_maxu = prim_maxu;
        var->range_mins = prim_mins;
        var->range_maxs = prim_maxs;
        return;
    }

    switch (insn->op2) {
        case IR_OP2_sgts:
        case IR_OP2_sles:
        case IR_OP2_slts:
        case IR_OP2_sges:
        case IR_OP2_sgtu:
        case IR_OP2_sleu:
        case IR_OP2_sltu:
        case IR_OP2_sgeu:
        case IR_OP2_seq:
        case IR_OP2_sne: UNREACHABLE(); // Already covered.

        case IR_OP2_add:
            ir_expand_range(
                var,
                add128u_saturate(lhs_minu, rhs_minu),
                add128u_saturate(lhs_maxu, rhs_maxu),
                add128s_saturate(lhs_mins, rhs_mins),
                add128s_saturate(lhs_maxs, rhs_maxs)
            );
            break;

        case IR_OP2_sub:
            ir_expand_range(
                var,
                sub128u_saturate(lhs_minu, rhs_minu),
                sub128u_saturate(lhs_maxu, rhs_maxu),
                sub128s_saturate(lhs_mins, rhs_mins),
                sub128s_saturate(lhs_maxs, rhs_maxs)
            );
            break;

        case IR_OP2_mul:
            ir_expand_range(
                var,
                mul128u_saturate(lhs_minu, rhs_minu),
                mul128u_saturate(lhs_maxu, rhs_maxu),
                mul128s_saturate(lhs_mins, rhs_mins),
                mul128s_saturate(lhs_maxs, rhs_maxs)
            );
            break;

        case IR_OP2_divs:
        case IR_OP2_divu:
            if (eqzero128(rhs_minu)) {
                // This is UB. Result can be anything.
                var->range_minu = prim_minu;
                var->range_maxu = prim_maxu;
                var->range_mins = prim_mins;
                var->range_maxs = prim_maxs;
            } else {
                ir_expand_range(
                    var,
                    div128u(lhs_minu, rhs_minu),
                    div128u(lhs_maxu, rhs_minu),
                    lhs_mins, // Too expensive to compute properly.
                    lhs_maxs
                );
            }
            break;

        case IR_OP2_rems:
        case IR_OP2_remu:
            if (eqzero128(rhs_minu) || eqzero128(rhs_mins)) {
                // This is UB. Result can be anything.
                var->range_minu = prim_minu;
                var->range_maxu = prim_maxu;
                var->range_mins = prim_mins;
                var->range_maxs = prim_maxs;
            } else {
                // TODO: Check correct range for signed remainder.
                ir_expand_range(var, I128_ZERO, sub128u_saturate(rhs_maxu, ui128(1)), lhs_mins, lhs_maxs);
            }
            break;

        case IR_OP2_shl: {
            int bit = ir_prim_bits(var->prim_type);
            if (
                cmp128u(rhs_maxu, ui128(bit)) || cmp128u(rhs_maxs, ui128(bit)) // Unsigned also checks max<0.
            ) {
                // This is UB. Result can be anything.
                var->range_minu = prim_minu;
                var->range_maxu = prim_maxu;
                var->range_mins = prim_mins;
                var->range_maxs = prim_maxs;
            } else {
                int    shamt = 128 - clz128(max128u(rhs_maxu, rhs_maxs));
                i128_t mult  = shl128(ui128(1), shamt);
                ir_expand_range(
                    var,
                    mul128u_saturate(lhs_minu, mult),
                    mul128u_saturate(lhs_maxu, mult),
                    mul128s_saturate(lhs_mins, mult),
                    mul128s_saturate(lhs_maxs, mult)
                );
            }
        } break;

        case IR_OP2_shru: {
            int    bit       = ir_prim_bits(var->prim_type);
            i128_t max_shamt = max128u(rhs_maxu, rhs_maxs);
            if (
                cmp128u(rhs_minu, ui128(bit)) || cmp128u(rhs_mins, ui128(bit)) // Unsigned also checks max<0.
            ) {
                // This is UB. Result can be anything.
                var->range_minu = prim_minu;
                var->range_maxu = prim_maxu;
                var->range_mins = prim_mins;
                var->range_maxs = prim_maxs;
            } else if (eqzero128(max_shamt)) {
                ir_expand_range(var, lhs_minu, lhs_maxu, lhs_mins, lhs_maxs);
            } else {
                int    shamt    = 128 - clz128(max_shamt);
                i128_t signed_a = shr128u(lhs_mins, shamt);
                i128_t signed_b = shr128u(lhs_maxs, shamt);
                ir_expand_range(
                    var,
                    shr128u(lhs_minu, shamt),
                    shr128u(lhs_maxu, shamt),
                    min128s(signed_a, signed_b), // The sign can flip.
                    max128s(signed_a, signed_b)
                );
            }
        } break;

        case IR_OP2_shrs: {
            int    bit       = ir_prim_bits(var->prim_type);
            i128_t min_shamt = min128u(rhs_maxu, rhs_maxs);
            if (
                cmp128u(rhs_minu, ui128(bit)) || cmp128u(rhs_mins, ui128(bit)) // Unsigned also checks max<0.
            ) {
                // This is UB. Result can be anything.
                var->range_minu = prim_minu;
                var->range_maxu = prim_maxu;
                var->range_mins = prim_mins;
                var->range_maxs = prim_maxs;
            } else if (eqzero128(min_shamt)) {
                ir_expand_range(var, lhs_minu, lhs_maxu, lhs_mins, lhs_maxs);
            } else {
                int    shamt      = 128 - clz128(min_shamt);
                i128_t unsigned_a = shr128s(lhs_minu, shamt);
                i128_t unsigned_b = shr128s(lhs_maxu, shamt);
                ir_expand_range(
                    var,
                    min128s(unsigned_a, unsigned_b), // Sign bit on unsigned number makes it larger.
                    max128s(unsigned_a, unsigned_b),
                    shr128s(lhs_mins, shamt),
                    shr128s(lhs_maxs, shamt)
                );
            }
        } break;

        case IR_OP2_band:
        case IR_OP2_bor:
        case IR_OP2_bxor: {
            // TODO: Could calculate accurately, is it worth doing?
            var->range_minu = prim_minu;
            var->range_maxu = prim_maxu;
            var->range_mins = prim_mins;
            var->range_maxs = prim_maxs;
        } break;

        case IR_N_OP2: UNREACHABLE();
    }
}

// Update the possible range according to an expr1 instruction.
void ir_expr1_range(ir_var_t *var, ir_insn_t const *insn) {
    if (var->prim_type >= IR_PRIM_f32) {
        return;
    }
    switch (insn->op1) {
        case IR_OP1_mov:
        case IR_OP1_signext:
        case IR_OP1_zeroext:
        case IR_OP1_trunc:
            if (insn->operands[0].type == IR_OPERAND_TYPE_VAR && insn->operands[0].var->prim_type < IR_PRIM_f32) {
                ir_expand_range(
                    var,
                    insn->operands[0].var->range_minu,
                    insn->operands[0].var->range_maxu,
                    insn->operands[0].var->range_mins,
                    insn->operands[0].var->range_maxs
                );
            } else {
                var->range_minu = UI128_ZERO;
                var->range_mins = ir_prim_min(var->orig_prim_type, true);
                var->range_maxu = ir_prim_max(var->orig_prim_type, false);
                var->range_maxs = ir_prim_max(var->orig_prim_type, true);
            }
            break;
        case IR_OP1_bneg:
        case IR_OP1_neg:     // Too expensive to compute.
        case IR_OP1_bitcast: // Assume only that it's a legal value.
            var->range_minu = UI128_ZERO;
            var->range_mins = ir_prim_min(var->orig_prim_type, true);
            var->range_maxu = ir_prim_max(var->orig_prim_type, false);
            var->range_maxs = ir_prim_max(var->orig_prim_type, true);
            break;
        case IR_OP1_seqz:
        case IR_OP1_snez: ir_expand_range(var, I128_ZERO, ui128(1), I128_ZERO, ui128(1)); break;
        case IR_N_OP1: UNREACHABLE();
    }
}

// Update the possible range according to an instruction.
void ir_insn_range(ir_var_t *var, ir_insn_t const *insn) {
    switch (insn->type) {
        case IR_INSN_EXPR2: ir_expr2_range(var, insn); break;
        case IR_INSN_EXPR1: ir_expr1_range(var, insn); break;
        case IR_INSN_LEA:
        case IR_INSN_LOAD:
        case IR_INSN_COMBINATOR:
        case IR_INSN_CALL:
        case IR_INSN_MACHINE:
        case IR_INSN_ALLOCA:
            var->range_minu = UI128_ZERO;
            var->range_mins = ir_prim_min(var->orig_prim_type, true);
            var->range_maxu = ir_prim_max(var->orig_prim_type, false);
            var->range_maxs = ir_prim_max(var->orig_prim_type, true);
            break;
        default: break;
    }
}

// Expand the possible range of a variable.
void ir_expand_range(ir_var_t *var, i128_t minu, i128_t maxu, i128_t mins, i128_t maxs) {
    if (var->assigned_at.len == 0) {
        var->range_minu = minu;
        var->range_maxu = maxu;
        var->range_mins = mins;
        var->range_maxs = maxs;
    } else {
        var->range_minu = min128u(var->range_minu, minu);
        var->range_maxu = max128u(var->range_maxu, maxu);
        var->range_mins = min128s(var->range_mins, mins);
        var->range_maxs = max128s(var->range_maxs, maxs);
    }
    i128_t const prim_mins = ir_prim_min(var->prim_type, true);
    i128_t const prim_maxs = ir_prim_max(var->prim_type, true);
    i128_t const prim_maxu = ir_prim_max(var->prim_type, false);

    var->range_maxu = min128u(var->range_maxu, prim_maxu);
    var->range_mins = max128s(var->range_mins, prim_mins);
    var->range_maxs = min128s(var->range_maxs, prim_maxs);
}

// Recompute the possible range of a variable.
void ir_calc_var_range(ir_var_t *var) {
    if (var->prim_type >= IR_PRIM_f32) {
        return;
    }
    i128_t const prim_mins = ir_prim_min(var->orig_prim_type, true);
    i128_t const prim_maxu = ir_prim_max(var->orig_prim_type, false);
    i128_t const prim_maxs = ir_prim_max(var->orig_prim_type, true);
    if (var->assigned_at.len == 0) {
        var->range_minu = UI128_ZERO;
        var->range_mins = prim_mins;
        var->range_maxu = prim_maxu;
        var->range_maxs = prim_maxs;
        return;
    }

    // "Smallest" possible range.
    var->range_minu = prim_maxu;
    var->range_mins = prim_maxs;
    var->range_maxu = UI128_ZERO;
    var->range_maxs = prim_mins;

    set_foreach(ir_insn_t, insn, &var->assigned_at) {
        ir_insn_range(var, insn);
    }
}

static bool calc_ready(ir_var_t const *var, set_t const *work) {
    set_foreach(ir_insn_t, insn, &var->assigned_at) {
        if (insn->type == IR_INSN_EXPR1 || insn->type == IR_INSN_EXPR2) {
            for (size_t i = 0; i < insn->operands_len; i++) {
                if (insn->operands[i].type == IR_OPERAND_TYPE_VAR && set_contains(work, insn->operands[i].var)) {
                    return false;
                }
            }
        }
    }
    return true;
}

// Recompute the possible ranges of all variables in the function.
void ir_calc_all_ranges(ir_func_t *func) {
    set_t work = PTR_SET_EMPTY;
    dlist_foreach_node(ir_var_t, var, &func->vars_list) {
        if (var->prim_type < IR_PRIM_f32) {
            set_add(&work, var);
        }
    }

    while (work.len) {
        set_foreach(ir_var_t, var, &work) {
            if (calc_ready(var, &work)) {
                ir_calc_var_range(var);
                set_remove(&work, var);
                goto again;
            }
        }
    again:;
    }
}



// Count how many bits are needed to represent the value.
int ir_count_bits(ir_const_t iconst, bool allow_s, bool allow_u) {
    if (iconst.prim_type == IR_PRIM_f32 || iconst.prim_type == IR_PRIM_f64) {
        printf("BUG: ir_count_bits with float value\n");
        abort();
    }

    i128_t value = ir_trim_const(iconst, true).const128;
    int    sign  = cmp128s(value, I128_ZERO);
    if (sign == 0) {
        return 0;
    }

    if (sign < 0 && allow_s) {
        value = bneg128(value);
    }
    int bits;
    if (hi64(value)) {
        bits = 128 - __builtin_clzll(hi64(value));
    } else {
        bits = 64 - __builtin_clzll(lo64(value));
    }

    if (sign > 0 && !allow_u) {
        bits++;
    }

    return bits;
}

// Count number of leading zeroes. Interprets all values as 128-bit.
int ir_const_clz(ir_const_t value) {
    if (value.prim_type == IR_PRIM_f32 || value.prim_type == IR_PRIM_f64) {
        printf("BUG: ir_const_clz with float value\n");
        abort();
    }

    if (value.consth) {
        return __builtin_clzll(value.consth);
    } else {
        return 64 + __builtin_clzll(value.constl);
    }
}

// Count number of trailing zeroes.
int ir_const_ctz(ir_const_t value) {
    if (value.prim_type == IR_PRIM_f32 || value.prim_type == IR_PRIM_f64) {
        printf("BUG: ir_const_ctz with float value\n");
        abort();
    }

    if (value.constl) {
        return __builtin_ctzll(value.constl);
    } else {
        return 64 + __builtin_ctzll(value.consth);
    }
}

// Count number of set bits.
int ir_const_popcnt(ir_const_t value) {
    if (value.prim_type == IR_PRIM_f32 || value.prim_type == IR_PRIM_f64) {
        printf("BUG: ir_const_popcnt with float value\n");
        abort();
    }

    // Make it unsigned so the number isn't sign-extended on truncate.
    value = ir_trim_const(value, false);

    // Actually count the bits.
    int count = 0;
    while (value.constl) {
        value.constl &= value.constl - 1;
        count++;
    }
    while (value.consth) {
        value.consth &= value.consth - 1;
        count++;
    }

    return count;
}

// Whether a constant is negative; assumes signed type.
bool ir_const_is_negative(ir_const_t value) {
    switch (value.prim_type) {
        case IR_PRIM_bool: return false;
        case IR_PRIM_i8: return (value.constl >> 7) & 1;
        case IR_PRIM_i16: return (value.constl >> 15) & 1;
        case IR_PRIM_i32: return (value.constl >> 31) & 1;
        case IR_PRIM_i64: return (value.constl >> 63) & 1;
        case IR_PRIM_i128: return (value.consth >> 63) & 1;
        case IR_PRIM_f32: return value.constf32 < 0;
        case IR_PRIM_f64: return value.constf64 < 0;
        case IR_N_PRIM: break;
    }
    UNREACHABLE();
}

// Determines whether two constants are identical. Floats will be compared bitwise.
bool ir_const_identical(ir_const_t lhs, ir_const_t rhs) {
    if (lhs.prim_type != rhs.prim_type) {
        return false;
    } else if (lhs.prim_type == IR_PRIM_f32) {
        return memcmp(&lhs.constf32, &rhs.constf32, sizeof(float)) == 0; // NOLINT
    } else if (lhs.prim_type == IR_PRIM_f64) {
        return memcmp(&lhs.constf64, &rhs.constf64, sizeof(double)) == 0; // NOLINT
    } else {
        return ir_calc2(IR_OP2_seq, lhs, rhs).constl & 1;
    }
}

// Truncate unused bits of a constant.
ir_const_t ir_trim_const(ir_const_t value, bool is_signed) {
    uint8_t bytes;
    switch (value.prim_type) {
        case IR_PRIM_i8: bytes = 1; break;
        case IR_PRIM_i16: bytes = 2; break;
        case IR_PRIM_i32: bytes = 4; break;
        case IR_PRIM_i64:
            if (is_signed) {
                value.consth = -((int64_t)value.constl < 0);
                return value;
            } else {
                value.consth = 0;
                return value;
            }
        case IR_PRIM_i128: return value;
        case IR_PRIM_bool:
            value.constl &= 1;
            value.consth  = 0;
            return value;
        case IR_PRIM_f32: bytes = 4; break;
        case IR_PRIM_f64: bytes = 8; break;
        default: abort();
    }
    value.constl &= (1llu << (8 * bytes % 64)) - 1; // Intentional overflow.
    value.consth  = 0;
    if (!is_signed && (value.constl & (1llu << (8 * bytes - 1)))) {
        value.consth  = -1llu;
        value.constl |= -1llu << (8 * bytes);
    }
    return value;
}

// Arithmetic cast between two types.
ir_const_t ir_cast(ir_prim_t type, ir_const_t value, bool is_signed) {
    if (type == value.prim_type) {
        return value;
    }
    if (type == IR_PRIM_bool) {
        return ir_calc1(IR_OP1_snez, value);
    }
    if (type == IR_PRIM_f32) {
        if (value.prim_type == IR_PRIM_f64) {
            value.constf32 = (float)value.constf64;
        } else {
            value = ir_trim_const(value, is_signed);
#if defined(__SIZEOF_INT128__) && !defined(LILY_SOFT_INT128)
            if (is_signed) {
                value.constf64 = (float)value.const128.val;
            } else {
                value.constf64 = (float)(__int128_t)value.const128.val;
            }
#else
            if (value.prim_type == IR_PRIM_i128) {
                value.constl = value.consth;
            }
            if (value.prim_type <= IR_PRIM_i128 && !is_signed) {
                value.constf32 = value.constl;
            } else {
                value.constf32 = (int64_t)value.constl;
            }
            if (value.prim_type == IR_PRIM_i128) {
                value.constf32 *= 0x1.0p64f;
            }
#endif
            value.prim_type = IR_PRIM_f32;
            return value;
        }
        return value;
    } else if (type == IR_PRIM_f64) {
        if (type == IR_PRIM_f32) {
            value.constf32 = (float)value.constf64;
        } else {
            value = ir_trim_const(value, is_signed);
#if defined(__SIZEOF_INT128__) && !defined(LILY_SOFT_INT128)
            if (value.prim_type <= IR_PRIM_i128 && is_signed) {
                value.constf64 = (double)value.const128.val;
            } else {
                value.constf64 = (double)(__int128_t)value.const128.val;
            }
#else
            if (value.prim_type == IR_PRIM_i128) {
                value.constl = value.consth;
            }
            if (value.prim_type <= IR_PRIM_i128 && !is_signed) {
                value.constf64 = value.constl;
            } else {
                value.constf64 = (int64_t)value.constl;
            }
            if (value.prim_type == IR_PRIM_i128) {
                value.constf64 *= 0x1.0p64;
            }
#endif
            value.prim_type = IR_PRIM_f64;
            return value;
        }
        return value;
    } else {
        value.prim_type = type;
        return ir_trim_const(value, is_signed);
    }
}

// Calculate the result of an expr1.
ir_const_t ir_calc1(ir_op1_type_t oper, ir_const_t value) {
    if (oper == IR_OP1_mov || oper == IR_OP1_signext || oper == IR_OP1_zeroext || oper == IR_OP1_trunc
        || oper == IR_OP1_bitcast) {
        printf("BUG: IR move/cast operations are invalid for ir_calc1\n");
        abort();
    } else if (oper == IR_OP1_snez || oper == IR_OP1_seqz) {
        bool eqz;
        if (value.prim_type == IR_PRIM_f64) {
            eqz = value.constf64 == 0;
        } else if (value.prim_type == IR_PRIM_f32) {
            eqz = value.constf32 == 0;
        } else if (value.prim_type == IR_PRIM_bool) {
            eqz = (value.constl & 1) == 0;
        } else if (value.prim_type == IR_PRIM_i128) {
            eqz = value.consth == 0 && value.constl == 0;
        } else {
            uint8_t bits = 8 << (value.prim_type >> 1);
            eqz          = (value.constl & ((1llu << bits) - 1)) == 0;
        }
        return (ir_const_t){
            .prim_type = IR_PRIM_bool,
            .constl    = eqz ^ (oper == IR_OP1_snez),
            .consth    = 0,
        };
    } else if (oper == IR_OP1_bneg) {
        if (value.prim_type == IR_PRIM_bool) {
            value.constl ^= 1;
        } else if (value.prim_type == IR_PRIM_f32 || value.prim_type == IR_PRIM_f64) {
            fprintf(stderr, "BUG: Cannot do bitwise negation on f32 or f64\n");
            abort();
        } else {
            value.constl ^= -1;
            value.consth ^= -1;
        }
        return value;
    } else if (oper == IR_OP1_neg) {
        if (value.prim_type == IR_PRIM_bool) {
            fprintf(stderr, "BUG: Cannot do arithmetic negation on bool\n");
            abort();
        } else if (value.prim_type == IR_PRIM_f64) {
            value.constf64 = -value.constf64;
        } else if (value.prim_type == IR_PRIM_f32) {
            value.constf32 = -value.constf32;
        } else {
            value.constl ^= -1;
            value.consth ^= -1;
            value.constl++;
            if (!value.constl) {
                value.consth++;
            }
        }
        return value;
    } else {
        fprintf(stderr, "BUG: Invalid op1 type\n");
        abort();
    }
}

// Calculate the result of an expr2.
ir_const_t ir_calc2(ir_op2_type_t oper, ir_const_t lhs, ir_const_t rhs) {
    ir_const_t out = {.prim_type = lhs.prim_type, .constl = 0, .consth = 0};
    if (oper >= IR_OP2_sgts && oper <= IR_OP2_sne) {
        out.prim_type = IR_PRIM_bool;
    } else {
        out.prim_type = lhs.prim_type;
    }
    if (lhs.prim_type == IR_PRIM_f32) {
        switch (oper) {
            case IR_OP2_sgts: out.constl = lhs.constf32 > rhs.constf32; break;
            case IR_OP2_sles: out.constl = lhs.constf32 <= rhs.constf32; break;
            case IR_OP2_slts: out.constl = lhs.constf32 < rhs.constf32; break;
            case IR_OP2_sges: out.constl = lhs.constf32 >= rhs.constf32; break;
            case IR_OP2_seq: out.constl = lhs.constf32 == rhs.constf32; break;
            case IR_OP2_sne: out.constl = lhs.constf32 != rhs.constf32; break;
            case IR_OP2_add: out.constf32 = lhs.constf32 + rhs.constf32; break;
            case IR_OP2_sub: out.constf32 = lhs.constf32 - rhs.constf32; break;
            case IR_OP2_mul: out.constf32 = lhs.constf32 * rhs.constf32; break;
            case IR_OP2_divs: out.constf32 = lhs.constf32 / rhs.constf32; break;
            default:
                fprintf(stderr, "BUG: Invalid op2 type for f32: %s\n", ir_op2_names[oper]);
                abort();
                break;
        }
        return out;
    } else if (lhs.prim_type == IR_PRIM_f64) {
        switch (oper) {
            case IR_OP2_sgts: out.constl = lhs.constf64 > rhs.constf64; break;
            case IR_OP2_sles: out.constl = lhs.constf64 <= rhs.constf64; break;
            case IR_OP2_slts: out.constl = lhs.constf64 < rhs.constf64; break;
            case IR_OP2_sges: out.constl = lhs.constf64 >= rhs.constf64; break;
            case IR_OP2_seq: out.constl = lhs.constf64 == rhs.constf64; break;
            case IR_OP2_sne: out.constl = lhs.constf64 != rhs.constf64; break;
            case IR_OP2_add: out.constf64 = lhs.constf64 + rhs.constf64; break;
            case IR_OP2_sub: out.constf64 = lhs.constf64 - rhs.constf64; break;
            case IR_OP2_mul: out.constf64 = lhs.constf64 * rhs.constf64; break;
            case IR_OP2_divs: out.constf64 = lhs.constf64 / rhs.constf64; break;
            default:
                fprintf(stderr, "BUG: Invalid op2 type for f64: %s\n", ir_op2_names[oper]);
                abort();
                break;
        }
        return out;
    } else if (lhs.prim_type == IR_PRIM_bool) {
        lhs.constl &= 1;
        rhs.constl &= 1;
        switch (oper) {
            case IR_OP2_seq: out.constl = 1 & (1 ^ lhs.constl ^ rhs.constl); break;
            case IR_OP2_sne: out.constl = 1 & (lhs.constl ^ rhs.constl); break;
            case IR_OP2_band: out.constl = 1 & (lhs.constl & rhs.constl); break;
            case IR_OP2_bor: out.constl = 1 & (lhs.constl | rhs.constl); break;
            case IR_OP2_bxor: out.constl = 1 & (lhs.constl ^ rhs.constl); break;
            default:
                fprintf(stderr, "BUG: Invalid op2 type for bool: %s\n", ir_op2_names[oper]);
                abort();
                break;
        }
        return out;
    } else if (lhs.prim_type == IR_PRIM_i128) {
        switch (oper) {
            case IR_OP2_sgts: out.constl = cmp128s(lhs.const128, rhs.const128) > 0; break;
            case IR_OP2_sles: out.constl = cmp128s(lhs.const128, rhs.const128) <= 0; break;
            case IR_OP2_slts: out.constl = cmp128s(lhs.const128, rhs.const128) < 0; break;
            case IR_OP2_sges: out.constl = cmp128s(lhs.const128, rhs.const128) >= 0; break;
            case IR_OP2_sgtu: out.constl = cmp128u(lhs.const128, rhs.const128) > 0; break;
            case IR_OP2_sleu: out.constl = cmp128u(lhs.const128, rhs.const128) <= 0; break;
            case IR_OP2_sltu: out.constl = cmp128u(lhs.const128, rhs.const128) < 0; break;
            case IR_OP2_sgeu: out.constl = cmp128u(lhs.const128, rhs.const128) >= 0; break;
            case IR_OP2_seq: out.constl = cmp128s(lhs.const128, rhs.const128) == 0; break;
            case IR_OP2_sne: out.constl = cmp128s(lhs.const128, rhs.const128) != 0; break;
            case IR_OP2_add: out.const128 = add128(lhs.const128, rhs.const128); break;
            case IR_OP2_sub: out.const128 = add128(lhs.const128, neg128(rhs.const128)); break;
            case IR_OP2_mul: out.const128 = mul128(lhs.const128, rhs.const128); break;
            case IR_OP2_divu: out.const128 = div128u(lhs.const128, rhs.const128); break;
            case IR_OP2_remu: out.const128 = rem128u(lhs.const128, rhs.const128); break;
            case IR_OP2_divs: out.const128 = div128s(lhs.const128, rhs.const128); break;
            case IR_OP2_rems: out.const128 = rem128s(lhs.const128, rhs.const128); break;
            case IR_OP2_shl: out.const128 = shl128(lhs.const128, (uint8_t)rhs.constl); break;
            case IR_OP2_shru: out.const128 = shr128u(lhs.const128, (uint8_t)rhs.constl); break;
            case IR_OP2_shrs: out.const128 = shr128s(lhs.const128, (uint8_t)rhs.constl); break;
            case IR_OP2_band: out.const128 = and128(lhs.const128, rhs.const128); break;
            case IR_OP2_bor: out.const128 = or128(lhs.const128, rhs.const128); break;
            case IR_OP2_bxor: out.const128 = xor128(lhs.const128, rhs.const128); break;
            case IR_N_OP2: UNREACHABLE();
        }
        return out;
    } else {
        uint64_t lhs_u = ir_trim_const(lhs, false).constl;
        uint64_t rhs_u = ir_trim_const(rhs, false).constl;
        int64_t  lhs_s = (int64_t)lhs_u;
        int64_t  rhs_s = (int64_t)rhs_u;
        switch (oper) {
            case IR_OP2_sgts: out.constl = lhs_s > rhs_s; break;
            case IR_OP2_sles: out.constl = lhs_s <= rhs_s; break;
            case IR_OP2_slts: out.constl = lhs_s < rhs_s; break;
            case IR_OP2_sges: out.constl = lhs_s >= rhs_s; break;
            case IR_OP2_sgtu: out.constl = lhs_u > rhs_u; break;
            case IR_OP2_sleu: out.constl = lhs_u <= rhs_u; break;
            case IR_OP2_sltu: out.constl = lhs_u < rhs_u; break;
            case IR_OP2_sgeu: out.constl = lhs_u >= rhs_u; break;
            case IR_OP2_seq: out.constl = lhs_s == rhs_s; break;
            case IR_OP2_sne: out.constl = lhs_s != rhs_s; break;
            case IR_OP2_add: out.constl = lhs_s + rhs_s; break;
            case IR_OP2_sub: out.constl = lhs_s - rhs_s; break;
            case IR_OP2_mul: out.constl = lhs_s * rhs_s; break;
            case IR_OP2_divu: out.constl = lhs_u / rhs_u; break;
            case IR_OP2_remu: out.constl = lhs_u % rhs_u; break;
            case IR_OP2_divs: out.constl = lhs_s / rhs_s; break;
            case IR_OP2_rems: out.constl = lhs_s % rhs_s; break;
            case IR_OP2_shl: out.constl = lhs_s << rhs_s; break;
            case IR_OP2_shru: out.constl = lhs_u >> rhs_u; break;
            case IR_OP2_shrs: out.constl = lhs_s >> rhs_s; break;
            case IR_OP2_band: out.constl = lhs_s & rhs_s; break;
            case IR_OP2_bor: out.constl = lhs_s | rhs_s; break;
            case IR_OP2_bxor: out.constl = lhs_s ^ rhs_s; break;
            case IR_N_OP2: UNREACHABLE();
        }
        return out;
    }
    abort();
}



// Test whether to `ir_memref_t` are identical.
static bool ir_memref_identical(ir_memref_t lhs, ir_memref_t rhs) {
    if (lhs.base_type != rhs.base_type || lhs.offset != rhs.offset) {
        return false;
    }
    switch (lhs.base_type) {
        case IR_MEMBASE_ABS: return true;
        case IR_MEMBASE_SYM: return !strcmp(lhs.base_sym, rhs.base_sym);
        case IR_MEMBASE_FRAME: return lhs.base_frame == rhs.base_frame;
        case IR_MEMBASE_CODE: return lhs.base_code == rhs.base_code;
        case IR_MEMBASE_VAR: return lhs.base_var == rhs.base_var;
        case IR_MEMBASE_REG: return lhs.base_regno == rhs.base_regno;
    }
    abort();
}

// Determine whether two operands are either the same variable or identical.
// Floats will be compared bitwise.
bool ir_operand_identical(ir_operand_t lhs, ir_operand_t rhs) {
    if (lhs.type != rhs.type) {
        return false;
    }
    switch (lhs.type) {
        case IR_OPERAND_TYPE_VAR: return lhs.var == rhs.var;
        case IR_OPERAND_TYPE_UNDEF: return false;
        case IR_OPERAND_TYPE_MEM: return ir_memref_identical(lhs.mem, rhs.mem);
        default: return ir_const_identical(lhs.iconst, rhs.iconst);
    }
}
