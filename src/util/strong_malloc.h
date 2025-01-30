
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include <malloc.h>



// Strong malloc; abort if out of memory.
void *strong_malloc(size_t size);
// Strong calloc; abort if out of memory.
void *strong_calloc(size_t size, size_t count);
// Strong realloc; abort if out of memory.
void *strong_realloc(void *ptr, size_t size);
