
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "backend.h"



// Expand the ABI for a specific return instruction.
ir_insn_t *rv_xabi_return(backend_profile_t *profile, ir_insn_t *ret_insn);
// Expand the ABI for a specific call instruction.
ir_insn_t *rv_xabi_call(backend_profile_t *profile, ir_insn_t *call_insn);
// Expand the ABI for a function entry.
void       rv_xabi_entry(backend_profile_t *profile, ir_func_t *func);
