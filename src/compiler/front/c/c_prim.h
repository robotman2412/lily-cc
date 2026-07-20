
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once


#include "ir_types.h"
#include "unreachable.h"

#include <stdint.h>



// C type primitives.
typedef enum {
    // `_Bool`, `bool`
    C_PRIM_BOOL,
    // `char`
    C_PRIM_CHAR,
    // `signed char`
    C_PRIM_SCHAR,
    // `unsigned char`
    C_PRIM_UCHAR,
    // `(signed) short (int)`
    C_PRIM_SSHORT,
    // `unsigned short (int)`
    C_PRIM_USHORT,
    // `(signed) int`, `signed`
    C_PRIM_SINT,
    // `unsigned (int)`
    C_PRIM_UINT,
    // `(signed) long (int)`
    C_PRIM_SLONG,
    // `unsigned long (int)`
    C_PRIM_ULONG,
    // `(signed) long long (int)`
    C_PRIM_SLLONG,
    // `unsigned long long (int)`
    C_PRIM_ULLONG,
    // `(signed) __int128`
    C_PRIM_S128,
    // `unsigned __int128`
    C_PRIM_U128,

    // `float`
    C_PRIM_FLOAT,
    // `double`
    C_PRIM_DOUBLE,
    // `long double`
    C_PRIM_LDOUBLE,

    // `void`
    C_PRIM_VOID,

    // Number of type primitives.
    C_N_PRIM,

    // Type is a struct.
    C_COMP_STRUCT,
    // Type is an union.
    C_COMP_UNION,
    // Type is an enum.
    C_COMP_ENUM,
    // Type is a pointer.
    C_COMP_POINTER,
    // Type is an array.
    C_COMP_ARRAY,
    // Type is a function.
    C_COMP_FUNCTION,
} c_prim_t;

// Primitive types' names.
extern char const *c_prim_name[];

// Whether a primitive is an usigned integer type.
static inline bool c_prim_is_uint(bool char_is_signed, c_prim_t prim) {
    switch (prim) {
        case C_PRIM_CHAR: return char_is_signed;
        case C_PRIM_UCHAR:
        case C_PRIM_USHORT:
        case C_PRIM_UINT:
        case C_PRIM_ULONG:
        case C_PRIM_ULLONG:
        case C_PRIM_U128: return true;
        default: return false;
    }
}

// Whether a primitive is a signed or unsigned integer type.
static inline bool c_prim_is_int(c_prim_t prim) {
    switch (prim) {
        case C_PRIM_BOOL:
        case C_PRIM_CHAR:
        case C_PRIM_UCHAR:
        case C_PRIM_SCHAR:
        case C_PRIM_USHORT:
        case C_PRIM_SSHORT:
        case C_PRIM_UINT:
        case C_PRIM_SINT:
        case C_PRIM_ULONG:
        case C_PRIM_SLONG:
        case C_PRIM_ULLONG:
        case C_PRIM_SLLONG:
        case C_PRIM_S128:
        case C_PRIM_U128: return true;
        default: return false;
    }
}

// Whether a primitive is a scalar type.
static inline bool c_prim_is_scalar(c_prim_t prim) {
    switch (prim) {
        case C_PRIM_BOOL:
        case C_PRIM_CHAR:
        case C_PRIM_UCHAR:
        case C_PRIM_SCHAR:
        case C_PRIM_USHORT:
        case C_PRIM_SSHORT:
        case C_PRIM_UINT:
        case C_PRIM_SINT:
        case C_PRIM_ULONG:
        case C_PRIM_SLONG:
        case C_PRIM_ULLONG:
        case C_PRIM_SLLONG:
        case C_PRIM_S128:
        case C_PRIM_U128:
        case C_PRIM_FLOAT:
        case C_PRIM_DOUBLE:
        case C_PRIM_LDOUBLE: return true;
        case C_N_PRIM:
        case C_PRIM_VOID:
        case C_COMP_STRUCT:
        case C_COMP_UNION:
        case C_COMP_ENUM: return false;
        case C_COMP_POINTER: return true;
        case C_COMP_ARRAY:
        case C_COMP_FUNCTION: return false;
    }
    UNREACHABLE();
}

// Whether a primitive is or decays into a pointer.
static inline bool c_prim_is_ptr(c_prim_t prim) {
    return prim == C_COMP_POINTER || prim == C_COMP_ARRAY;
}
