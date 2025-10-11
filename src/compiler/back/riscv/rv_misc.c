
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "rv_misc.h"

// RISC-V ELF relocation names.
char const *const rv_reloc_names[] = {
#define RV_RELOC_DEF(ord, name) [ord] = #name,
#include "rv_reloc.inc"
};
