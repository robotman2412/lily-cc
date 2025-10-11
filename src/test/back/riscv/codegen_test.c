
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT


#include "backend.h"
#include "codegen.h"
#include "ir.h"
#include "ir_serialization.h"
#include "ir_types.h"
#include "rv_backend.h"
#include "testcase.h"



char *test_rv_isel() {
    // clang-format off
    char const ir_src[] =
    "ssa_function <test_rv_isel>\n"
    "    entry %code0\n"
    "    var %a u32\n"
    "    var %tmp1 u32\n"
    "    var %tmp2 u32\n"
    "    var %tmp3 s8\n"
    "    var %tmp4 bool\n"
    "    arg %a\n"
    "code %code0\n"
    "    %tmp1 = add %a, u32'0xf00\n"
    "    %tmp2 = add %tmp1, u32'0xcafebabe\n"
    "    %tmp4 = sgt u32'0x1000, %tmp2\n"
    "    branch (%code1), %tmp4\n"
    "    jump (%code2)\n"
    "code %code2\n"
    "    %tmp3 = load (s8 %tmp2)\n"
    "code %code1\n"
    ;
    // clang-format on

    ir_func_t *func = ir_func_deserialize_str(ir_src, sizeof(ir_src), "<test_rv_isel>");
    if (!func) {
        return TEST_FAIL_MSG("Skipped");
    }

    backend_profile_t *profile = rv_create_profile();
    profile->backend->init_codegen(profile);
    codegen(profile, func);
    putchar('\n');
    ir_func_serialize(func, profile, stdout);

    ir_func_delete(func);
    profile->backend->delete_profile(profile);

    return TEST_OK;
}
LILY_TEST_CASE(test_rv_isel)
