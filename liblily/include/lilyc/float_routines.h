
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

// Integer arithmetic routines that may be implicitly called by Lily-CC if the machine instruction is not available.

#pragma once

#include "./float_properties.h"
#include "./int_properties.h"

// Helper macro that defines an operation for each float type.
#define _LILYC_DEFINE_FLOAT_ROUTINE_2(__name)                                                                          \
    _LILYC_IF_F32(__lily_f32 __name##_f32(__lily_f32 __lhs, __lily_f32 __rhs);)                                        \
    _LILYC_IF_F64(__lily_f64 __name##_f64(__lily_f64 __lhs, __lily_f64 __rhs);)

// Helper macro that defines an operation for each float type.
#define _LILYC_DEFINE_FLOAT_ROUTINE_1(__name)                                                                          \
    _LILYC_IF_F32(__lily_f32 __name##_f32(__lily_f32 __a);)                                                            \
    _LILYC_IF_F64(__lily_f64 __name##_f64(__lily_f64 __a);)

// Helper macro that defines an operation for each float type.
#define _LILYC_DEFINE_ICONV_ROUTINE(__name, __type)                                                                    \
    __lily_s8   __name##_s8(__type __a);                                                                               \
    __lily_s16  __name##_s16(__type __a);                                                                              \
    __lily_s32  __name##_s32(__type __a);                                                                              \
    __lily_s64  __name##_s64(__type __a);                                                                              \
    __lily_s128 __name##_s128(__type __a);

// Helper macro that defines an operation for each float type.
#define _LILYC_DEFINE_FCONV_ROUTINE(__name, __type)                                                                    \
    __type __name##_s8(__lily_s8 __a);                                                                                 \
    __type __name##_s16(__lily_s16 __a);                                                                               \
    __type __name##_s32(__lily_s32 __a);                                                                               \
    __type __name##_s64(__lily_s64 __a);                                                                               \
    __type __name##_s128(__lily_s128 __a);

// Convert `__a` to a 64-bit float.
__lily_f64 __lily_fconv_f64(__lily_f32 __a);
// Convert `__a` to a 32-bit float.
__lily_f32 __lily_fconv_f32(__lily_f64 __a);
// Convert `__a` to an integer.
// Clamps infinities within range; returns max value if NaN.
_LILYC_DEFINE_ICONV_ROUTINE(__lily_ftoi_f32, __lily_f32)
_LILYC_DEFINE_ICONV_ROUTINE(__lily_ftoi_f64, __lily_f64)
// Convert `__a` to a float.
// May lose precision.
_LILYC_DEFINE_FCONV_ROUTINE(__lily_itof_f32, __lily_f32)
_LILYC_DEFINE_FCONV_ROUTINE(__lily_itof_f64, __lily_f64)

// Negate `__a`.
_LILYC_DEFINE_FLOAT_ROUTINE_1(__lily_neg)
// Add `__lhs` to `__rhs`.
_LILYC_DEFINE_FLOAT_ROUTINE_2(__lily_add)
// Subtract `__rhs` from `__lhs`.
_LILYC_DEFINE_FLOAT_ROUTINE_2(__lily_sub)
// Multiply `__lhs` by `__rhs`.
_LILYC_DEFINE_FLOAT_ROUTINE_2(__lily_mul)
// Divide `__lhs` by `__rhs`.
_LILYC_DEFINE_FLOAT_ROUTINE_2(__lily_div)
// Remainder of `__lhs` divided by `__rhs`.
_LILYC_DEFINE_FLOAT_ROUTINE_2(__lily_rem)
// Square root of `__a`.
_LILYC_DEFINE_FLOAT_ROUTINE_1(__lily_sqrt)
// Absolute value of `__a`.
_LILYC_DEFINE_FLOAT_ROUTINE_1(__lily_abs)
