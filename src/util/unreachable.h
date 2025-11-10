
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once



#include <stdio.h>
#include <stdlib.h>

#ifdef NDEBUG
// Mark some code as unreachable.
#define UNREACHABLE() __builtin_unreachable()
#else
// Mark some code as unreachable.
void UNREACHABLE(char const *file, int line) __attribute__((noreturn));
// Mark some code as unreachable.
#define UNREACHABLE() UNREACHABLE(__FILE__, __LINE__)
#endif
