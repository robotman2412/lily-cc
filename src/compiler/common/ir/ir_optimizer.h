
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "ir.h"



// Run optimizations on some IR.
// Returns whether any code was changed.
bool ir_optimize(ir_func_t *func);

// Optimization: Delete all variables and assignments to them whose value is never read.
// Returns whether any variables were deleted.
bool opt_unused_vars(ir_func_t *func);
// Optimization: Delete code from dead paths.
// Returns whether any code was changed or removed.
bool opt_dead_code(ir_func_t *func);
// Optimization: Propagate constants.
// Returns whether any code was changed.
bool opt_const_prop(ir_func_t *func);
// Optimization: Remove redundant branches.
// Returns whether any code was changed.
// WARNING: You MUST run the dead code optimization first.
bool opt_branches(ir_func_t *func);
