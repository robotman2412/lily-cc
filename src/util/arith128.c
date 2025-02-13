
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "arith128.h"

#if !defined(__SIZEOF_INT128__) || defined(LILY_SOFT_INT128)

typedef struct {
    i128_t div, rem;
} i128_divrem_t;

// Perform 64 to 128-bit multiplication.
static i128_t mul64to128(uint64_t lhs, uint64_t rhs) {
    uint64_t lhs_lo = (uint32_t)lhs;
    uint64_t lhs_hi = lhs >> 32;
    uint64_t rhs_lo = (uint32_t)rhs;
    uint64_t rhs_hi = rhs >> 32;

    uint64_t res0  = lhs_lo * rhs_lo;
    uint64_t res1a = lhs_hi * rhs_lo;
    uint64_t res1b = lhs_lo * rhs_hi;
    uint64_t res2  = lhs_hi * rhs_hi;

    uint64_t res1  = res1a + res1b;
    bool     res1c = res1 < res1a;

    uint64_t lo = res0 + (res1 << 32);
    uint64_t hi = res2 + (res1 >> 32) + ((uint64_t)res1c << 32);
    if (lo < res0) {
        hi++;
    }

    return (i128_t){lo, hi};
}

// Perform 128-bit multipliciation.
i128_t mul128(i128_t lhs, i128_t rhs) {
    i128_t   res0  = mul64to128(lhs.lo, rhs.lo);
    uint64_t res1a = lhs.lo * rhs.hi;
    uint64_t res1b = lhs.hi * rhs.lo;
    return (i128_t){
        res0.lo,
        res0.hi + res1a + res1b,
    };
}

// Perform unsigned 128-bit division and modulo.
i128_divrem_t divrem128u(i128_t lhs, i128_t rhs) {
    if (lhs.hi == 0 && rhs.hi == 0) {
        return (i128_divrem_t) {
            .div = {
                lhs.lo / rhs.lo,
                0,
            },
            .rem = {
                lhs.lo % rhs.lo,
                0,
            },
        };
    }

    // Determine max shift amount.
    int max;
    if (rhs.hi == 0) {
        max = __builtin_clzll(rhs.lo) + 64;
    } else {
        max = __builtin_clzll(rhs.hi);
    }

    // Shift divisor left by this amount.
    i128_t value;
    if (max >= 64) {
        rhs.hi   = rhs.lo << (max & 63);
        rhs.lo   = 0;
        value.hi = 1 << (max & 63);
        value.lo = 0;
    } else {
        rhs.hi   <<= max;
        rhs.hi    |= rhs.lo >> (64 - max);
        rhs.lo   <<= max;
        value.hi   = 0;
        value.lo   = 1 << max;
    }

    i128_divrem_t out = {
        .div = 0,
        .rem = lhs,
    };

    rhs = neg128(rhs);
    while (!(value.lo & 1)) {
        // Check if it can be subtracted.
        i128_t tmp = add128(out.rem, rhs);
        if (tmp.hi > out.rem.hi || (tmp.hi == out.rem.hi && tmp.lo > out.rem.lo)) {
            // Carry out is set; can subtract.
            out.rem     = tmp;
            out.div.hi |= value.hi;
            out.div.lo |= value.lo;
        }

        // Shift value right by one.
        value.lo >>= 1;
        value.lo  |= value.hi << 63;
        value.hi >>= 1;
        rhs.lo   >>= 1;
        rhs.lo    |= rhs.hi << 63;
        rhs.hi     = (rhs.hi >> 1) | (rhs.hi & (1llu << 63));
    }

    return out;
}

// Perform signed 128-bit division and modulo.
static i128_divrem_t divrem128s(i128_t lhs, i128_t rhs) {
    bool div_sign = (lhs.hi & (1llu << 63)) ^ (rhs.hi & (1llu << 63));
    bool rem_sign = lhs.hi & (1llu << 63);

    if (lhs.hi & (1llu << 63)) {
        lhs = neg128(lhs);
    }
    if (rhs.hi & (1llu << 63)) {
        rhs = neg128(rhs);
    }

    i128_divrem_t res = divrem128u(lhs, rhs);

    if (div_sign) {
        res.div = neg128(res.div);
    }
    if (rem_sign) {
        res.rem = neg128(res.rem);
    }

    return res;
}

// Perform unsigned 128-bit division.
i128_t div128u(i128_t lhs, i128_t rhs) {
    return divrem128u(lhs, rhs).div;
}

// Perform unsigned 128-bit remainder.
i128_t rem128u(i128_t lhs, i128_t rhs) {
    return divrem128u(lhs, rhs).rem;
}

// Perform unsigned 128-bit division.
i128_t div128s(i128_t lhs, i128_t rhs) {
    return divrem128s(lhs, rhs).div;
}

// Perform unsigned 128-bit remainder.
i128_t rem128s(i128_t lhs, i128_t rhs) {
    return divrem128s(lhs, rhs).rem;
}

#endif