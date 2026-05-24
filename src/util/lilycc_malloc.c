
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
atomic_size_t lilycc_total_alloc   = 0;
// Allocation debug printing output.
FILE         *lilycc_alloc_debugfd = NULL;

// Strong malloc; abort if out of memory.
void *lilycc_malloc(size_t size) {
    void *mem = malloc(size);
    if (!mem && size) {
        fprintf(stderr, "Out of memory (allocating %8zu byte%s)", size, size == 1 ? "" : "s");
        abort();
    }
    size_t realsize = malloc_usable_size(mem);
    atomic_fetch_add_explicit(&lilycc_total_alloc, realsize, memory_order_relaxed);
#ifndef NDEBUG
    memset(mem, 0xCC, realsize);
#endif
    if (lilycc_alloc_debugfd) {
        fprintf(
            lilycc_alloc_debugfd,
            "lilycc_malloc (%8zu)                     -> 0x%016zx (+%8zu bytes)\n",
            size,
            (size_t)mem,
            realsize
        );
    }
    return mem;
}

// Strong calloc; abort if out of memory.
void *lilycc_calloc(size_t nmemb, size_t size) {
    void *mem = calloc(nmemb, size);
    if (!mem && nmemb && size) {
        fprintf(stderr, "Out of memory (allocating %8zux%8zu byte%s)", nmemb, size, nmemb == 1 ? "" : "s");
        abort();
    }
    size_t realsize = malloc_usable_size(mem);
    atomic_fetch_add_explicit(&lilycc_total_alloc, realsize, memory_order_relaxed);
    if (lilycc_alloc_debugfd) {
        fprintf(
            lilycc_alloc_debugfd,
            "lilycc_calloc (%8zu, %8zu)           -> 0x%016zx (+%8zu bytes)\n",
            nmemb,
            size,
            (size_t)mem,
            realsize
        );
    }
    return mem;
}

// Strong realloc; abort if out of memory.
void *lilycc_realloc(void *ptr, size_t size) {
    size_t oldsize = malloc_usable_size(ptr);
    void  *mem     = realloc(ptr, size);
    if (!mem && size) {
        fprintf(stderr, "Out of memory (allocating %8zu byte%s)", size, size == 1 ? "" : "s");
        abort();
    }
    size_t newsize = mem ? malloc_usable_size(mem) : 0;
    atomic_fetch_add_explicit(&lilycc_total_alloc, newsize - oldsize, memory_order_relaxed);
    if (lilycc_alloc_debugfd) {
        // GCC warns for use-after-free here, but all we're doing is printing the address of the pointer.
#pragma GCC diagnostic push
#if !defined __clang__ && !defined __LILYC__
#pragma GCC diagnostic ignored "-Wuse-after-free"
#endif
        fprintf(
            lilycc_alloc_debugfd,
            "lilycc_realloc(0x%016zx, %8zu) -> 0x%016zx (%c%8zd bytes)\n",
            (size_t)ptr,
            size,
            (size_t)mem,
            newsize < oldsize ? '-' : '+',
            newsize < oldsize ? oldsize - newsize : newsize - oldsize
        );
#pragma GCC diagnostic pop
    }
    return mem;
}

// Strong stdup; abort if out of memory.
char *lilycc_strdup(char const *str) {
    char *mem = strdup(str);
    if (!mem) {
        size_t size = strlen(str) + 1;
        fprintf(stderr, "Out of memory (allocating %8zu byte%s)", size, size == 1 ? "" : "s");
        abort();
    }
    size_t realsize = malloc_usable_size(mem);
    atomic_fetch_add_explicit(&lilycc_total_alloc, realsize, memory_order_relaxed);
    if (lilycc_alloc_debugfd) {
        fprintf(
            lilycc_alloc_debugfd,
            "lilycc_strdup (0x%016zx)           -> 0x%016zx (+%8zu bytes)\n",
            (size_t)str,
            (size_t)mem,
            realsize
        );
    }
    return mem;
}

// Lily-CC free.
void lilycc_free(void *mem) {
    size_t realsize = malloc_usable_size(mem);
    atomic_fetch_sub_explicit(&lilycc_total_alloc, realsize, memory_order_relaxed);
#ifndef NDEBUG
    memset(mem, 0xCC, realsize);
#endif
    if (lilycc_alloc_debugfd) {
        fprintf(
            lilycc_alloc_debugfd,
            "lilycc_free   (0x%016zx)                                 (-%8zu bytes)\n",
            (size_t)mem,
            realsize
        );
    }
    free(mem);
}
