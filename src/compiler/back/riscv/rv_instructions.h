
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "backend.h"



// RISC-V major opcodes.
typedef enum {
    /* clang-format off */
    RV_OP_LOAD,   RV_OP_LOAD_FP,  RV_OP__cust0, RV_OP_MISC_MEM, RV_OP_OP_IMM, RV_OP_AUIPC, RV_OP_OP_IMM_32, RV_OP__48b,
    RV_OP_STORE,  RV_OP_STORE_FP, RV_OP__cust1, RV_OP_AMO,      RV_OP_OP,     RV_OP_LUI,   RV_OP_OP_32,     RV_OP__64b,
    RV_OP_MADD,   RV_OP_MSUB,     RV_OP_NMSUB,  RV_OP_NMADD,    RV_OP_OP_FP,  RV_OP_OP_V,  RV_OP__cust2,    RV_OP__48b,
    RV_OP_BRANCH, RV_OP_JALR,     RV_OP__resv0, RV_OP_JAL,      RV_OP_SYSTEM, RV_OP_OP_VE, RV_OP__cust3,    RV_OP__80b,
    /* clang-format on */
} rv_opcode_t;

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
} rv_ext_t;
