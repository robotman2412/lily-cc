
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_options.h"
#include "c_prim.h"
#include "c_std.h"

static c_options_t c_test_options = {
    .c_std          = C_STD_max,
    .gnu_ext_enable = true,
    .char_is_signed = true,
    .short16        = true,
    .int32          = true,
    .long64         = true,
    .big_endian     = false,
    .size_type      = C_PRIM_ULONG,
};
