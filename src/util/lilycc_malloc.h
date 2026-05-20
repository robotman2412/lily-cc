
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include <stdatomic.h>
#include <stdlib.h>



#ifndef NDEBUG
// Total size allocated through Lily-CC's allocator.
extern atomic_size_t lilycc_total_alloc;
#endif

// Lily-CC malloc; abort if out of memory.
void *lilycc_malloc(size_t size);
// Lily-CC calloc; abort if out of memory.
void *lilycc_calloc(size_t nmemb, size_t size);
// Lily-CC realloc; abort if out of memory.
void *lilycc_realloc(void *ptr, size_t size);
// Lily-CC stdup; abort if out of memory.
char *lilycc_strdup(char const *str);
// Lily-CC free.
void  lilycc_free(void *mem);
