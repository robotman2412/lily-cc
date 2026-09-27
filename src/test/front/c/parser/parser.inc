
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "../options.inc"
#include "c_parser.h"
#include "c_tokenizer.h"
#include "testcase.h"

#include <stdio.h>

static bool c_parser_test_diagnostics(cctx_t *cctx) {
    if (!cctx->diagnostics.len) {
        return false;
    }
    diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
    printf("\n");
    while (diag) {
        print_diagnostic(diag, stderr);
        diag = (diagnostic_t const *)diag->node.next;
    }
    return true;
}
