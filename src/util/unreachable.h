
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once



#ifdef NDEBUG
// Mark some code as unreachable.
#define UNREACHABLE() __builtin_unreachable()
#else
// Mark some code as unreachable.
void lilycc_unreachable_code(char const *file, int line) __attribute__((noreturn));
// Mark some code as unreachable.
#define UNREACHABLE() lilycc_unreachable_code(__FILE__, __LINE__)
#endif
