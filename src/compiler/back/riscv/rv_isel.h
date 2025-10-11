
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "backend.h"



// Perform instruction selection.
ir_insn_t *rv_isel(backend_profile_t *profile, ir_insn_t *ir_insn);

// Post-isel pass that creates instructions for loading immediates where only registers are accepted.
void rv_post_isel(backend_profile_t *profile, ir_func_t *func);
