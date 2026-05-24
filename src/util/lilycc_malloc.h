
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>



// Total size allocated through Lily-CC's allocator.
extern atomic_size_t lilycc_total_alloc;
// Allocation debug printing output.
extern FILE         *lilycc_alloc_debugfd;

// Lily-CC malloc; abort if out of memory.
void *lilycc_malloc(size_t size) __attribute__((malloc, warn_unused_result));
// Lily-CC calloc; abort if out of memory.
void *lilycc_calloc(size_t nmemb, size_t size) __attribute__((alloc_size(2, 1), warn_unused_result));
// Lily-CC realloc; abort if out of memory.
void *lilycc_realloc(void *ptr, size_t size) __attribute__((alloc_size(2), warn_unused_result));
// Lily-CC stdup; abort if out of memory.
char *lilycc_strdup(char const *str) __attribute__((warn_unused_result));
// Lily-CC free.
void  lilycc_free(void *mem);
