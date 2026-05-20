
// Copyright © 2026, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "vec.h"

#include "strong_malloc.h"

#include <stdio.h>
#include <stdlib.h>

// Reserve space for at least N additional elements.
void rawvec_reserve(rawvec_t *vec, size_t elem_size, size_t n_elem) {
    if (n_elem + vec->len < n_elem) {
        fprintf(stderr, "Vector size overflow\n");
        abort();
    }
    n_elem += vec->len;
    if (vec->cap >= n_elem) {
        return;
    }
    size_t cap = vec->cap;
    if (cap == 0) {
        cap = 1;
    }
    while (cap < n_elem) {
        if (cap * 2 < cap) {
            fprintf(stderr, "Vector size overflow\n");
            abort();
        }
        cap *= 2;
    }
    size_t bytes = elem_size * cap;
    if (bytes / elem_size != cap) {
        fprintf(stderr, "Vector size overflow\n");
        abort();
    }
    vec->buffer = strong_realloc(vec->buffer, bytes);
    vec->cap    = cap;
}

// Reserve space for exactly N additional elements.
void rawvec_reserve_exact(rawvec_t *vec, size_t elem_size, size_t n_elem) {
    if (n_elem + vec->len < n_elem) {
        fprintf(stderr, "Vector size overflow\n");
        abort();
    }
    n_elem += vec->len;
    if (vec->cap >= n_elem) {
        return;
    }
    size_t bytes = elem_size * n_elem;
    if (bytes / elem_size != n_elem) {
        fprintf(stderr, "Vector size overflow\n");
        abort();
    }
    vec->buffer = strong_realloc(vec->buffer, bytes);
    vec->cap    = n_elem;
}

// Insert an element into the vector.
void rawvec_insert_n(rawvec_t *vec, size_t elem_size, size_t index, void const *elem, size_t count) {
    if (index > vec->len) {
        fprintf(stderr, "Vector insert index out of bounds\n");
        abort();
    }
    rawvec_reserve(vec, elem_size, count);
    if (index < vec->len) {
        memmove(
            vec->buffer + (index + count) * elem_size,
            vec->buffer + index * elem_size,
            (vec->len - index) * elem_size
        );
    }
    memcpy(vec->buffer + index * elem_size, elem, elem_size * count);
    vec->len += count;
}

// Remove elements from the vector.
void rawvec_remove_n(rawvec_t *vec, size_t elem_size, size_t index, size_t count, void *arr_out_opt) {
    if (vec->len < count) {
        fprintf(stderr, "Attempt to remove too many elements from vector\n");
        abort();
    }
    if (index > vec->len - count) {
        fprintf(stderr, "Vector remove index out of bounds\n");
        abort();
    }
    vec->len -= count;
    if (arr_out_opt) {
        memcpy(arr_out_opt, vec->buffer + index * elem_size, elem_size * count);
    }
    if (index < vec->len) {
        memmove(
            vec->buffer + index * elem_size,
            vec->buffer + (index + count) * elem_size,
            (vec->len - index) * elem_size
        );
    }
    if (vec->len == 0) {
        free(vec->buffer);
        vec->buffer = NULL;
        vec->cap    = 0;
    }
}
