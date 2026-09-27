
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "compiler.h"
#include "ir.h"
#include "ir_serialization.h"
#include "ir_tokenizer.h"
#include "testcase.h"
#include "tokenizer.h"

#include <stdio.h>



int main() {
    // clang-format off
    char const src[] =
    "function <test_ir_deserialize>\n"
    "    var %a s32\n"
    "    var %b s32\n"
    "    var %c bool\n"
    "    entry %code0\n"
    "code %code0\n"
    "    %b = add %a, s32'-3\n"
    "    %c = seqz %b\n"
    "    branch (%code1), %c\n"
    "    return %b\n"
    "code %code1\n"
    "    return s32'1\n"
    ;
    // clang-format on

    // Create tokenizer context.
    cctx_t      *cctx    = cctx_create();
    srcfile_t   *srcfile = srcfile_create(cctx, "<test_ir_deserialize>.lily_ir", src, sizeof(src) - 1);
    tokenizer_t *tctx    = ir_tkn_create(srcfile);

    // Try to deserialize this function.
    ir_func_t *func = ir_func_deserialize(tctx);
    if (func) {
        // printf("\n");
        // ir_func_serialize(func, stdout);
        ir_func_delete(func);
    }

    // Report errors.
    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        tkn_ctx_delete(tctx);
        cctx_delete(cctx);
        TEST_FAIL();
    }

    // Clean up.
    tkn_ctx_delete(tctx);
    cctx_delete(cctx);

    TEST_OK();
}
