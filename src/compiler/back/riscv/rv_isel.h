
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "backend.h"



// Perform instruction selection.
isel_t rv_isel(backend_profile_t *profile, ir_insn_t const *ir_insn);
