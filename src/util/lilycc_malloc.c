
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "lilycc_malloc.h"

#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined NDEBUG && defined __GNUC__
#include <stdbool.h>
#include <stdint.h>



// Total size allocated through Lily-CC's allocator.
atomic_size_t lilycc_total_alloc   = 0;
// Allocation debug printing output.
FILE         *lilycc_alloc_debugfd = NULL;

static bool alloc_instrument = true;



__attribute__((constructor)) static void init_alloc_instrument() {
    char const *preload = getenv("LD_PRELOAD");
    if (preload) {
        alloc_instrument = !(strstr(preload, "valgrind") || strstr(preload, "vgpreload"));
    }
}

typedef struct alloc_hdr alloc_hdr_t;

struct __attribute__((aligned(16))) alloc_hdr {
    size_t size;
};

static void *wrap_malloc(size_t size) {
    if (!alloc_instrument) {
        return malloc(size);
    }
    if (!size) {
        return NULL;
    }

    alloc_hdr_t *hdr = malloc(sizeof(alloc_hdr_t) + size);
    if (!hdr) {
        return NULL;
    }

    hdr->size           = size;
    lilycc_total_alloc += size;
    memset(hdr + 1, 0xcc, size);

    return hdr + 1;
}

static void wrap_free(void *ptr) {
    if (!alloc_instrument) {
        free(ptr);
        return;
    }

    if (!ptr) {
        return;
    }

    alloc_hdr_t *hdr = (alloc_hdr_t *)ptr - 1;

    lilycc_total_alloc -= hdr->size;
    free(hdr);
}

static void *wrap_realloc(void *ptr, size_t newsize) {
    if (!alloc_instrument) {
        return realloc(ptr, newsize);
    }
    if (!ptr) {
        return wrap_malloc(newsize);
    } else if (!newsize) {
        wrap_free(ptr);
        return NULL;
    }

    alloc_hdr_t *hdr = (alloc_hdr_t *)ptr - 1;

    if (newsize < hdr->size) {
        // memset((uint8_t *)(hdr + 1) + newsize, 0xcc, hdr->size - newsize);
    }

    alloc_hdr_t *mem = realloc(hdr, sizeof(alloc_hdr_t) + newsize);
    if (!mem) {
        return NULL;
    }

    if (newsize > mem->size) {
        // memset((uint8_t *)(mem + 1) + mem->size, 0xcc, newsize - mem->size);
    }

    lilycc_total_alloc -= mem->size;
    lilycc_total_alloc += newsize;
    mem->size           = newsize;

    return mem + 1;
}

static void *wrap_calloc(size_t nmemb, size_t size) {
    if (!alloc_instrument) {
        return calloc(nmemb, size);
    }
    if (!nmemb || !size) {
        return NULL;
    }
    size_t total;
    if (__builtin_mul_overflow(nmemb, size, &total)) {
        return NULL;
    }
    void *mem = wrap_malloc(total);
    if (mem) {
        memset(mem, 0, total);
    }
    return mem;
}

static char *wrap_strdup(char const *str) {
    if (!alloc_instrument) {
        return strdup(str);
    }

    size_t len = strlen(str);
    char  *mem = wrap_malloc(len + 1);
    if (mem) {
        memcpy(mem, str, len + 1);
    }

    return mem;
}

#else
#define alloc_instrument 0
#define wrap_malloc      malloc
#define wrap_calloc      calloc
#define wrap_realloc     realloc
#define wrap_strdup      strdup
#endif

// Strong malloc; abort if out of memory.
void *lilycc_malloc(size_t size) {
    void *mem = wrap_malloc(size);
    if (!mem && size) {
        fprintf(stderr, "Out of memory (allocating %8zu byte%s)", size, size == 1 ? "" : "s");
        abort();
    }
    return mem;
}

// Strong calloc; abort if out of memory.
void *lilycc_calloc(size_t nmemb, size_t size) {
    void *mem = wrap_calloc(nmemb, size);
    if (!mem && nmemb && size) {
        fprintf(stderr, "Out of memory (allocating %8zux%8zu byte%s)", nmemb, size, nmemb == 1 ? "" : "s");
        abort();
    }
    return mem;
}

// Strong realloc; abort if out of memory.
void *lilycc_realloc(void *ptr, size_t size) {
    void *mem = wrap_realloc(ptr, size);
    if (!mem && size) {
        fprintf(stderr, "Out of memory (allocating %8zu byte%s)", size, size == 1 ? "" : "s");
        abort();
    }
    return mem;
}

// Strong stdup; abort if out of memory.
char *lilycc_strdup(char const *str) {
    if (!str) {
        return NULL;
    }
    char *mem = wrap_strdup(str);
    if (!mem) {
        size_t size = strlen(str) + 1;
        fprintf(stderr, "Out of memory (allocating %8zu byte%s)", size, size == 1 ? "" : "s");
        abort();
    }
    return mem;
}

// Lily-CC free.
void lilycc_free(void *mem) {
    wrap_free(mem);
}
