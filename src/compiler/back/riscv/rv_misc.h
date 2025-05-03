
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

// RISC-V instruction set extensions.
typedef enum {
    // Always allowed.
    RV_BASE,
    // Allowed if RV32 but not RV64 or RV128.
    RV_32ONLY,
    // Allowed if RV64.
    RV_64,
    // Allowed if RV128,
    RV_128,
    // Allowed if M is present.
    RV_EXT_M,
    // Allowed if A is present.
    RV_EXT_A,
    // Allowed if C is present.
    RV_EXT_C,
    // Allowed if F is present.
    RV_EXT_F,
    // Allowed if D is present,
    RV_EXT_D,
    // Number of RISC-V extensions recognised.
    RV_N_EXT,
} rv_ext_t;
