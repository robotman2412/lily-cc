
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir/ir_interpreter.h"

#include "arith128.h"
#include "ir_types.h"
#include "list.h"
#include "set.h"
#include "unreachable.h"

#include <stdlib.h>
#include <string.h>



// Get the range of possible values from an IR operand.
bool ir_get_operand_range(ir_operand_t operand, i128_t *min_out, i128_t *max_out) {
    switch (operand.type) {
        case IR_OPERAND_TYPE_CONST:
            if (operand.iconst.prim_type >= IR_PRIM_f32) {
                return false;
            }
            *min_out = operand.iconst.const128;
            *max_out = operand.iconst.const128;
            return true;
        case IR_OPERAND_TYPE_VAR:
            if (operand.var->prim_type >= IR_PRIM_f32) {
                return false;
            }
            *min_out = operand.var->range_min;
            *max_out = operand.var->range_max;
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
        case IR_OP2_sgt:
        case IR_OP2_sle:
        case IR_OP2_slt:
        case IR_OP2_sge:
        case IR_OP2_seq:
        case IR_OP2_sne: ir_expand_range(var, I128_ZERO, ui128(1)); return;
        default: break;
    }

    i128_t const prim_min = ir_prim_min(var->prim_type);
    i128_t const prim_max = ir_prim_max(var->prim_type);

    i128_t lhs_min = prim_min, lhs_max = prim_max;
    i128_t rhs_min = prim_min, rhs_max = prim_max;
    if (!ir_get_operand_range(insn->operands[0], &lhs_min, &lhs_max)
        && !ir_get_operand_range(insn->operands[1], &rhs_min, &rhs_max)) {
        var->range_min = prim_min;
        var->range_max = prim_max;
        return;
    }

    switch (insn->op2) {
        case IR_OP2_sgt:
        case IR_OP2_sle:
        case IR_OP2_slt:
        case IR_OP2_sge:
        case IR_OP2_seq:
        case IR_OP2_sne: UNREACHABLE(); // Already covered.

        case IR_OP2_add:
            if (ir_prim_is_signed(var->prim_type)) {
                ir_expand_range(var, add128s_saturate(lhs_min, rhs_min), add128s_saturate(lhs_max, rhs_max));
            } else {
                ir_expand_range(var, add128u_saturate(lhs_min, rhs_min), add128u_saturate(lhs_max, rhs_max));
            }
            break;

        case IR_OP2_sub:
            if (ir_prim_is_signed(var->prim_type)) {
                ir_expand_range(var, sub128s_saturate(lhs_min, rhs_min), sub128s_saturate(lhs_max, rhs_max));
            } else {
                ir_expand_range(var, sub128u_saturate(lhs_min, rhs_min), sub128u_saturate(lhs_max, rhs_max));
            }
            break;

        case IR_OP2_mul:
            if (ir_prim_is_signed(var->prim_type)) {
                ir_expand_range(var, mul128s_saturate(lhs_min, rhs_min), mul128s_saturate(lhs_max, rhs_max));
            } else {
                ir_expand_range(var, mul128u_saturate(lhs_min, rhs_min), mul128u_saturate(lhs_max, rhs_max));
            }
            break;

        case IR_OP2_div:
            if (ir_prim_is_signed(var->prim_type)) {
                if (cmp128s(rhs_min, I128_ZERO) == 0) {
                    ir_expand_range(var, lhs_min, lhs_max);
                } else {
                    ir_expand_range(var, div128s(lhs_min, rhs_min), div128s(lhs_max, rhs_min));
                }
            } else {
                if (cmp128u(rhs_min, I128_ZERO) == 0) {
                    ir_expand_range(var, lhs_min, lhs_max);
                } else {
                    ir_expand_range(var, div128u(lhs_min, rhs_min), div128u(lhs_max, rhs_min));
                }
            }
            break;

        case IR_OP2_rem:
            if (ir_prim_is_signed(var->prim_type)) {
                i128_t max = sub128u_saturate(max128u(abs128s(rhs_min), abs128s(rhs_max)), ui128(1));
                ir_expand_range(var, neg128(max), max);
            } else {
                ir_expand_range(var, I128_ZERO, sub128u_saturate(rhs_max, ui128(1)));
            }
            break;

        case IR_OP2_shl:
            // TODO: Could calculate accurately, probably not worth doing.
            var->range_min = prim_min;
            var->range_max = prim_max;
            break;

        case IR_OP2_shr: {
            int shamt = ir_prim_bits(var->prim_type) - 1;
            if (cmp128u(rhs_min, ui128(shamt)) < 0) {
                shamt = (int)lo64(rhs_min);
            }

            if (ir_prim_is_signed(var->prim_type)) {
                ir_expand_range(var, shr128s(lhs_min, shamt), shr128s(lhs_max, shamt));
            } else {
                ir_expand_range(var, shr128u(lhs_min, shamt), shr128u(lhs_max, shamt));
            }
        } break;

        case IR_OP2_band: {
            // Again, could do better for non-constant, but probably not worth doing.
            i128_t lhs_mask = lhs_max, rhs_mask = rhs_max;
            if (cmp128u(lhs_min, lhs_max) != 0) {
                lhs_mask = or128(prim_min, prim_max);
            }
            if (cmp128u(rhs_min, rhs_max) != 0) {
                rhs_mask = or128(prim_min, prim_max);
            }

            i128_t mask = and128(lhs_mask, rhs_mask);
            i128_t a    = and128(prim_min, mask);
            i128_t b    = and128(prim_min, mask);
            ir_expand_range(var, a, b);
            ir_expand_range(var, b, a);
        } break;

        case IR_OP2_bor:
        case IR_OP2_bxor:
            ir_expand_range(var, or128(lhs_min, rhs_min), or128(lhs_max, rhs_max));
            ir_expand_range(var, lhs_min, lhs_max);
            ir_expand_range(var, rhs_min, rhs_max);
            break;

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
            if (insn->operands[0].type == IR_OPERAND_TYPE_VAR && insn->operands[0].var->prim_type < IR_PRIM_f32) {
                ir_expand_range(var, insn->operands[0].var->range_min, insn->operands[0].var->range_max);
            } else {
                var->range_min = ir_prim_min(var->orig_prim_type);
                var->range_max = ir_prim_max(var->orig_prim_type);
            }
            break;
        case IR_OP1_bneg:
        case IR_OP1_bitcast:
            var->range_min = ir_prim_min(var->orig_prim_type);
            var->range_max = ir_prim_max(var->orig_prim_type);
            break;
        case IR_OP1_seqz:
        case IR_OP1_snez: ir_expand_range(var, I128_ZERO, ui128(1)); break;
        case IR_OP1_neg: ir_expand_range(var, var->range_max, var->range_min); break;
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
            var->range_min = ir_prim_min(var->orig_prim_type);
            var->range_max = ir_prim_max(var->orig_prim_type);
            break;
        default: break;
    }
}

// Expand the possible range of a variable.
void ir_expand_range(ir_var_t *var, i128_t min, i128_t max) {
    if (var->assigned_at.len == 0) {
        var->range_min = min;
        var->range_max = max;
    } else if (ir_prim_is_signed(var->prim_type)) {
        var->range_min = min128s(var->range_min, min);
        var->range_max = max128s(var->range_max, max);
    } else {
        var->range_min = min128u(var->range_min, min);
        var->range_max = max128u(var->range_max, max);
    }
    i128_t const prim_min = ir_prim_min(var->prim_type);
    i128_t const prim_max = ir_prim_max(var->prim_type);
    if (ir_prim_is_signed(var->prim_type)) {
        if (cmp128s(var->range_min, prim_min) < 0) {
            var->range_min = prim_min;
        }
        if (cmp128s(var->range_max, prim_max) > 0) {
            var->range_max = prim_max;
        }
    } else {
        if (cmp128u(var->range_min, prim_min) < 0) {
            var->range_min = prim_min;
        }
        if (cmp128u(var->range_max, prim_max) > 0) {
            var->range_max = prim_max;
        }
    }
}

// Recompute the possible range of a variable.
void ir_calc_var_range(ir_var_t *var) {
    if (var->prim_type >= IR_PRIM_f32) {
        return;
    }
    i128_t const prim_min = ir_prim_min(var->prim_type);
    i128_t const prim_max = ir_prim_max(var->prim_type);
    if (var->assigned_at.len == 0) {
        var->range_min = prim_min;
        var->range_max = prim_max;
        return;
    }

    var->range_min = prim_max;
    var->range_max = prim_min;

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

    iconst.prim_type = ir_prim_as_signed(iconst.prim_type);
    i128_t value     = ir_trim_const(iconst).const128;
    int    sign      = cmp128s(value, I128_ZERO);
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
    if (value.prim_type <= IR_PRIM_u128) {
        value.prim_type |= 1;
    }
    value = ir_trim_const(value);

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

// Whether a constant is negative.
bool ir_const_is_negative(ir_const_t value) {
    switch (value.prim_type) {
        case IR_PRIM_u8:
        case IR_PRIM_u16:
        case IR_PRIM_u32:
        case IR_PRIM_u64:
        case IR_PRIM_u128:
        case IR_PRIM_bool: return false;
        case IR_PRIM_s8: return (value.constl >> 7) & 1;
        case IR_PRIM_s16: return (value.constl >> 15) & 1;
        case IR_PRIM_s32: return (value.constl >> 31) & 1;
        case IR_PRIM_s64: return (value.constl >> 63) & 1;
        case IR_PRIM_s128: return (value.consth >> 63) & 1;
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
    } else {
        return ir_calc2(IR_OP2_seq, lhs, rhs).constl & 1;
    }
}

// Determines whether two constants are effectively identical after casting.
// Floats are promoted to f64, then compared bitwise.
bool ir_const_lenient_identical(ir_const_t lhs, ir_const_t rhs) {
    if (lhs.prim_type == IR_PRIM_f32) {
        lhs.constf64  = lhs.constf32;
        lhs.prim_type = IR_PRIM_f64;
    }
    if (rhs.prim_type == IR_PRIM_f32) {
        rhs.constf64  = rhs.constf32;
        rhs.prim_type = IR_PRIM_f64;
    }
    if ((lhs.prim_type == IR_PRIM_f32) != (rhs.prim_type == IR_PRIM_f64)) {
        return false;
    } else if (lhs.prim_type == IR_PRIM_f64) {
        return lhs.constl == rhs.constl;
    }
    lhs = ir_trim_const(lhs);
    rhs = ir_trim_const(rhs);
    return lhs.consth == rhs.consth && lhs.constl == rhs.constl;
}

// Truncate unused bits of a constant.
ir_const_t ir_trim_const(ir_const_t value) {
    uint8_t bytes;
    switch (value.prim_type) {
        case IR_PRIM_s8:
        case IR_PRIM_u8: bytes = 1; break;
        case IR_PRIM_s16:
        case IR_PRIM_u16: bytes = 2; break;
        case IR_PRIM_s32:
        case IR_PRIM_u32: bytes = 4; break;
        case IR_PRIM_s64: value.consth = -((int64_t)value.constl < 0); return value;
        case IR_PRIM_u64: value.consth = 0; return value;
        case IR_PRIM_s128:
        case IR_PRIM_u128: return value;
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
    if (!(value.prim_type & 1) && (value.constl & (1llu << (8 * bytes - 1)))) {
        value.consth  = -1llu;
        value.constl |= -1llu << (8 * bytes);
    }
    return value;
}

// Cast from one type to another with IR rules.
ir_const_t ir_cast(ir_prim_t type, ir_const_t value) {
    if (type == value.prim_type) {
        return value;
    }
    if (type == IR_PRIM_bool) {
        return ir_calc1(IR_OP1_snez, value);
    }
    if (type == IR_PRIM_f32) {
        if (type == IR_PRIM_f64) {
            value.constf64 = value.constf32;
        } else {
            value = ir_trim_const(value);
#if defined(__SIZEOF_INT128__) && !defined(LILY_SOFT_INT128)
            if (value.prim_type <= IR_PRIM_u128 && (value.prim_type & 1)) {
                value.constf64 = (double)value.const128.val;
            } else {
                value.constf64 = (double)(__int128_t)value.const128.val;
            }
#else
            if (value.prim_type == IR_PRIM_s128 || value.prim_type == IR_PRIM_u128) {
                value.constl = value.consth;
            }
            if (value.prim_type <= IR_PRIM_u128 && (value.prim_type & 1)) {
                value.constf32 = value.constl;
            } else {
                value.constf32 = (int64_t)value.constl;
            }
            if (value.prim_type == IR_PRIM_s128 || value.prim_type == IR_PRIM_u128) {
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
            value = ir_trim_const(value);
#if defined(__SIZEOF_INT128__) && !defined(LILY_SOFT_INT128)
            if (value.prim_type <= IR_PRIM_u128 && (value.prim_type & 1)) {
                value.constf64 = (double)value.const128.val;
            } else {
                value.constf64 = (double)(__int128_t)value.const128.val;
            }
#else
            if (value.prim_type == IR_PRIM_s128 || value.prim_type == IR_PRIM_u128) {
                value.constl = value.consth;
            }
            if (value.prim_type <= IR_PRIM_u128 && (value.prim_type & 1)) {
                value.constf64 = value.constl;
            } else {
                value.constf64 = (int64_t)value.constl;
            }
            if (value.prim_type == IR_PRIM_s128 || value.prim_type == IR_PRIM_u128) {
                value.constf64 *= 0x1.0p64;
            }
#endif
            value.prim_type = IR_PRIM_f64;
            return value;
        }
        return value;
    } else {
        value.prim_type = type;
        return ir_trim_const(value);
    }
}

// Calculate the result of an expr1.
ir_const_t ir_calc1(ir_op1_type_t oper, ir_const_t value) {
    if (oper == IR_OP1_mov || oper == IR_OP1_bitcast) {
        printf("BUG: ir_calc1 on IR_OP1_mov or IR_OP1_bitcast is invalid\n");
        abort();
    } else if (oper == IR_OP1_snez || oper == IR_OP1_seqz) {
        bool eqz;
        if (value.prim_type == IR_PRIM_f64) {
            eqz = value.constf64 == 0;
        } else if (value.prim_type == IR_PRIM_f32) {
            eqz = value.constf32 == 0;
        } else if (value.prim_type == IR_PRIM_bool) {
            eqz = (value.constl & 1) == 0;
        } else if (value.prim_type == IR_PRIM_s128 || value.prim_type == IR_PRIM_u128) {
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
    if (oper >= IR_OP2_sgt && oper <= IR_OP2_sne) {
        out.prim_type = IR_PRIM_bool;
    } else {
        out.prim_type = lhs.prim_type;
    }
    if (lhs.prim_type == IR_PRIM_f32) {
        switch (oper) {
            case IR_OP2_sgt: out.constl = lhs.constf32 > rhs.constf32; break;
            case IR_OP2_sle: out.constl = lhs.constf32 <= rhs.constf32; break;
            case IR_OP2_slt: out.constl = lhs.constf32 < rhs.constf32; break;
            case IR_OP2_sge: out.constl = lhs.constf32 >= rhs.constf32; break;
            case IR_OP2_seq: out.constl = lhs.constf32 == rhs.constf32; break;
            case IR_OP2_sne: out.constl = lhs.constf32 != rhs.constf32; break;
            case IR_OP2_add: out.constf32 = lhs.constf32 + rhs.constf32; break;
            case IR_OP2_sub: out.constf32 = lhs.constf32 - rhs.constf32; break;
            case IR_OP2_mul: out.constf32 = lhs.constf32 * rhs.constf32; break;
            case IR_OP2_div: out.constf32 = lhs.constf32 / rhs.constf32; break;
            default:
                fprintf(stderr, "BUG: Invalid op2 type for f32: %s\n", ir_op2_names[oper]);
                abort();
                break;
        }
        return out;
    } else if (lhs.prim_type == IR_PRIM_f64) {
        switch (oper) {
            case IR_OP2_sgt: out.constl = lhs.constf64 > rhs.constf64; break;
            case IR_OP2_sle: out.constl = lhs.constf64 <= rhs.constf64; break;
            case IR_OP2_slt: out.constl = lhs.constf64 < rhs.constf64; break;
            case IR_OP2_sge: out.constl = lhs.constf64 >= rhs.constf64; break;
            case IR_OP2_seq: out.constl = lhs.constf64 == rhs.constf64; break;
            case IR_OP2_sne: out.constl = lhs.constf64 != rhs.constf64; break;
            case IR_OP2_add: out.constf64 = lhs.constf64 + rhs.constf64; break;
            case IR_OP2_sub: out.constf64 = lhs.constf64 - rhs.constf64; break;
            case IR_OP2_mul: out.constf64 = lhs.constf64 * rhs.constf64; break;
            case IR_OP2_div: out.constf64 = lhs.constf64 / rhs.constf64; break;
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
    } else if (lhs.prim_type == IR_PRIM_u128) {
        switch (oper) {
            case IR_OP2_sgt: out.constl = cmp128u(lhs.const128, rhs.const128) > 0; break;
            case IR_OP2_sle: out.constl = cmp128u(lhs.const128, rhs.const128) <= 0; break;
            case IR_OP2_slt: out.constl = cmp128u(lhs.const128, rhs.const128) < 0; break;
            case IR_OP2_sge: out.constl = cmp128u(lhs.const128, rhs.const128) >= 0; break;
            case IR_OP2_seq: out.constl = cmp128u(lhs.const128, rhs.const128) == 0; break;
            case IR_OP2_sne: out.constl = cmp128u(lhs.const128, rhs.const128) != 0; break;
            case IR_OP2_add: out.const128 = add128(lhs.const128, rhs.const128); break;
            case IR_OP2_sub: out.const128 = add128(lhs.const128, neg128(rhs.const128)); break;
            case IR_OP2_mul: out.const128 = mul128(lhs.const128, rhs.const128); break;
            case IR_OP2_div: out.const128 = div128u(lhs.const128, rhs.const128); break;
            case IR_OP2_rem: out.const128 = rem128u(lhs.const128, rhs.const128); break;
            case IR_OP2_shl: out.const128 = shl128(lhs.const128, (uint8_t)rhs.constl); break;
            case IR_OP2_shr: out.const128 = shr128u(lhs.const128, (uint8_t)rhs.constl); break;
            case IR_OP2_band: out.const128 = and128(lhs.const128, rhs.const128); break;
            case IR_OP2_bor: out.const128 = or128(lhs.const128, rhs.const128); break;
            case IR_OP2_bxor: out.const128 = xor128(lhs.const128, rhs.const128); break;
            default: abort();
        }
        return out;
    } else if (lhs.prim_type == IR_PRIM_s128) {
        switch (oper) {
            case IR_OP2_sgt: out.constl = cmp128s(lhs.const128, rhs.const128) > 0; break;
            case IR_OP2_sle: out.constl = cmp128s(lhs.const128, rhs.const128) <= 0; break;
            case IR_OP2_slt: out.constl = cmp128s(lhs.const128, rhs.const128) < 0; break;
            case IR_OP2_sge: out.constl = cmp128s(lhs.const128, rhs.const128) >= 0; break;
            case IR_OP2_seq: out.constl = cmp128s(lhs.const128, rhs.const128) == 0; break;
            case IR_OP2_sne: out.constl = cmp128s(lhs.const128, rhs.const128) != 0; break;
            case IR_OP2_add: out.const128 = add128(lhs.const128, rhs.const128); break;
            case IR_OP2_sub: out.const128 = add128(lhs.const128, neg128(rhs.const128)); break;
            case IR_OP2_mul: out.const128 = mul128(lhs.const128, rhs.const128); break;
            case IR_OP2_div: out.const128 = div128s(lhs.const128, rhs.const128); break;
            case IR_OP2_rem: out.const128 = rem128s(lhs.const128, rhs.const128); break;
            case IR_OP2_shl: out.const128 = shl128(lhs.const128, (uint8_t)rhs.constl); break;
            case IR_OP2_shr: out.const128 = shr128s(lhs.const128, (uint8_t)rhs.constl); break;
            case IR_OP2_band: out.const128 = and128(lhs.const128, rhs.const128); break;
            case IR_OP2_bor: out.const128 = or128(lhs.const128, rhs.const128); break;
            case IR_OP2_bxor: out.const128 = xor128(lhs.const128, rhs.const128); break;
            default: abort();
        }
        return out;
    } else if (lhs.prim_type & 1) {
        lhs = ir_trim_const(lhs);
        rhs = ir_trim_const(rhs);
        switch (oper) {
            case IR_OP2_sgt: out.constl = lhs.constl > rhs.constl; break;
            case IR_OP2_sle: out.constl = lhs.constl <= rhs.constl; break;
            case IR_OP2_slt: out.constl = lhs.constl < rhs.constl; break;
            case IR_OP2_sge: out.constl = lhs.constl >= rhs.constl; break;
            case IR_OP2_seq: out.constl = lhs.constl == rhs.constl; break;
            case IR_OP2_sne: out.constl = lhs.constl != rhs.constl; break;
            case IR_OP2_add: out.constl = lhs.constl + rhs.constl; break;
            case IR_OP2_sub: out.constl = lhs.constl - rhs.constl; break;
            case IR_OP2_mul: out.constl = lhs.constl * rhs.constl; break;
            case IR_OP2_div: out.constl = lhs.constl / rhs.constl; break;
            case IR_OP2_rem: out.constl = lhs.constl % rhs.constl; break;
            case IR_OP2_shl: out.constl = lhs.constl << rhs.constl; break;
            case IR_OP2_shr: out.constl = lhs.constl >> rhs.constl; break;
            case IR_OP2_band: out.constl = lhs.constl & rhs.constl; break;
            case IR_OP2_bor: out.constl = lhs.constl | rhs.constl; break;
            case IR_OP2_bxor: out.constl = lhs.constl ^ rhs.constl; break;
            default: abort();
        }
        return out;
    } else {
        lhs = ir_trim_const(lhs);
        rhs = ir_trim_const(rhs);
        switch (oper) {
            case IR_OP2_sgt: out.constl = (int64_t)lhs.constl > (int64_t)rhs.constl; break;
            case IR_OP2_sle: out.constl = (int64_t)lhs.constl <= (int64_t)rhs.constl; break;
            case IR_OP2_slt: out.constl = (int64_t)lhs.constl < (int64_t)rhs.constl; break;
            case IR_OP2_sge: out.constl = (int64_t)lhs.constl >= (int64_t)rhs.constl; break;
            case IR_OP2_seq: out.constl = (int64_t)lhs.constl == (int64_t)rhs.constl; break;
            case IR_OP2_sne: out.constl = (int64_t)lhs.constl != (int64_t)rhs.constl; break;
            case IR_OP2_add: out.constl = (int64_t)lhs.constl + (int64_t)rhs.constl; break;
            case IR_OP2_sub: out.constl = (int64_t)lhs.constl - (int64_t)rhs.constl; break;
            case IR_OP2_mul: out.constl = (int64_t)lhs.constl * (int64_t)rhs.constl; break;
            case IR_OP2_div: out.constl = (int64_t)lhs.constl / (int64_t)rhs.constl; break;
            case IR_OP2_rem: out.constl = (int64_t)lhs.constl % (int64_t)rhs.constl; break;
            case IR_OP2_shl: out.constl = (int64_t)lhs.constl << (int64_t)rhs.constl; break;
            case IR_OP2_shr: out.constl = (int64_t)lhs.constl >> (int64_t)rhs.constl; break;
            case IR_OP2_band: out.constl = (int64_t)lhs.constl & (int64_t)rhs.constl; break;
            case IR_OP2_bor: out.constl = (int64_t)lhs.constl | (int64_t)rhs.constl; break;
            case IR_OP2_bxor: out.constl = (int64_t)lhs.constl ^ (int64_t)rhs.constl; break;
            default: abort();
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

// Determines whether two operands are either the same variable or effectively identical after casting.
// Floats are promoted to f64, then compared bitwise.
bool ir_operand_lenient_identical(ir_operand_t lhs, ir_operand_t rhs) {
    if (lhs.type != rhs.type) {
        return false;
    }
    switch (lhs.type) {
        case IR_OPERAND_TYPE_VAR: return lhs.var == rhs.var;
        case IR_OPERAND_TYPE_UNDEF: return false;
        case IR_OPERAND_TYPE_MEM: return ir_memref_identical(lhs.mem, rhs.mem);
        default: return ir_const_lenient_identical(lhs.iconst, rhs.iconst);
    }
}
