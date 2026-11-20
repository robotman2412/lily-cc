
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "asm_print.h"
#include "backend.h"
#include "c_ast.h"
#include "c_compile.h"
#include "c_compiler.h"
#include "c_ir.h"
#include "c_lower.h"
#include "c_parser.h"
#include "codegen.h"
#include "ir.h"
#include "ir_optimizer.h"
#include "ir_serialization.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool optimize = false;
static bool emit_ir  = false;
static bool selected = false;

// The order here is also the execution order for explicitly selected passes.
static struct {
    char const *name;
    bool (*run)(ir_func_t *);
    bool enabled;
} passes[] = {
    {"const-prop", opt_const_prop, false},
    {"mem2reg", opt_mem2reg, false},
    {"unused-vars", opt_unused_vars, false},
    {"dead-code", opt_dead_code, false},
    {"branches", opt_branches, false},
    {"strength-reduce", opt_strength_reduce, false},
    {"unused-frames", opt_unused_frames, false},
};

static void print_help(char const *program) {
    printf(
        "Usage: %s [options] file...\n"
        "Compile C source files to assembly on standard output.\n\n"
        "Options:\n"
        "  -h, --help          Show this help menu and exit\n"
        "  -O, --optimize      Run the default ir_optimize pipeline\n"
        "      --passes=LIST   Run selected comma-separated passes until nothing changes\n"
        "      --emit-ir       Emit IR instead of assembly\n"
        "      --              Treat remaining arguments as file names\n\n"
        "Available passes (execution order):\n",
        program
    );
    for (size_t i = 0; i < sizeof(passes) / sizeof(*passes); i++) {
        printf("  %s\n", passes[i].name);
    }
    printf(
        "\nRepeated --passes options add to the selection.\n"
        "The branches pass also enables dead-code, which it requires.\n"
        "--optimize and --passes cannot be combined.\n"
        "Without either option, no optimization passes are run.\n"
    );
}

static bool select_passes(char const *list) {
    do {
        size_t len   = strcspn(list, ",");
        bool   found = false;
        for (size_t i = 0; i < sizeof(passes) / sizeof(*passes); i++) {
            if (strlen(passes[i].name) == len && !strncmp(list, passes[i].name, len)) {
                passes[i].enabled = true;
                found             = true;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "Unknown optimization pass: '%.*s'\n", (int)len, list);
            return false;
        }
        list += len;
    } while (*list++ == ',');

    // opt_branches requires dead-code cleanup and up-to-date flow information.
    for (size_t i = 0; i < sizeof(passes) / sizeof(*passes); i++) {
        if (passes[i].run == opt_branches && passes[i].enabled) {
            for (size_t j = 0; j < sizeof(passes) / sizeof(*passes); j++) {
                if (passes[j].run == opt_dead_code) {
                    passes[j].enabled = true;
                }
            }
        }
    }
    return true;
}

static void optimize_selected(ir_func_t *ir) {
    ir_func_to_ssa(ir);
    bool changed;
    do {
        changed = false;
        for (size_t i = 0; i < sizeof(passes) / sizeof(*passes); i++) {
            if (passes[i].enabled) {
                changed |= passes[i].run(ir);
            }
        }
    } while (changed);
}

static void compile2(char const *path) {
    // Create requisite contexts.
    cctx_t    *cctx = cctx_create();
    srcfile_t *src  = srcfile_open(cctx, path);
    if (!src) {
        perror("Cannot open source file");
        cctx_delete(cctx);
        return;
    }
    c_compiler_t *cc = c_compiler_create(
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
    tokenizer_t       *tctx    = c_tokenizer_create(cc, src, true);
    c_parser_t        *pctx    = c_parser_create(cc, tctx);
    backend_t const   *backend = backend_default();
    backend_profile_t *profile = backend->create_profile();
    backend->init_codegen(profile);

    printf("// Compiling %s\n", path);

    // While not EOF, keep parsing and compiling stuff.
    c_ast_def_list_t *ast = c_parse(pctx);
    cir_trans_unit_t *tu  = c_compile(cc, ast);

    if (cctx->n_err == 0) {
        for (size_t i = 0; i < tu->units.len; i++) {
            if (tu->units.arr[i]->tag != CIR_UNIT_FUNC) {
                continue;
            }
            ir_func_t *ir = c_lower_func(cc, tu->units.arr[i]->func);
            if (optimize) {
                ir_optimize(ir);
            } else if (selected) {
                optimize_selected(ir);
            }
            if (emit_ir) {
                printf("\n");
                ir_func_serialize(ir, profile, stdout);
            } else {
                codegen(profile, ir);
                printf("\n");
                asm_print_func(ir, profile, stdout);
            }
        }
    }

    c_ast_def_list_delete(ast);
    cir_trans_unit_delete(tu);

    // Print diagnostics.
    if (cctx->diagnostics.len) {
        diagnostic_t const *diag = (diagnostic_t const *)cctx->diagnostics.head;
        printf("\n");
        fflush(stdout);
        while (diag) {
            print_diagnostic(diag, stderr);
            diag = (diagnostic_t const *)diag->node.next;
        }
        fflush(stderr);
    }

    // Clean up.
    backend->delete_profile(profile);
    c_parser_delete(pctx);
    c_compiler_delete(cc);
    cctx_delete(cctx);
}

int main(int argc, char **argv) {
    static struct option const options[] = {
        {"help", no_argument, NULL, 'h'},
        {"optimize", no_argument, NULL, 'O'},
        {"passes", required_argument, NULL, 'p'},
        {"emit-ir", no_argument, NULL, 'i'},
        {NULL, 0, NULL, 0},
    };
    int option;
    while ((option = getopt_long(argc, argv, "hOpi", options, NULL)) != -1) {
        switch (option) {
            case 'h': print_help(argv[0]); return EXIT_SUCCESS;
            case 'O': optimize = true; break;
            case 'i': emit_ir = true; break;
            case 'p':
                if (!select_passes(optarg)) {
                    return EXIT_FAILURE;
                }
                selected = true;
                break;
            default: return EXIT_FAILURE;
        }
    }
    if (optimize && selected) {
        fprintf(stderr, "--optimize and --passes cannot be combined\n");
        return EXIT_FAILURE;
    }
    if (optind == argc) {
        fprintf(stderr, "No input files; use --help for usage\n");
        return EXIT_FAILURE;
    }
    for (int i = optind; i < argc; i++) {
        compile2(argv[i]);
    }
    return EXIT_SUCCESS;
}
