
#ifndef CTXALLOC_H
#define CTXALLOC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#ifdef CTXALLOC_C

struct alloc_bit;
struct alloc_ctx;

typedef struct alloc_bit  alloc_bit_t;
typedef struct alloc_ctx  alloc_ctx_s;
typedef struct alloc_ctx *alloc_ctx_t;

struct alloc_bit {
	uint64_t     magic1;
	alloc_ctx_t  owner;
	alloc_bit_t *prev;
	alloc_bit_t *next;
	uint64_t     magic2;
};

struct alloc_ctx {
	uint64_t     magic1;
	alloc_ctx_t  parent;
	alloc_bit_t *first;
	alloc_bit_t *last;
	uint64_t     magic2;
};

#else //CTXALLOC_C

typedef void *alloc_ctx_t;

#endif //CTXALLOC_C

extern alloc_ctx_t global_alloc;

#define ALLOC_NO_PARENT ((void *) 0)

// Initialises the alloc system thingy.
void        alloc_init    ();

// Creates a new memory allocation context.
alloc_ctx_t alloc_create  (alloc_ctx_t parent);
// Frees all memory of the context, recursively.
void        alloc_clear   (alloc_ctx_t ctx);
// Frees all memory of the context and destroys the context.
void        alloc_destroy (alloc_ctx_t ctx);

// Allocates memory belonging to a context.
void       *alloc_on_ctx  (alloc_ctx_t ctx, size_t size);
// Re-allocates memory belonging to a context.
void       *realloc_on_ctx(alloc_ctx_t ctx, void  *memory, size_t size);
// Frees memory belonging to a context.
void        free_on_ctx   (alloc_ctx_t ctx, void  *memory);

#ifndef CTXALLOC_C

// Allocates memory belonging to a context.
static inline void *xalloc(alloc_ctx_t ctx, size_t size) {
	return alloc_on_ctx(ctx, size);
}

// Re-allocates memory belonging to a context.
static inline void *xrealloc(alloc_ctx_t ctx, void *memory, size_t size) {
	return realloc_on_ctx(ctx, memory, size);
}

// Frees memory belonging to a context.
static inline void xfree(alloc_ctx_t ctx, void *memory) {
	free_on_ctx(ctx, memory);
}

// Strdup but with an allocator.
static inline char *xstrdup(alloc_ctx_t ctx, const char *memory) {
	char *newmem = alloc_on_ctx(ctx, strlen(memory) + 1);
	strcpy(newmem, memory);
	return newmem;
}

#endif //CTXALLOC_C

#endif //CTXALLOC_H
