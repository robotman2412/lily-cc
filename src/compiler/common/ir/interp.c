
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "interp.h"

#include <stdlib.h>



// Truncate unused bits of a constant.
ir_const_t ir_trim_const(ir_const_t value) {
    uint8_t bytes;
    switch (value.prim_type) {
        case IR_PRIM_S8: bytes = 1; break;
        case IR_PRIM_U8: bytes = 1; break;
        case IR_PRIM_S16: bytes = 2; break;
        case IR_PRIM_U16: bytes = 2; break;
        case IR_PRIM_S32: bytes = 4; break;
        case IR_PRIM_U32: bytes = 4; break;
        case IR_PRIM_S64: bytes = 8; break;
        case IR_PRIM_U64: bytes = 8; break;
        case IR_PRIM_S128: return value;
        case IR_PRIM_U128: return value;
        case IR_PRIM_BOOL:
            value.constl = value.constl || value.consth;
            value.consth = 0;
            return value;
        case IR_PRIM_F32: bytes = 4; break;
        case IR_PRIM_F64: bytes = 8; break;
        default: abort();
    }
    value.constl &= (1llu << (8 * bytes)) - 1;
    value.consth  = 0;
    if (value.prim_type <= IR_PRIM_S64) {
        if (!(value.prim_type & 1) && (value.constl & (1 << (8 * bytes - 1)))) {
            value.consth  = -1llu;
            value.constl |= -1llu << (8 * bytes);
        }
    }
    return value;
}

// Cast from one type to another with IR rules.
ir_const_t ir_cast(ir_prim_t type, ir_const_t value) {
    if (type == value.prim_type) {
        return value;
    }
    if (type == IR_PRIM_BOOL) {
        return ir_calc1(IR_OP1_SNEZ, value);
    }
    if (type == IR_PRIM_F32) {
        if (type == IR_PRIM_F64) {
            value.constf64 = value.constf32;
        } else {
            value = ir_trim_const(value);
#if defined(__SIZEOF_INT128__) && !defined(LILY_SOFT_INT128)
            if (value.prim_type <= IR_PRIM_U128 && (value.prim_type & 1)) {
                value.constf64 = value.const128;
            } else {
                value.constf64 = (__int128_t)value.const128;
            }
#else
            if (value.prim_type == IR_PRIM_S128 || value.prim_type == IR_PRIM_U128) {
                value.constl = value.consth;
            }
            if (value.prim_type <= IR_PRIM_U128 && (value.prim_type & 1)) {
                value.constf32 = value.constl;
            } else {
                value.constf32 = (int64_t)value.constl;
            }
            if (value.prim_type == IR_PRIM_S128 || value.prim_type == IR_PRIM_U128) {
                value.constf32 *= 0x1.0p64f;
            }
#endif
            value.prim_type = IR_PRIM_F32;
            return value;
        }
        return value;
    } else if (type == IR_PRIM_F64) {
        if (type == IR_PRIM_F32) {
            value.constf32 = value.constf64;
        } else {
            value = ir_trim_const(value);
#if defined(__SIZEOF_INT128__) && !defined(LILY_SOFT_INT128)
            if (value.prim_type <= IR_PRIM_U128 && (value.prim_type & 1)) {
                value.constf64 = value.const128;
            } else {
                value.constf64 = (__int128_t)value.const128;
            }
#else
            if (value.prim_type == IR_PRIM_S128 || value.prim_type == IR_PRIM_U128) {
                value.constl = value.consth;
            }
            if (value.prim_type <= IR_PRIM_U128 && (value.prim_type & 1)) {
                value.constf64 = value.constl;
            } else {
                value.constf64 = (int64_t)value.constl;
            }
            if (value.prim_type == IR_PRIM_S128 || value.prim_type == IR_PRIM_U128) {
                value.constf64 *= 0x1.0p64;
            }
#endif
            value.prim_type = IR_PRIM_F64;
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
    if (oper == IR_OP1_MOV) {
        return value;
    } else if (oper == IR_OP1_SNEZ || oper == IR_OP1_SEQZ) {
        bool eqz;
        if (value.prim_type == IR_PRIM_F64) {
            eqz = value.constf64 == 0;
        } else if (value.prim_type == IR_PRIM_F32) {
            eqz = value.constf32 == 0;
        } else if (value.prim_type == IR_PRIM_BOOL) {
            eqz = (value.constl & 1) == 0;
        } else if (value.prim_type == IR_PRIM_S128 || value.prim_type == IR_PRIM_U128) {
            eqz = value.consth == 0 && value.constl == 0;
        } else {
            uint8_t bits = 8 << (value.prim_type >> 1);
            eqz          = (value.constl & ((1llu << bits) - 1)) == 0;
        }
        return (ir_const_t){
            .prim_type = IR_PRIM_BOOL,
            .constl    = eqz ^ (oper == IR_OP1_SNEZ),
            .consth    = 0,
        };
    } else if (oper == IR_OP1_BNEG) {
        if (value.prim_type == IR_PRIM_BOOL) {
            value.constl ^= 1;
        } else if (value.prim_type == IR_PRIM_F32 || value.prim_type == IR_PRIM_F64) {
            fprintf(stderr, "[BUG] Cannot do bitwise negation on f32 or f64\n");
            abort();
        } else {
            value.constl ^= -1;
            value.consth ^= -1;
        }
        return value;
    } else if (oper == IR_OP1_NEG) {
        if (value.prim_type == IR_PRIM_BOOL) {
            fprintf(stderr, "[BUG] Cannot do arithmetic negation on bool\n");
            abort();
        } else if (value.prim_type == IR_PRIM_F64) {
            value.constf64 = -value.constf64;
        } else if (value.prim_type == IR_PRIM_F32) {
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
        fprintf(stderr, "[BUG] Invalid op1 type\n");
        abort();
    }
}

// Calculate the result of an expr2.
ir_const_t ir_calc2(ir_op2_type_t oper, ir_const_t lhs, ir_const_t rhs) {
    ir_const_t out = {.prim_type = lhs.prim_type, .constl = 0, .consth = 0};
    if (oper >= IR_OP2_SGT && oper <= IR_OP2_SNE) {
        out.prim_type = IR_PRIM_BOOL;
    } else {
        out.prim_type = lhs.prim_type;
    }
    if (lhs.prim_type == IR_PRIM_F32) {
        switch (oper) {
            case IR_OP2_SGT: out.constl = lhs.constf32 > rhs.constf32; break;
            case IR_OP2_SLE: out.constl = lhs.constf32 <= rhs.constf32; break;
            case IR_OP2_SLT: out.constl = lhs.constf32 < rhs.constf32; break;
            case IR_OP2_SGE: out.constl = lhs.constf32 >= rhs.constf32; break;
            case IR_OP2_SEQ: out.constl = lhs.constf32 == rhs.constf32; break;
            case IR_OP2_SNE: out.constl = lhs.constf32 != rhs.constf32; break;
            case IR_OP2_ADD: out.constf32 = lhs.constf32 + rhs.constf32; break;
            case IR_OP2_SUB: out.constf32 = lhs.constf32 - rhs.constf32; break;
            case IR_OP2_MUL: out.constf32 = lhs.constf32 * rhs.constf32; break;
            case IR_OP2_DIV: out.constf32 = lhs.constf32 / rhs.constf32; break;
            default:
                fprintf(stderr, "[BUG] Invalid op2 type for f32: %s\n", ir_op2_names[oper]);
                abort();
                break;
        }
        return out;
    } else if (lhs.prim_type == IR_PRIM_F64) {
        switch (oper) {
            case IR_OP2_SGT: out.constl = lhs.constf64 > rhs.constf64; break;
            case IR_OP2_SLE: out.constl = lhs.constf64 <= rhs.constf64; break;
            case IR_OP2_SLT: out.constl = lhs.constf64 < rhs.constf64; break;
            case IR_OP2_SGE: out.constl = lhs.constf64 >= rhs.constf64; break;
            case IR_OP2_SEQ: out.constl = lhs.constf64 == rhs.constf64; break;
            case IR_OP2_SNE: out.constl = lhs.constf64 != rhs.constf64; break;
            case IR_OP2_ADD: out.constf32 = lhs.constf64 + rhs.constf64; break;
            case IR_OP2_SUB: out.constf32 = lhs.constf64 - rhs.constf64; break;
            case IR_OP2_MUL: out.constf32 = lhs.constf64 * rhs.constf64; break;
            case IR_OP2_DIV: out.constf32 = lhs.constf64 / rhs.constf64; break;
            default:
                fprintf(stderr, "[BUG] Invalid op2 type for f64: %s\n", ir_op2_names[oper]);
                abort();
                break;
        }
        return out;
    } else if (lhs.prim_type == IR_PRIM_BOOL) {
        lhs.constl &= 1;
        rhs.constl &= 1;
        switch (oper) {
            case IR_OP2_SEQ: out.constl = 1 & (1 ^ lhs.constl ^ rhs.constl); break;
            case IR_OP2_SNE: out.constl = 1 & (lhs.constl ^ rhs.constl); break;
            case IR_OP2_BAND: out.constl = 1 & (lhs.constl & rhs.constl); break;
            case IR_OP2_BOR: out.constl = 1 & (lhs.constl | rhs.constl); break;
            case IR_OP2_BXOR: out.constl = 1 & (lhs.constl ^ rhs.constl); break;
            default:
                fprintf(stderr, "[BUG] Invalid op2 type for bool: %s\n", ir_op2_names[oper]);
                abort();
                break;
        }
        return out;
    } else if (lhs.prim_type == IR_PRIM_U128) {
        switch (oper) {
            case IR_OP2_SGT: out.constl = cmp128u(lhs.const128, rhs.const128) > 0; break;
            case IR_OP2_SLE: out.constl = cmp128u(lhs.const128, rhs.const128) <= 0; break;
            case IR_OP2_SLT: out.constl = cmp128u(lhs.const128, rhs.const128) < 0; break;
            case IR_OP2_SGE: out.constl = cmp128u(lhs.const128, rhs.const128) >= 0; break;
            case IR_OP2_SEQ: out.constl = cmp128u(lhs.const128, rhs.const128) == 0; break;
            case IR_OP2_SNE: out.constl = cmp128u(lhs.const128, rhs.const128) != 0; break;
            case IR_OP2_ADD: out.const128 = add128(lhs.const128, rhs.const128); break;
            case IR_OP2_SUB: out.const128 = add128(lhs.const128, neg128(rhs.const128)); break;
            case IR_OP2_MUL: out.const128 = mul128(lhs.const128, rhs.const128); break;
            case IR_OP2_DIV: out.const128 = div128u(lhs.const128, rhs.const128); break;
            case IR_OP2_MOD: out.const128 = rem128u(lhs.const128, rhs.const128); break;
            case IR_OP2_SHL: out.const128 = shl128(lhs.const128, rhs.constl); break;
            case IR_OP2_SHR: out.const128 = shr128u(lhs.const128, rhs.constl); break;
            case IR_OP2_BAND: out.const128 = and128(lhs.const128, rhs.const128); break;
            case IR_OP2_BOR: out.const128 = or128(lhs.const128, rhs.const128); break;
            case IR_OP2_BXOR: out.const128 = xor128(lhs.const128, rhs.const128); break;
            default: abort();
        }
        return out;
    } else if (lhs.prim_type == IR_PRIM_S128) {
        switch (oper) {
            case IR_OP2_SGT: out.constl = cmp128s(lhs.const128, rhs.const128) > 0; break;
            case IR_OP2_SLE: out.constl = cmp128s(lhs.const128, rhs.const128) <= 0; break;
            case IR_OP2_SLT: out.constl = cmp128s(lhs.const128, rhs.const128) < 0; break;
            case IR_OP2_SGE: out.constl = cmp128s(lhs.const128, rhs.const128) >= 0; break;
            case IR_OP2_SEQ: out.constl = cmp128s(lhs.const128, rhs.const128) == 0; break;
            case IR_OP2_SNE: out.constl = cmp128s(lhs.const128, rhs.const128) != 0; break;
            case IR_OP2_ADD: out.const128 = add128(lhs.const128, rhs.const128); break;
            case IR_OP2_SUB: out.const128 = add128(lhs.const128, neg128(rhs.const128)); break;
            case IR_OP2_MUL: out.const128 = mul128(lhs.const128, rhs.const128); break;
            case IR_OP2_DIV: out.const128 = div128s(lhs.const128, rhs.const128); break;
            case IR_OP2_MOD: out.const128 = rem128s(lhs.const128, rhs.const128); break;
            case IR_OP2_SHL: out.const128 = shl128(lhs.const128, rhs.constl); break;
            case IR_OP2_SHR: out.const128 = shr128s(lhs.const128, rhs.constl); break;
            case IR_OP2_BAND: out.const128 = and128(lhs.const128, rhs.const128); break;
            case IR_OP2_BOR: out.const128 = or128(lhs.const128, rhs.const128); break;
            case IR_OP2_BXOR: out.const128 = xor128(lhs.const128, rhs.const128); break;
            default: abort();
        }
        return out;
    } else if (lhs.prim_type & 1) {
        lhs = ir_trim_const(lhs);
        rhs = ir_trim_const(rhs);
        switch (oper) {
            case IR_OP2_SGT: out.constl = lhs.constl > rhs.constl; break;
            case IR_OP2_SLE: out.constl = lhs.constl <= rhs.constl; break;
            case IR_OP2_SLT: out.constl = lhs.constl < rhs.constl; break;
            case IR_OP2_SGE: out.constl = lhs.constl >= rhs.constl; break;
            case IR_OP2_SEQ: out.constl = lhs.constl == rhs.constl; break;
            case IR_OP2_SNE: out.constl = lhs.constl != rhs.constl; break;
            case IR_OP2_ADD: out.constl = lhs.constl + rhs.constl; break;
            case IR_OP2_SUB: out.constl = lhs.constl - rhs.constl; break;
            case IR_OP2_MUL: out.constl = lhs.constl * rhs.constl; break;
            case IR_OP2_DIV: out.constl = lhs.constl / rhs.constl; break;
            case IR_OP2_MOD: out.constl = lhs.constl % rhs.constl; break;
            case IR_OP2_SHL: out.constl = lhs.constl << rhs.constl; break;
            case IR_OP2_SHR: out.constl = lhs.constl >> rhs.constl; break;
            case IR_OP2_BAND: out.constl = lhs.constl & rhs.constl; break;
            case IR_OP2_BOR: out.constl = lhs.constl | rhs.constl; break;
            case IR_OP2_BXOR: out.constl = lhs.constl ^ rhs.constl; break;
            default: abort();
        }
        return out;
    } else if (lhs.prim_type & 1) {
        lhs = ir_trim_const(lhs);
        rhs = ir_trim_const(rhs);
        switch (oper) {
            case IR_OP2_SGT: out.constl = (int64_t)lhs.constl > (int64_t)rhs.constl; break;
            case IR_OP2_SLE: out.constl = (int64_t)lhs.constl <= (int64_t)rhs.constl; break;
            case IR_OP2_SLT: out.constl = (int64_t)lhs.constl < (int64_t)rhs.constl; break;
            case IR_OP2_SGE: out.constl = (int64_t)lhs.constl >= (int64_t)rhs.constl; break;
            case IR_OP2_SEQ: out.constl = (int64_t)lhs.constl == (int64_t)rhs.constl; break;
            case IR_OP2_SNE: out.constl = (int64_t)lhs.constl != (int64_t)rhs.constl; break;
            case IR_OP2_ADD: out.constl = (int64_t)lhs.constl + (int64_t)rhs.constl; break;
            case IR_OP2_SUB: out.constl = (int64_t)lhs.constl - (int64_t)rhs.constl; break;
            case IR_OP2_MUL: out.constl = (int64_t)lhs.constl * (int64_t)rhs.constl; break;
            case IR_OP2_DIV: out.constl = (int64_t)lhs.constl / (int64_t)rhs.constl; break;
            case IR_OP2_MOD: out.constl = (int64_t)lhs.constl % (int64_t)rhs.constl; break;
            case IR_OP2_SHL: out.constl = (int64_t)lhs.constl << (int64_t)rhs.constl; break;
            case IR_OP2_SHR: out.constl = (int64_t)lhs.constl >> (int64_t)rhs.constl; break;
            case IR_OP2_BAND: out.constl = (int64_t)lhs.constl & (int64_t)rhs.constl; break;
            case IR_OP2_BOR: out.constl = (int64_t)lhs.constl | (int64_t)rhs.constl; break;
            case IR_OP2_BXOR: out.constl = (int64_t)lhs.constl ^ (int64_t)rhs.constl; break;
            default: abort();
        }
        return out;
    }
    abort();
}
