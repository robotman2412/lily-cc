
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "asm_print.h"

#include "ir_types.h"
#include "list.h"

#include <inttypes.h>
#include <stdio.h>



// Print a reference to the local label for a code block.
void asm_print_code_label(ir_code_t const *code, FILE *to) {
    fprintf(to, ".L%s", code->name);
}

// Print a function for the assembler.
void asm_print_func(ir_func_t *func, backend_profile_t *profile, FILE *to) {
    // TODO: Symbol visibility.
    fprintf(to, "    .section \".text\", \"ax\"\n");
    fprintf(to, "    .type %s, @function\n", func->name);
    profile->backend->asm_print_prefunc(profile, func, to);
    fprintf(to, "%s:\n", func->name);

    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        asm_print_code_label(code, to);
        fputs(":\n", to);
        dlist_foreach_node(ir_insn_t, insn, &code->insns) {
            fputs("    ", to);
            profile->backend->asm_print_insn(profile, insn, to);
            fputc('\n', to);
        }
    }

    fprintf(to, "    .size %s, .-%s\n", func->name, func->name);
}
