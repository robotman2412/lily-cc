
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include <endian.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



#define INLINE_MATH128 static inline __attribute__((always_inline)) __attribute__((const))

#if defined(__SIZEOF_INT128__) && !defined(LILY_SOFT_INT128)
// Structure that stores 128 bits of data.
typedef struct __attribute__((packed, aligned(8))) {
    // The struct overrides the alignment because the normal 16 is not needed.
    // This allows other structs that depend on this one to be smaller.
    __uint128_t val;
} i128_t;

// Create a 128-bit integer from two 64-bit ones.
#define int128(hi, lo) ((i128_t){(((__uint128_t)(uint64_t)(hi) << 64) | ((uint64_t)(lo)))})
// Get low 64 bits of 128-bit integer.
#define lo64(i128)     ((uint64_t)(i128).val)
// Get high 64 bits of 128-bit integer.
#define hi64(i128)     ((uint64_t)((i128).val >> 64))

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

// Create a 128-bit integer from two 64-bit ones.
#define int128(hi, lo) ((i128_t){.lo = (lo), .hi = (hi)})
// Get low 64 bits of 128-bit integer.
#define lo64(i128)     ((i128).lo)
// Get high 64 bits of 128-bit integer.
#define hi64(i128)     ((i128).hi)

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

// Convert a 128-bit integer to decimal (unsigned).
// Assumes a buffer of at least 40 bytes is provided.
void itoa128(i128_t n, int decimals, char buf[static 40]);
