
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once



#include "c_prim.h"

#include <stdint.h>



// C compiler options.
typedef struct c_options c_options_t;

// C compiler options.
struct c_options {
    // Current C standard.
    int      c_std;
    // GNU extensions are enabled.
    uint32_t gnu_ext_enable : 1;
    // Char is signed by default.
    uint32_t char_is_signed : 1;
    // `short` is 16-bit.
    uint32_t short16        : 1;
    // `int` is 32-bit.
    uint32_t int32          : 1;
    // `long` is 64-bit.
    uint32_t long64         : 1;
    // Target is big-endian.
    uint32_t big_endian     : 1;
    // C primitive corresponding to unsigned size_t.
    c_prim_t size_type;
};
