
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



#if defined(__SIZEOF_INT128__) && !defined(LILY_SOFT_INT128)
// Structure that stores 128 bits of data.
typedef __uint128_t i128_t;

// Create a 128-bit integer from two 64-bit ones.
#define int128(hi, lo) (((i128_t)(uint64_t)(hi) << 64) | ((uint64_t)(lo)))
// Get low 64 bits of 128-bit integer.
#define lo64(i128)     ((uint64_t)(i128))
// Get high 64 bits of 128-bit integer.
#define hi64(i128)     ((uint64_t)((i128) >> 64))

// Perform 128-bit multipliciation.
static inline __attribute__((always_inline)) i128_t mul128(i128_t lhs, i128_t rhs) {
    return lhs * rhs;
}

// Perform unsigned 128-bit division.
static inline __attribute__((always_inline)) i128_t div128u(i128_t lhs, i128_t rhs) {
    return lhs / rhs;
}

// Perform unsigned 128-bit remainder.
static inline __attribute__((always_inline)) i128_t rem128u(i128_t lhs, i128_t rhs) {
    return lhs % rhs;
}

// Perform signed 128-bit division.
static inline __attribute__((always_inline)) i128_t div128s(i128_t lhs, i128_t rhs) {
    return (__int128_t)lhs / (__int128_t)rhs;
}

// Perform signed 128-bit remainder.
static inline __attribute__((always_inline)) i128_t rem128s(i128_t lhs, i128_t rhs) {
    return (__int128_t)lhs % (__int128_t)rhs;
}

// Perform 128-bit addition.
static inline __attribute__((always_inline)) i128_t add128(i128_t lhs, i128_t rhs) {
    return lhs + rhs;
}

// Perform 128-bit negation.
static inline __attribute__((always_inline)) i128_t neg128(i128_t a) {
    return -a;
}


// Perform unsigned 128-bit comparison.
static inline __attribute__((always_inline)) int cmp128u(i128_t lhs, i128_t rhs) {
    if (lhs < rhs) {
        return -1;
    } else if (lhs > rhs) {
        return 1;
    } else {
        return 0;
    }
}

// Perform unsigned 128-bit comparison.
static inline __attribute__((always_inline)) int cmp128s(i128_t lhs, i128_t rhs) {
    if ((__int128_t)lhs < (__int128_t)rhs) {
        return -1;
    } else if ((__int128_t)lhs > (__int128_t)rhs) {
        return 1;
    } else {
        return 0;
    }
}


// Perform unsigned 128-bit right shift.
static inline __attribute__((always_inline)) int shr128u(i128_t lhs, int rhs) {
    rhs &= 127;
    return lhs >> rhs;
}

// Perform signed 128-bit right shift.
static inline __attribute__((always_inline)) int shr128s(i128_t lhs, int rhs) {
    rhs &= 127;
    return (__int128_t)lhs >> rhs;
}

// Perform 128-bit left shift.
static inline __attribute__((always_inline)) int shl128(i128_t lhs, int rhs) {
    rhs &= 127;
    return lhs << rhs;
}

// Perform 128-bit bitwise AND.
static inline __attribute__((always_inline)) int and128(i128_t lhs, i128_t rhs) {
    return lhs & rhs;
}

// Perform 128-bit bitwise OR.
static inline __attribute__((always_inline)) int or128(i128_t lhs, i128_t rhs) {
    return lhs | rhs;
}

// Perform 128-bit bitwise XOR.
static inline __attribute__((always_inline)) int xor128(i128_t lhs, i128_t rhs) {
    return lhs ^ rhs;
}
#else

// Structure that stores 128 bits of data.
typedef struct {
    uint64_t lo, hi;
} i128_t;

// Create a 128-bit integer from two 64-bit ones.
#define int128(lo, hi) ((i128_t){(lo), (hi)})
// Get low 64 bits of 128-bit integer.
#define lo64(i128)     ((i128).lo)
// Get high 64 bits of 128-bit integer.
#define hi64(i128)     ((i128).hi)

// Perform 128-bit multipliciation.
i128_t mul128(i128_t lhs, i128_t rhs);
// Perform unsigned 128-bit division.
i128_t div128u(i128_t lhs, i128_t rhs);
// Perform unsigned 128-bit remainder.
i128_t rem128u(i128_t lhs, i128_t rhs);
// Perform signed 128-bit division.
i128_t div128s(i128_t lhs, i128_t rhs);
// Perform signed 128-bit remainder.
i128_t rem128s(i128_t lhs, i128_t rhs);

// Perform 128-bit addition.
static inline __attribute__((always_inline)) i128_t add128(i128_t lhs, i128_t rhs) {
    lhs.lo += rhs.lo;
    lhs.hi += rhs.hi;
    if (lhs.lo < rhs.lo) {
        lhs.hi++;
    }
    return lhs;
}

// Perform 128-bit negation.
static inline __attribute__((always_inline)) i128_t neg128(i128_t a) {
    a.lo ^= -1;
    a.hi ^= -1;
    a.lo++;
    if (a.lo == 0) {
        a.hi++;
    }
    return a;
}

// Perform unsigned 128-bit comparison.
static inline __attribute__((always_inline)) int cmp128u(i128_t lhs, i128_t rhs) {
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
static inline __attribute__((always_inline)) int cmp128s(i128_t lhs, i128_t rhs) {
    lhs.hi ^= 1llu << 63;
    rhs.hi ^= 1llu << 63;
    return cmp128u(lhs, rhs);
}

// Perform unsigned 128-bit right shift.
static inline __attribute__((always_inline)) int shr128u(i128_t lhs, int rhs) {
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
static inline __attribute__((always_inline)) int shr128s(i128_t lhs, int rhs) {
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
static inline __attribute__((always_inline)) int shl128(i128_t lhs, int rhs) {
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
static inline __attribute__((always_inline)) int and128(i128_t lhs, i128_t rhs) {
    lhs.lo &= rhs.lo;
    lhs.hi &= rhs.hi;
    return lhs;
}

// Perform 128-bit bitwise OR.
static inline __attribute__((always_inline)) int or128(i128_t lhs, i128_t rhs) {
    lhs.lo |= rhs.lo;
    lhs.hi |= rhs.hi;
    return lhs;
}

// Perform 128-bit bitwise XOR.
static inline __attribute__((always_inline)) int xor128(i128_t lhs, i128_t rhs) {
    lhs.lo ^= rhs.lo;
    lhs.hi ^= rhs.hi;
    return lhs;
}
#endif
