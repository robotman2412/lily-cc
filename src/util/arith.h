
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

// Checked addition, evaluates to true if the operation overflowed.
#define lilycc_add_overflow(a, b, res) __builtin_add_overflow((a), (b), (res))
// Checked subtraction, evaluates to true if the operation overflowed.
#define lilycc_sub_overflow(a, b, res) __builtin_sub_overflow((a), (b), (res))
// Checked multiplication, evaluates to true if the operation overflowed.
#define lilycc_mul_overflow(a, b, res) __builtin_mul_overflow((a), (b), (res))
