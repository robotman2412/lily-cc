
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "lilycc_malloc.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef NDEBUG
// This is a GNU header
#include <malloc.h>
#endif



// Total size allocated through Lily-CC's allocator.
atomic_size_t lilycc_total_alloc = 0;

// Strong malloc; abort if out of memory.
void *lilycc_malloc(size_t size) {
    void *mem = malloc(size);
    if (!mem && size) {
        fprintf(stderr, "Out of memory (allocating %zu byte%s)", size, size == 1 ? "" : "s");
        abort();
    }
#ifndef NDEBUG
    atomic_fetch_add_explicit(&lilycc_total_alloc, malloc_usable_size(mem), memory_order_relaxed);
    memset(mem, 0xCC, size);
#endif
    return mem;
}

// Strong calloc; abort if out of memory.
void *lilycc_calloc(size_t nmemb, size_t size) {
    void *mem = calloc(nmemb, size);
    if (!mem && nmemb && size) {
        fprintf(stderr, "Out of memory (allocating %zux%zu byte%s)", nmemb, size, nmemb == 1 ? "" : "s");
        abort();
    }
#ifndef NDEBUG
    atomic_fetch_add_explicit(&lilycc_total_alloc, malloc_usable_size(mem), memory_order_relaxed);
#endif
    return mem;
}

// Strong realloc; abort if out of memory.
void *lilycc_realloc(void *ptr, size_t size) {
#ifndef NDEBUG
    size_t oldsize = malloc_usable_size(ptr);
#endif
    void *mem = realloc(ptr, size);
    if (!mem && size) {
        fprintf(stderr, "Out of memory (allocating %zu byte%s)", size, size == 1 ? "" : "s");
        abort();
    }
#ifndef NDEBUG
    atomic_fetch_add_explicit(&lilycc_total_alloc, malloc_usable_size(mem) - oldsize, memory_order_relaxed);
#endif
    return mem;
}

// Strong stdup; abort if out of memory.
char *lilycc_strdup(char const *str) {
    char *mem = strdup(str);
    if (!mem) {
        size_t size = strlen(str) + 1;
        fprintf(stderr, "Out of memory (allocating %zu byte%s)", size, size == 1 ? "" : "s");
        abort();
    }
#ifndef NDEBUG
    atomic_fetch_add_explicit(&lilycc_total_alloc, malloc_usable_size(mem), memory_order_relaxed);
#endif
    return mem;
}

// Lily-CC free.
void lilycc_free(void *mem) {
#ifndef NDEBUG
    atomic_fetch_sub_explicit(&lilycc_total_alloc, malloc_usable_size(mem), memory_order_relaxed);
#endif
    free(mem);
}
