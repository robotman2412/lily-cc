
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include <endian.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



#define INLINE_MATH128 static inline __attribute__((always_inline)) __attribute__((const))

#define I128_ZERO i128_pack(0, 0)
#define UI128_MAX i128_pack(UINT64_MAX, UINT64_MAX)
#define I128_MIN  i128_pack(INT64_MIN, 0)
#define I128_MAX  i128_pack(INT64_MAX, UINT64_MAX)

#if defined(__SIZEOF_INT128__) && !defined(LILY_SOFT_INT128)
// Structure that stores 128 bits of data.
typedef struct __attribute__((packed, aligned(8))) {
    // The struct overrides the alignment because the normal 16 is not needed.
    // This allows other structs that depend on this one to be smaller.
    __uint128_t val;
} i128_t;

// Cast to signed 128-bit integer.
#define i128(i64)         ((i128_t){(__int128_t)(i64)})
// Cast to unsigned 128-bit integer.
#define ui128(ui64)       ((i128_t){(__uint128_t)(ui64)})
// Create a 128-bit integer from two 64-bit ones.
#define i128_pack(hi, lo) ((i128_t){(((__uint128_t)(uint64_t)(hi) << 64) | ((uint64_t)(lo)))})
// Get low 64 bits of 128-bit integer.
#define lo64(i128)        ((uint64_t)(i128).val)
// Get high 64 bits of 128-bit integer.
#define hi64(i128)        ((uint64_t)((i128).val >> 64))

// Perform 128-bit multipliciation.
INLINE_MATH128 i128_t mul128(i128_t lhs, i128_t rhs) {
    return (i128_t){lhs.val * rhs.val};
}

// Perform unsigned 128-bit division.
INLINE_MATH128 i128_t div128u(i128_t lhs, i128_t rhs) {
    return (i128_t){lhs.val / rhs.val};
}

// Perform unsigned 128-bit remainder.
INLINE_MATH128 i128_t rem128u(i128_t lhs, i128_t rhs) {
    return (i128_t){lhs.val % rhs.val};
}

// Perform signed 128-bit division.
INLINE_MATH128 i128_t div128s(i128_t lhs, i128_t rhs) {
    return (i128_t){(__int128_t)lhs.val / (__int128_t)rhs.val};
}

// Perform signed 128-bit remainder.
INLINE_MATH128 i128_t rem128s(i128_t lhs, i128_t rhs) {
    return (i128_t){(__int128_t)lhs.val % (__int128_t)rhs.val};
}

// Perform 128-bit addition.
INLINE_MATH128 i128_t add128(i128_t lhs, i128_t rhs) {
    return (i128_t){lhs.val + rhs.val};
}

// Perform 128-bit subtraction.
INLINE_MATH128 i128_t sub128(i128_t lhs, i128_t rhs) {
    return (i128_t){lhs.val - rhs.val};
}

// Perform 128-bit arithmetic negation.
INLINE_MATH128 i128_t neg128(i128_t a) {
    return (i128_t){-a.val};
}


// Perform unsigned 128-bit comparison.
INLINE_MATH128 int cmp128u(i128_t lhs, i128_t rhs) {
    if (lhs.val < rhs.val) {
        return -1;
    } else if (lhs.val > rhs.val) {
        return 1;
    } else {
        return 0;
    }
}

// Perform unsigned 128-bit comparison.
INLINE_MATH128 int cmp128s(i128_t lhs, i128_t rhs) {
    if ((__int128_t)lhs.val < (__int128_t)rhs.val) {
        return -1;
    } else if ((__int128_t)lhs.val > (__int128_t)rhs.val) {
        return 1;
    } else {
        return 0;
    }
}


// Perform unsigned 128-bit right shift.
INLINE_MATH128 i128_t shr128u(i128_t lhs, int rhs) {
    rhs &= 127;
    return (i128_t){lhs.val >> rhs};
}

// Perform signed 128-bit right shift.
INLINE_MATH128 i128_t shr128s(i128_t lhs, int rhs) {
    rhs &= 127;
    return (i128_t){(__int128_t)lhs.val >> rhs};
}

// Perform 128-bit left shift.
INLINE_MATH128 i128_t shl128(i128_t lhs, int rhs) {
    rhs &= 127;
    return (i128_t){lhs.val << rhs};
}

// Perform 128-bit bitwise AND.
INLINE_MATH128 i128_t and128(i128_t lhs, i128_t rhs) {
    return (i128_t){lhs.val & rhs.val};
}

// Perform 128-bit bitwise OR.
INLINE_MATH128 i128_t or128(i128_t lhs, i128_t rhs) {
    return (i128_t){lhs.val | rhs.val};
}

// Perform 128-bit bitwise XOR.
INLINE_MATH128 i128_t xor128(i128_t lhs, i128_t rhs) {
    return (i128_t){lhs.val ^ rhs.val};
}

// Perform 128-bit bitwise negation.
INLINE_MATH128 i128_t bneg128(i128_t a) {
    return (i128_t){~a.val};
}
#else

// Structure that stores 128 bits of data.
typedef struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint64_t lo, hi;
#elif __BYTE_ORDER == __BIG_ENDIAN
    uint64_t hi, lo;
#else
#error                                                                                                                 \
    "Invalid endianness; Lily-CC requires a machine that is big-endian or little-endian, but the endianness either different or not detected"
#endif
} i128_t;

// Cast to signed 128-bit integer.
#define i128(i64)         ((i128_t){.lo = (i64), .hi = (int64_t)(i64) >> 63})
// Cast to unsigned 128-bit integer.
#define ui128(ui64)       ((i128_t){.lo = (ui64), .hi = 0})
// Create a 128-bit integer from two 64-bit ones.
#define i128_pack(hi, lo) ((i128_t){.lo = (lo), .hi = (hi)})
// Get low 64 bits of 128-bit integer.
#define lo64(i128)        ((i128).lo)
// Get high 64 bits of 128-bit integer.
#define hi64(i128)        ((i128).hi)

// Perform 128-bit multipliciation.
i128_t mul128(i128_t lhs, i128_t rhs) __attribute__((const));
// Perform unsigned 128-bit division.
i128_t div128u(i128_t lhs, i128_t rhs) __attribute__((const));
// Perform unsigned 128-bit remainder.
i128_t rem128u(i128_t lhs, i128_t rhs) __attribute__((const));
// Perform signed 128-bit division.
i128_t div128s(i128_t lhs, i128_t rhs) __attribute__((const));
// Perform signed 128-bit remainder.
i128_t rem128s(i128_t lhs, i128_t rhs) __attribute__((const));

// Perform 128-bit addition.
INLINE_MATH128 i128_t add128(i128_t lhs, i128_t rhs) {
    lhs.lo += rhs.lo;
    lhs.hi += rhs.hi;
    if (lhs.lo < rhs.lo) {
        lhs.hi++;
    }
    return lhs;
}

// Perform 128-bit subtraction.
INLINE_MATH128 i128_t sub128(i128_t lhs, i128_t rhs) {
    lhs.lo -= rhs.lo;
    lhs.hi -= rhs.hi;
    if (lhs.lo > rhs.lo) {
        lhs.hi--;
    }
    return lhs;
}

// Perform 128-bit arithmetic negation.
INLINE_MATH128 i128_t neg128(i128_t a) {
    a.lo ^= -1;
    a.hi ^= -1;
    a.lo++;
    if (a.lo == 0) {
        a.hi++;
    }
    return a;
}

// Perform unsigned 128-bit comparison.
INLINE_MATH128 int cmp128u(i128_t lhs, i128_t rhs) {
    if (lhs.hi < rhs.hi) {
        return -1;
    } else if (lhs.hi > rhs.hi) {
        return 1;
    } else if (lhs.lo < rhs.lo) {
        return -1;
    } else if (lhs.lo > rhs.lo) {
        return 1;
    } else {
        return 0;
    }
}

// Perform unsigned 128-bit comparison.
INLINE_MATH128 int cmp128s(i128_t lhs, i128_t rhs) {
    lhs.hi ^= 1llu << 63;
    rhs.hi ^= 1llu << 63;
    return cmp128u(lhs, rhs);
}

// Perform unsigned 128-bit right shift.
INLINE_MATH128 i128_t shr128u(i128_t lhs, int rhs) {
    rhs &= 127;
    if (rhs >= 64) {
        lhs.lo = lhs.hi >> (rhs & 63);
        lhs.hi = 0;
    } else {
        lhs.lo >>= rhs;
        lhs.lo  |= lhs.hi << (64 - rhs);
        lhs.hi >>= rhs;
    }
    return lhs;
}

// Perform signed 128-bit right shift.
INLINE_MATH128 i128_t shr128s(i128_t lhs, int rhs) {
    rhs &= 127;
    if (rhs >= 64) {
        lhs.hi = lhs.hi & (1 << 63) ? -1 : 0;
        lhs.lo = (int64_t)lhs.hi >> (rhs & 63);
    } else {
        lhs.lo >>= rhs;
        lhs.lo  |= lhs.hi << (64 - rhs);
        lhs.hi >>= rhs;
    }
    return lhs;
}

// Perform 128-bit left shift.
INLINE_MATH128 i128_t shl128(i128_t lhs, int rhs) {
    rhs &= 127;
    if (rhs >= 64) {
        lhs.hi = lhs.lo << (rhs & 63);
        lhs.lo = 0;
    } else {
        lhs.hi <<= rhs;
        lhs.hi  |= lhs.lo >> (64 - rhs);
        lhs.lo <<= rhs;
    }
    return lhs;
}

// Perform 128-bit bitwise AND.
INLINE_MATH128 i128_t and128(i128_t lhs, i128_t rhs) {
    lhs.lo &= rhs.lo;
    lhs.hi &= rhs.hi;
    return lhs;
}

// Perform 128-bit bitwise OR.
INLINE_MATH128 i128_t or128(i128_t lhs, i128_t rhs) {
    lhs.lo |= rhs.lo;
    lhs.hi |= rhs.hi;
    return lhs;
}

// Perform 128-bit bitwise XOR.
INLINE_MATH128 i128_t xor128(i128_t lhs, i128_t rhs) {
    lhs.lo ^= rhs.lo;
    lhs.hi ^= rhs.hi;
    return lhs;
}

// Perform 128-bit bitwise negation.
INLINE_MATH128 i128_t bneg128(i128_t a) {
    return (i128_t){
        .lo = ~a.lo,
        .hi = ~a.hi,
    };
}
#endif

// Perform 128-bit addition (saturating, unsigned).
INLINE_MATH128 i128_t add128u_saturate(i128_t lhs, i128_t rhs) {
    i128_t res = add128(lhs, rhs);
    if (cmp128u(res, lhs) < 0) {
        return UI128_MAX;
    }
    return res;
}

// Perform 128-bit addition (saturating, signed).
INLINE_MATH128 i128_t add128s_saturate(i128_t lhs, i128_t rhs) {
    if (cmp128s(lhs, I128_ZERO) > 0) {
        if (cmp128s(rhs, sub128(I128_MAX, lhs)) > 0) {
            return I128_MAX;
        }
    } else {
        if (cmp128s(rhs, sub128(I128_MIN, lhs)) < 0) {
            return I128_MIN;
        }
    }
    return add128(lhs, rhs);
}

// Perform 128-bit subtraction (saturating, unsigned).
INLINE_MATH128 i128_t sub128u_saturate(i128_t lhs, i128_t rhs) {
    i128_t res = sub128(lhs, rhs);
    if (cmp128u(res, lhs) > 0) {
        return I128_ZERO;
    }
    return res;
}

// Perform 128-bit subtraction (saturating, signed).
INLINE_MATH128 i128_t sub128s_saturate(i128_t lhs, i128_t rhs) {
    if (cmp128s(lhs, I128_ZERO) > 0) {
        if (cmp128s(rhs, sub128(lhs, I128_MAX)) < 0) {
            return I128_MAX;
        }
    } else {
        if (cmp128s(rhs, sub128(lhs, I128_MIN)) > 0) {
            return I128_MIN;
        }
    }
    return sub128(lhs, rhs);
}

// Perform 128-bit multiplication (saturating, unsigned).
INLINE_MATH128 i128_t mul128u_saturate(i128_t lhs, i128_t rhs) {
    i128_t res = mul128(lhs, rhs);
    if (cmp128u(lhs, I128_ZERO) != 0 && cmp128u(div128u(res, lhs), rhs) != 0) {
        return UI128_MAX;
    }
    return res;
}

// Perform 128-bit multiplication (saturating, signed).
INLINE_MATH128 i128_t mul128s_saturate(i128_t lhs, i128_t rhs) {
    i128_t res = mul128(lhs, rhs);
    if (cmp128s(lhs, I128_ZERO) != 0 && cmp128s(div128s(res, lhs), rhs) != 0) {
        if ((cmp128s(lhs, I128_ZERO) > 0) == (cmp128s(rhs, I128_ZERO) > 0)) {
            return I128_MAX;
        }
        return I128_MIN;
    }
    return res;
}



// Count trailing zeroes.
int ctz128(i128_t a) __attribute__((const));
// Count leading zeroes.
int clz128(i128_t a) __attribute__((const));

// Convert a 128-bit integer to decimal (unsigned).
// Assumes a buffer of at least 40 bytes is provided.
void itoa128(i128_t n, int decimals, char buf[static 40]);
