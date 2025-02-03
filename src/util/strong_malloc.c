
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "strong_malloc.h"

#include <stdlib.h>
#include <string.h>



// Strong malloc; abort if out of memory.
void *strong_malloc(size_t size) {
    void *mem = malloc(size);
    if (!mem && size) {
        fprintf(stderr, "Out of memory (allocating %zu byte%s)", size, size == 1 ? "" : "s");
        abort();
    }
    return mem;
}

// Strong calloc; abort if out of memory.
void *strong_calloc(size_t size, size_t count) {
    void *mem = calloc(size, count);
    if (!mem && (size || count)) {
        fprintf(stderr, "Out of memory (allocating %zux%zu byte%s)", size, count, size == 1 ? "" : "s");
        abort();
    }
    return mem;
}

// Strong realloc; abort if out of memory.
void *strong_realloc(void *ptr, size_t size) {
    void *mem = realloc(ptr, size);
    if (!mem && size) {
        fprintf(stderr, "Out of memory (allocating %zu byte%s)", size, size == 1 ? "" : "s");
        abort();
    }
    return mem;
}

// Strong stdup; abort if out of memory.
char *strong_strdup(char const *str) {
    char *mem = strdup(str);
    if (!mem) {
        size_t size = strlen(str) + 1;
        fprintf(stderr, "Out of memory (allocating %zu byte%s)", size, size == 1 ? "" : "s");
        abort();
    }
    return mem;
}
