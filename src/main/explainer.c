
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_ast.h"
#include "c_compile.h"
#include "c_compiler.h"
#include "c_ir.h"
#include "c_parser.h"
#include "c_types.h"
#include "compiler.h"
#include "tokenizer.h"

#include <stdio.h>
#include <string.h>


static bool compile_explain_type(bool is_detailed, char const *value) {
    cctx_t       *cctx = cctx_create();
    srcfile_t    *src  = srcfile_create(cctx, "<argv>", value, strlen(value));
    c_compiler_t *cc   = c_compiler_create(
        cctx,
        (c_options_t){
            .c_std          = C_STD_def,
            .char_is_signed = true,
            .short16        = true,
            .int32          = true,
            .long64         = true,
            .size_type      = C_PRIM_ULONG,
        }
    );
    tokenizer_t *tctx = c_tokenizer_create(cc, src, false);
    c_parser_t  *pctx = c_parser_create(cc, tctx);

    c_ast_type_name_t *ast = c_parse_type_name(pctx);
    token_t            eof = tkn_peek(tctx);
    if (eof.type != TOKENTYPE_EOF) {
        cctx_diagnostic(cc->cctx, eof.pos, DIAG_ERR, "Expected end-of-file");
    }

    cir_scope_t *scope = cir_scope_create(CIR_SCOPE_GLOBAL, NULL);
    c_type_t     type  = c_compile_spec_qual_list(cc, ast->spec_qual, scope);
    if (ast->decl) {
        type = c_compile_type(cc, scope, type, ast->decl, NULL);
    }

    c_type_explain(cc, type, is_detailed, stdout);
    if (!is_detailed) {
        putc('\n', stdout);
    }

    bool errors = cctx->diagnostics.len;
    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        fflush(stdout);
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        fflush(stderr);
    }

    c_type_delete(type);
    c_ast_type_name_delete(ast);
    cir_scope_delete(scope);
    c_parser_delete(pctx);
    c_compiler_delete(cc);
    cctx_delete(cctx);

    return errors;
}

int main(int argc, char **argv) {
    int i = 1;

    // Parse options.
    bool detailed = false;
    bool help     = argc <= 1;
    bool error    = false;
    for (; i < argc; i++) {
        if (argv[i][0] != '-') {
            break;
        }
        if (!strcmp(argv[i], "--")) {
            i++;
            break;
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            help = true;
        } else if (!strcmp(argv[i], "--detailed")) {
            detailed = true;
        } else {
            printf("%s: error: unrecognised option `%s`\n", argv[0], argv[i]);
            error = true;
        }
    }

    if (help) {
        printf("Usage %s [options] type...\n", argv[0]);
        printf("Options:\n");
        printf("  -h --help     Show this help text.\n");
        printf("  --detailed    Print extra details including size and alignment.\n");
    }
    if (help || error) {
        return 1;
    }

    // Explain types.
    for (; i < argc; i++) {
        error |= compile_explain_type(detailed, argv[i]);
    }

    return error;
}
