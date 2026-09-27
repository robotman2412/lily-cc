
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "../options.inc"
#include "c_ast.h"
#include "c_compile.h"
#include "c_compile_expr.h"
#include "c_compiler.h"
#include "c_ir.h"
#include "c_parser.h"
#include "c_types.h"
#include "compiler.h"
#include "list.h"
#include "testcase.h"
#include "tokenizer.h"

#include <stdio.h>
#include <string.h>

static bool c_compiler_test_diagnostics(cctx_t *cctx) {
    if (!cctx->diagnostics.len) {
        return false;
    }
    fprintf(stdout, "\n");
    dlist_foreach_node(diagnostic_t, diag, &cctx->diagnostics) {
        print_diagnostic(diag, stdout);
    }
    return true;
}
