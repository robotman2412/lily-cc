
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once



// Count ones.
#define lilycc_popcount32(x) (__builtin_popcount(x))
// Count ones.
#define lilycc_popcount64(x) (__builtin_popcountll(x))
// Count trailing (least-significant) zeroes.
#define lilycc_ctz32(x)      (__builtin_ctz(x))
// Count trailing (least-significant) zeroes.
#define lilycc_ctz64(x)      (__builtin_ctzll(x))
// Count leading (most-significant) zeroes.
#define lilycc_clz32(x)      (__builtin_clz(x))
// Count leading (most-significant) zeroes.
#define lilycc_clz64(x)      (__builtin_clzll(x))
