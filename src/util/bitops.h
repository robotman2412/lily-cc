
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include <stddef.h>
#include <stdint.h>

// Count ones.
static inline int lilycc_popcount32(uint32_t x) {
    return __builtin_popcount(x);
}
static inline int lilycc_popcount64(uint64_t x) {
    return __builtin_popcountll(x);
}

// Count trailing (least-significant) zeroes.
static inline int lilycc_ctz32(uint32_t x) {
    return __builtin_ctz(x);
}
static inline int lilycc_ctz64(uint64_t x) {
    return __builtin_ctzll(x);
}

// Count leading (most-significant) zeroes.
static inline int lilycc_clz32(uint32_t x) {
    return __builtin_clz(x);
}
static inline int lilycc_clz64(uint64_t x) {
    return __builtin_clzll(x);
}
// Size depends on the platform
static inline int lilycc_clzl(size_t x) {
    return __builtin_clzl(x);
}
