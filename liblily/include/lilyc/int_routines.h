
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

// Integer arithmetic routines that may be implicitly called by Lily-CC if the machine instruction is not available.

#pragma once

#include "./int_properties.h"



// Attributes to apply to Lily-C integer routines.
#if __STDC_VERSION__ >= 202311L
#define _LILYC_INT_ATTR [[gnu::const]] [[unsequenced]] [[nodiscard]]
#elif defined(__GNUC__)
#define _LILYC_INT_ATTR __attribute__((__const__)) __attribute__((__warn_unused_result__))
#else
#define _LILYC_INT_ATTR
#endif

// Helper macro that defines an operation for each integer paired with u8 RHS.
#define _LILYC_DEFINE_INT_ROUTINE_SHIFT(__sign, __name)                                                                \
    _LILYC_IF_8(_LILYC_INT_ATTR __lily_##__sign##8 __name##8(__lily_##__sign##8 __lhs, __lily_u8 __rhs);)              \
    _LILYC_IF_16(_LILYC_INT_ATTR __lily_##__sign##16 __name##16(__lily_##__sign##16 __lhs, __lily_u8 __rhs);)          \
    _LILYC_IF_32(_LILYC_INT_ATTR __lily_##__sign##32 __name##32(__lily_##__sign##32 __lhs, __lily_u8 __rhs);)          \
    _LILYC_IF_64(_LILYC_INT_ATTR __lily_##__sign##64 __name##64(__lily_##__sign##64 __lhs, __lily_u8 __rhs);)          \
    _LILYC_IF_128(_LILYC_INT_ATTR __lily_##__sign##128 __name##128(__lily_##__sign##128 __lhs, __lily_u8 __rhs);)

// Helper macro that defines an operation for each integer width.
#define _LILYC_DEFINE_INT_ROUTINE_2(__sign, __name)                                                                    \
    _LILYC_IF_8(_LILYC_INT_ATTR __lily_##__sign##8 __name##8(__lily_##__sign##8 __lhs, __lily_##__sign##8 __rhs);)     \
    _LILYC_IF_16(                                                                                                      \
        _LILYC_INT_ATTR __lily_##__sign##16 __name##16(__lily_##__sign##16 __lhs, __lily_##__sign##16 __rhs);          \
    )                                                                                                                  \
    _LILYC_IF_32(                                                                                                      \
        _LILYC_INT_ATTR __lily_##__sign##32 __name##32(__lily_##__sign##32 __lhs, __lily_##__sign##32 __rhs);          \
    )                                                                                                                  \
    _LILYC_IF_64(                                                                                                      \
        _LILYC_INT_ATTR __lily_##__sign##64 __name##64(__lily_##__sign##64 __lhs, __lily_##__sign##64 __rhs);          \
    )                                                                                                                  \
    _LILYC_IF_128(                                                                                                     \
        _LILYC_INT_ATTR __lily_##__sign##128 __name##128(__lily_##__sign##128 __lhs, __lily_##__sign##128 __rhs);      \
    )

// Helper macro that defines an operation for each integer width.
#define _LILYC_DEFINE_INT_ROUTINE_1(__sign, __name)                                                                    \
    _LILYC_IF_8(_LILYC_INT_ATTR __lily_##__sign##8 __name##8(__lily_##__sign##8 __a);)                                 \
    _LILYC_IF_16(_LILYC_INT_ATTR __lily_##__sign##16 __name##16(__lily_##__sign##16 __a);)                             \
    _LILYC_IF_32(_LILYC_INT_ATTR __lily_##__sign##32 __name##32(__lily_##__sign##32 __a);)                             \
    _LILYC_IF_64(_LILYC_INT_ATTR __lily_##__sign##64 __name##64(__lily_##__sign##64 __a);)                             \
    _LILYC_IF_128(_LILYC_INT_ATTR __lily_##__sign##128 __name##128(__lily_##__sign##128 __a);)

// Helper macro that defines an operation for each integer type/width.
#define _LILYC_DEFINE_INT_ROUTINE_2_SU(__name)                                                                         \
    _LILYC_DEFINE_INT_ROUTINE_2(s, __name##_s) _LILYC_DEFINE_INT_ROUTINE_2(u, __name##_u)

// Helper macro that defines an operation for each integer type/width.
#define _LILYC_DEFINE_INT_ROUTINE_1_SU(__name)                                                                         \
    _LILYC_DEFINE_INT_ROUTINE_1(s, __name##_s) _LILYC_DEFINE_INT_ROUTINE_1(u, __name##_u)



// Shift `__lhs` left by `__rhs` bits.
_LILYC_DEFINE_INT_ROUTINE_SHIFT(u, __lily_shl_u)
// Shift `__lhs` right by `__rhs` bits, sign-extended.
_LILYC_DEFINE_INT_ROUTINE_SHIFT(s, __lily_shr_s)
// Shift `__lhs` right by `__rhs` bits, zero-extended.
_LILYC_DEFINE_INT_ROUTINE_SHIFT(u, __lily_shr_u)

// Multiply `__lhs` by `__rhs`.
_LILYC_DEFINE_INT_ROUTINE_2(u, __lily_mul_u)
// Divide `__lhs` by `__rhs`.
_LILYC_DEFINE_INT_ROUTINE_2_SU(__lily_div)
// Remainder of `__lhs` divided by `__rhs`.
_LILYC_DEFINE_INT_ROUTINE_2_SU(__lily_rem)

// Count leading zeroes in the binary representation `__a`.
// Return value for input 0 is undefined.
_LILYC_DEFINE_INT_ROUTINE_1(u, __lily_clz_u)
// Count trailing zeroes in the binary representation `__a`.
// Return value for input 0 is undefined.
_LILYC_DEFINE_INT_ROUTINE_1(u, __lily_ctz_u)
// Count number of set bits in the binary representation of `__a`.
_LILYC_DEFINE_INT_ROUTINE_1(u, __lily_popcnt_u)
