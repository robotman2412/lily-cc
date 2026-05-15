
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_preproc.h"
#include "c_tokenizer.h"

#include <stdio.h>

static void preprocess(char const *path) {
    cctx_t    *cctx = cctx_create();
    srcfile_t *src  = srcfile_open(cctx, path);
    if (!src) {
        perror("Cannot open source file");
        cctx_delete(cctx);
        return;
    }
    c_preproc_t *pre = c_preproc_create(src, C_STD_def);
    if (!pre) {
        cctx_delete(cctx);
        return;
    }
    pre->raw_mode = true;

    while (1) {
        token_t tkn = c_preproc_next(&pre->base);
        if (tkn.type == TOKENTYPE_EOF) {
            tkn_delete(tkn);
            break;
        }
        c_tkn_print_src(&tkn, stdout);
        tkn_delete(tkn);
    }
    fputc('\n', stdout);

    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        fflush(stdout);
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        fflush(stderr);
    }

    tkn_ctx_delete(&pre->base);
    cctx_delete(cctx);
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        preprocess(argv[i]);
    }
}
