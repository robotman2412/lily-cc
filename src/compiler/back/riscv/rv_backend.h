
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "backend.h"
#include "rv_misc.h"



#define RV_GPR_OFFSET 0
#define RV_FPR_OFFSET 32


#define RV_REG_RA    1
#define RV_REG_SP    2
#define RV_REG_GP    3
#define RV_REG_TP    4
#define RV_REG_T(tn) ((tn) <= 2 ? (tn) + 5 : (tn) + 28 - 3)
#define RV_REG_S(sn) ((sn) <= 2 ? (sn) + 8 : (sn) + 18 - 2)
#define RV_REG_A(an) ((an) + 10)

#define RV_ALL_T_REGS(X, C)        X(5) C X(6) C X(7) C X(28) C X(29) C X(30) C X(31)
#define RV_ALL_S_REGS(X, C)        X(8) C X(9) C X(18) C X(19) C X(20) C X(21) C X(22) C X(23) C X(24) C X(25) C X(26) C X(27)
#define RV_NONRVE_S_REGS(X, C)     X(18) C X(19) C X(20) C X(21) C X(22) C X(23) C X(24) C X(25) C X(26) C X(27)
#define RV_NONRET_A_REGS(X, C)     X(12) C X(13) C X(14) C X(15) C X(16) C X(17)
#define RV_RVE_NONRET_A_REGS(X, C) X(12) C X(13) C X(14) C X(15)
#define RV_RVE_T_REGS(X, C)        X(5) C X(6) C X(7)
#define RV_ALL_A_REGS(X, C)        X(10) C X(11) C X(12) C X(13) C X(14) C X(15) C X(16) C X(17)
#define RV_ALL_GPRS(X, C)                                                                                              \
    X(1)                                                                                                               \
    C X(2) C X(3) C X(4) C X(5) C X(6) C X(7) C X(8) C X(9) C X(10) C X(11) C X(12) C X(13) C X(14) C X(15) C X(16)    \
        C X(17) C X(18) C X(19) C X(20) C X(21) C X(22) C X(23) C X(24) C X(25) C X(26) C X(27) C X(28) C X(29)        \
            C X(30) C X(31)

#define RV_REG_T_MAX 6
#define RV_REG_S_MAX 11
#define RV_REG_A_MAX 7


#define RV_REG_FT(tn) ((tn) <= 7 ? (tn) + 0 : (tn) + 28 - 8)
#define RV_REG_FS(sn) ((sn) <= 2 ? (sn) + 8 : (sn) + 18 - 2)
#define RV_REG_FA(an) ((an) + 10)

#define RV_ALL_FT_REGS(X, C)    X(0) C X(1) C X(2) C X(3) C X(4) C X(5) C X(6) C X(7) C X(28) C X(29) C X(30) C X(31)
#define RV_ALL_FS_REGS(X, C)    X(8) C X(9) C X(18) C X(19) C X(20) C X(21) C X(22) C X(23) C X(24) C X(25) C X(26) C X(27)
#define RV_NONRET_FA_REGS(X, C) X(12) C X(13) C X(14) C X(15) C X(16) C X(17)
#define RV_ALL_FA_REGS(X, C)    X(10) C X(11) C X(12) C X(13) C X(14) C X(15) C X(16) C X(17)
#define RV_ALL_FPRS(X, C)                                                                                              \
    X(0)                                                                                                               \
    C X(1) C X(2) C X(3) C X(4) C X(5) C X(6) C X(7) C X(8) C X(9) C X(10) C X(11) C X(12) C X(13) C X(14) C X(15)     \
        C X(16) C X(17) C X(18) C X(19) C X(20) C X(21) C X(22) C X(23) C X(24) C X(25) C X(26) C X(27) C X(28)        \
            C X(29) C X(30) C X(31)

#define RV_REG_FT_MAX 11
#define RV_REG_FS_MAX 11
#define RV_REG_FA_MAX 7



// Supported RISC-V ABI types.
typedef enum {
    // 32-bit.
    RV_ABI_ILP32,
    // 32-bit, RVE registers only.
    RV_ABI_ILP32E,
    // 32-bit, float32.
    RV_ABI_ILP32F,
    // 32-bit, float64.
    RV_ABI_ILP32D,
    // 64-bit.
    RV_ABI_LP64,
    // 64-bit, float32.
    RV_ABI_LP64F,
    // 64-bit, float64.
    RV_ABI_LP64D,
} rv_abi_t;



// RISC-V backend profile.
typedef struct rv_profile rv_profile_t;



// RISC-V backend profile.
struct rv_profile {
    // Common backend profile information.
    backend_profile_t base;
    // Selected ABI.
    rv_abi_t          abi;
    // Profile is RV*E.
    bool              is_rve;
    // Which extensions are enabled in this profile.
    bool              ext_enabled[RV_N_EXT];
};



// Create a copy of the default profile for this type of backend.
backend_profile_t *rv_create_profile();
// Delete a profile for this backend.
void               rv_delete_profile(backend_profile_t *profile);
// Prepare backend for codegen stage.
void               rv_init_codegen(backend_profile_t *profile);



// The RISC-V backend.
extern backend_t const   rv_backend;
// Table of RISC-V register names.
extern char const *const rv_reg_names[];
