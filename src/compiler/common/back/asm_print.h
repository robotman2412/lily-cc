
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "backend.h"
#include "ir_types.h"



// Print a reference to the local label for a code block.
void asm_print_code_label(ir_code_t const *code, FILE *to);

// Print a function for the assembler.
void asm_print_func(ir_func_t *func, backend_profile_t *profile, FILE *to);
