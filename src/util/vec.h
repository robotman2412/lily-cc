
// Copyright © 2026, Julian Scheffers
// SPDX-License-Identifier: MIT

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

void lilycc_free(void *);



// Raw vector struct, not intended for direct use.
// Explicitly uses a different name for `buffer` so that `ASSERT_IS_VEC()` would fail.
typedef struct {
    uint8_t *buffer;
    size_t   len;
    size_t   cap;
} rawvec_t;

// Get vector element size.
#define VEC_ELEM_SIZE(ptr) sizeof(*(ptr)->arr)
// Get vector element type.
#define VEC_ELEM_TYPE(ptr) __typeof__(*(ptr)->arr)
// Statically assert that some pointer is a vector type.
#define ASSERT_IS_VEC(ptr)                                                                                             \
    {                                                                                                                  \
        _Static_assert(                                                                                                \
            __builtin_types_compatible_p(__typeof__((ptr)->arr), VEC_ELEM_TYPE(ptr) *)                                 \
                && !__builtin_types_compatible_p(VEC_ELEM_TYPE(ptr), void),                                            \
            "Vector `arr` must have pointer to non-void type"                                                          \
        );                                                                                                             \
        _Static_assert(__builtin_types_compatible_p(__typeof__((ptr)->len), size_t), "Vector `len` must be `size_t`"); \
        _Static_assert(__builtin_types_compatible_p(__typeof__((ptr)->cap), size_t), "Vector `cap` must be `size_t`"); \
        _Static_assert(__builtin_offsetof(__typeof__(*(ptr)), arr) == 0, "Vector `arr` must be first element");        \
        _Static_assert(                                                                                                \
            __builtin_offsetof(__typeof__(*(ptr)), len) == sizeof(size_t),                                             \
            "Vector `len` must be second element"                                                                      \
        );                                                                                                             \
        _Static_assert(                                                                                                \
            __builtin_offsetof(__typeof__(*(ptr)), cap) == 2 * sizeof(size_t),                                         \
            "Vector `cap` must be third element"                                                                       \
        );                                                                                                             \
    }

// Define a vector type.
#define VEC_TYPE_DEF(name, elem_type)                                                                                  \
    typedef struct {                                                                                                   \
        __typeof__(elem_type) *arr;                                                                                    \
        size_t                 len;                                                                                    \
        size_t                 cap;                                                                                    \
    } name; /* NOLINT(bugprone-macro-parentheses) */

// Reserve space for at least N additional elements.
// Not intended for direct use.
void rawvec_reserve(rawvec_t *vec, size_t elem_size, size_t n_elem);
// Reserve space for exactly N additional elements.
// Not intended for direct use.
void rawvec_reserve_exact(rawvec_t *vec, size_t elem_size, size_t n_elem);
// Insert elements into the vector.
// Not intended for direct use.
void rawvec_insert_n(rawvec_t *vec, size_t elem_size, size_t index, void const *arr, size_t count);
// Remove elements from the vector.
// Not intended for direct use.
void rawvec_remove_n(rawvec_t *vec, size_t elem_size, size_t index, size_t count, void *arr_out_opt);

// Helper macros for direct calls to a corresponding `rawvec` function.
#define rawvec_call(func, vec, ...)                                                                                    \
    ({                                                                                                                 \
        __auto_type vec__rawvec_vall = (vec);                                                                          \
        ASSERT_IS_VEC(vec);                                                                                            \
        func((rawvec_t *)(vec__rawvec_vall), VEC_ELEM_SIZE(vec) __VA_OPT__(, ) __VA_ARGS__);                           \
    })

// Reserve space for at least N additional elements.
#define vec_reserve(vec, n_elem)       rawvec_call(rawvec_reserve, (vec), (n_elem))
// Reserve space for exactly N elements.
#define vec_reserve_exact(vec, n_elem) rawvec_call(rawvec_reserve_exact, (vec), (n_elem))
// Insert an element into the vector.
#define vec_insert(vec, index, value)  vec_insert((vec), (index), (value), 1)
// Insert elements into the vector.
#define vec_insert_n(vec, index, value, count)                                                                         \
    rawvec_call(rawvec_insert, (vec), (index), (VEC_ELEM_TYPE(vec)[1]){(value)}, (vount))
// Remove an element from the vector.
#define vec_remove(vec, index)                                                                                         \
    ({                                                                                                                 \
        __auto_type vec__remove = (vec);                                                                               \
        VEC_ELEM_TYPE(vec) out;                                                                                        \
        rawvec_call(rawvec_remove_n, vec__remove, index, 1, &out);                                                     \
        out;                                                                                                           \
    })
// Remove elements from the vector.
#define vec_remove_n(vec, index, count, arr_out_opt) rawvec_call(rawvec_remove_n, (index), (arr_out_opt), (count))
// Push an element to the back of the vector.
#define vec_push(vec, value)                                                                                           \
    ({                                                                                                                 \
        __auto_type vec__push = (vec);                                                                                 \
        rawvec_call(rawvec_reserve, vec__push, 1);                                                                     \
        vec__push->arr[vec__push->len] = (value);                                                                      \
        vec__push->len++;                                                                                              \
    })
// Run a function on all elements in the vector by value.
#define vec_foreach_val(vec, func_)                                                                                    \
    ({                                                                                                                 \
        __auto_type vec__foreach_val = (vec);                                                                          \
        __auto_type func             = (func_);                                                                        \
        ASSERT_IS_VEC(vec);                                                                                            \
        for (size_t i = 0; i < vec__foreach_val->len; i++) {                                                           \
            func(vec__foreach_val->arr[i]);                                                                            \
        }                                                                                                              \
    })
// Run a function on all elements in the vector by reference.
#define vec_foreach_ref(vec, func_)                                                                                    \
    ({                                                                                                                 \
        __auto_type vec__foreach_ref = (vec);                                                                          \
        __auto_type func             = (func_);                                                                        \
        ASSERT_IS_VEC(vec);                                                                                            \
        for (size_t i = 0; i < vec__foreach_ref->len; i++) {                                                           \
            func(&vec__foreach_ref->arr[i]);                                                                           \
        }                                                                                                              \
    })
// Clear a vector and free its memory.
#define vec_clear(vec)                                                                                                 \
    ({                                                                                                                 \
        __auto_type vec__clear = (vec);                                                                                \
        ASSERT_IS_VEC(vec);                                                                                            \
        lilycc_free(vec__clear->arr);                                                                                  \
        vec__clear->arr = NULL;                                                                                        \
        vec__clear->len = 0;                                                                                           \
        vec__clear->cap = 0;                                                                                           \
    })



VEC_TYPE_DEF(vec_char_t, char)
VEC_TYPE_DEF(vec_schar_t, signed char)
VEC_TYPE_DEF(vec_short_t, short)
VEC_TYPE_DEF(vec_int_t, int)
VEC_TYPE_DEF(vec_long_t, long)
VEC_TYPE_DEF(vec_llong_t, long long)
VEC_TYPE_DEF(vec_uchar_t, char)
VEC_TYPE_DEF(vec_ushort_t, short)
VEC_TYPE_DEF(vec_uint_t, int)
VEC_TYPE_DEF(vec_ulong_t, long)
VEC_TYPE_DEF(vec_ullong_t, long long)

VEC_TYPE_DEF(vec_uint8_t, uint8_t)
VEC_TYPE_DEF(vec_uint16_t, uint16_t)
VEC_TYPE_DEF(vec_uint32_t, uint32_t)
VEC_TYPE_DEF(vec_uint64_t, uint64_t)
VEC_TYPE_DEF(vec_int8_t, int8_t)
VEC_TYPE_DEF(vec_int16_t, int16_t)
VEC_TYPE_DEF(vec_int32_t, int32_t)
VEC_TYPE_DEF(vec_int64_t, int64_t)
VEC_TYPE_DEF(vec_size_t, size_t)

VEC_TYPE_DEF(vec_bool_t, _Bool)
VEC_TYPE_DEF(vec_ptr_t, void *)
VEC_TYPE_DEF(vec_cstr_t, char *)
