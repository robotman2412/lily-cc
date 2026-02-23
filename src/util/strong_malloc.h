
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include <malloc.h>



// Strong malloc; abort if out of memory.
void *strong_malloc(size_t size) __attribute__((__malloc__, __alloc_size__(1)));
// Strong calloc; abort if out of memory.
void *strong_calloc(size_t nmemb, size_t size) __attribute__((__malloc__, __alloc_size__(1, 2)));
// Strong realloc; abort if out of memory.
void *strong_realloc(void *ptr, size_t size) __attribute__((__warn_unused_result__, __alloc_size__(2)));
// Strong stdup; abort if out of memory.
char *strong_strdup(char const *str) __attribute__((__malloc__, __nonnull__(1)));
