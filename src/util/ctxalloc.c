
#define CTXALLOC_C

#include "ctxalloc.h"
#include <malloc.h>
#include <stdlib.h>

#define ALLOC_BIT_MAGIC1 0x536018e5280f35adLLU
#define ALLOC_BIT_MAGIC2 0x4839678fcf3d0596LLU
#define ALLOC_CTX_MAGIC1 0x9cc1c4fd0e25a9d8LLU
#define ALLOC_CTX_MAGIC2 0x40ec817d60963a2dLLU

alloc_ctx_t global_ctx = NULL;

// Checks the magic values for a alloc bit.
static inline bool alloc_bit_magic_check(alloc_bit_t *bit) {
	return bit->magic1 == ALLOC_BIT_MAGIC1 && bit->magic2 == ALLOC_BIT_MAGIC2;
}

// Checks the magic values for a context.
static inline bool alloc_ctx_magic_check(alloc_ctx_t ctx) {
	return ctx->magic1 == ALLOC_CTX_MAGIC1 && ctx->magic2 == ALLOC_CTX_MAGIC2;
}

#define ALLOC_BIT_MAGIC_ASSERT(bit) do {\
		alloc_bit_t *p = (bit);\
		if (!p || !alloc_bit_magic_check(p)) {\
			fflush(stdout);\
			fprintf(stderr, "\033[1m%s:%d: \033[91mfatal error:\033[0m Allocated memory and/or pointer corruption (%p)\n", __FILE__, __LINE__, p);\
			fprintf(stderr, "\033[1;91mAborting!\n");\
			fflush(stderr);\
			abort();\
		}\
	} while(0)

#define ALLOC_CTX_MAGIC_ASSERT(ctx) do {\
		alloc_ctx_t p = (ctx);\
		if (!p || !alloc_ctx_magic_check(p)) {\
			fflush(stdout);\
			fprintf(stderr, "\033[1m%s:%d: \033[91mfatal error:\033[0m Allocator context and/or pointer corruption (%p)\n", __FILE__, __LINE__, p);\
			fprintf(stderr, "\033[1;91mAborting!\n");\
			fflush(stderr);\
			abort();\
		}\
	} while(0)

#define ALLOC_BIT_OWNER_ASSERT(bit, ctx) do {\
		alloc_bit_t *p = (bit);\
		alloc_ctx_t  q = (ctx);\
		if (p->owner != q) {\
			fflush(stdout);\
			fprintf(stderr, "\033[1m%s:%d: \033[91mfatal error:\033[0m Allocated memory does not belong to given context (%p to %p)\n", __FILE__, __LINE__, p, q);\
			fprintf(stderr, "\033[1;91mAborting!\n");\
			fflush(stderr);\
			abort();\
		}\
	} while(0)


// Initialises the alloc system thingy.
void alloc_init() {
	global_ctx = alloc_create(ALLOC_NO_PARENT);
}


// Creates a new memory allocation context.
alloc_ctx_t alloc_create(alloc_ctx_t parent) {
	// Assert the parent is valid.
	if (parent) ALLOC_CTX_MAGIC_ASSERT(parent);
	
	// Create a new struct.
	alloc_ctx_t ctx = malloc(sizeof(alloc_ctx_s));
	*ctx = (alloc_ctx_s) {
		.magic1 = ALLOC_CTX_MAGIC1,
		.parent = parent,
		.first  = NULL,
		.magic2 = ALLOC_CTX_MAGIC2,
	};
	return ctx;
}

// Frees all memory of the context, recursively.
void alloc_clear(alloc_ctx_t ctx) {
	// Assert the context is valid.
	ALLOC_CTX_MAGIC_ASSERT(ctx);
	
	// Iterate over ALL the things.
	alloc_bit_t *bit = ctx->first;
	while (bit) {
		void *mem = bit;
		bit->magic1 = 0;
		bit->magic2 = 0;
		bit = bit->next;
		free(mem);
	}
	ctx->first = NULL;
	ctx->last  = NULL;
}

// Frees all memory of the context and destroys the context.
void alloc_destroy(alloc_ctx_t ctx) {
	// Assert the context is valid.
	ALLOC_CTX_MAGIC_ASSERT(ctx);
	// Clear out the context.
	alloc_clear(ctx);
	ctx->magic1 = 0;
	ctx->magic2 = 0;
	// Free the memory.
	free(ctx);
}


// Allocates memory belonging to a context.
void *alloc_on_ctx(alloc_ctx_t ctx, size_t size) {
	// Assert the context is valid.
	ALLOC_CTX_MAGIC_ASSERT(ctx);
	
	// Try to get some memory, yes?
	void *newmem = malloc(sizeof(alloc_bit_t) + size);
	if (!newmem) return NULL;
	
	// Insert the bit data.
	alloc_bit_t *bit = newmem;
	void        *ptr = (void *) ((size_t) newmem + sizeof(alloc_bit_t));
	*bit = (alloc_bit_t) {
		.magic1 = ALLOC_BIT_MAGIC1,
		.owner  = ctx,
		.magic2 = ALLOC_BIT_MAGIC2,
	};
	
	// Set up next and prev pointers.
	if (!ctx->first) {
		ctx->first      = bit;
		bit->prev       = NULL;
	}
	if (!ctx->last) {
		bit->next       = NULL;
	} else {
		ctx->last->next = bit;
		bit->prev       = ctx->last;
	}
	ctx->last       = bit;
	
	// Return the allocated memory.
	return ptr;
}

// Re-allocates memory belonging to a context.
void *realloc_on_ctx(alloc_ctx_t ctx, void *memory, size_t size) {
	// Assert the context is valid.
	ALLOC_CTX_MAGIC_ASSERT(ctx);
	
	if (!size) {
		// If size is zero then free instead.
		free_on_ctx(ctx, memory);
		return NULL;
	} else if (!memory) {
		// If memory is NULL then allocate instead.
		return alloc_on_ctx(ctx, size);
	}
	
	// Magic checks.
	void *realmem = (void *) ((size_t) memory - sizeof(alloc_bit_t));
	ALLOC_BIT_MAGIC_ASSERT((alloc_bit_t *) realmem);
	// Assert the bit is owned by the given context.
	ALLOC_BIT_OWNER_ASSERT((alloc_bit_t *) realmem, ctx);
	// Re-allocate the memory.
	void *newmem = realloc(realmem, size + sizeof(alloc_bit_t));
	if (!newmem) {
		// No extra memory for you!
		// Pointers remain valid.
		return NULL;
	} else if (newmem == realmem) {
		// No need to fix pointers.
		return newmem;
	}
	
	// Fix next and prev pointers.
	alloc_bit_t *bit = newmem;
	if (bit->prev) {
		bit->prev->next = bit;
	} else {
		ctx->first = bit;
	}
	if (bit->next) {
		bit->next->prev = bit;
	} else {
		ctx->last = bit;
	}
	
	// Return usable memory.
	void *ptr = (void *) ((size_t) newmem + sizeof(alloc_bit_t));
	return ptr;
}

// Re-allocates memory belonging to a context.
void free_on_ctx(alloc_ctx_t ctx, void *memory) {
	// Assert the context is valid.
	ALLOC_CTX_MAGIC_ASSERT(ctx);
	
	// Ignore free of null.
	if (!memory) return;
	
	// Check magic values.
	void *realmem = (void *) ((size_t) memory - sizeof(alloc_bit_t));
	ALLOC_BIT_MAGIC_ASSERT((alloc_bit_t *) realmem);
	alloc_bit_t *bit = realmem;
	// Assert the bit is owned by the given context.
	ALLOC_BIT_OWNER_ASSERT(bit, ctx);
	
	// Unlink the bit.
	if (bit->prev) {
		bit->prev->next = bit->next;
	} else {
		ctx->first = bit->next;
	}
	if (bit->next) {
		bit->next->prev = bit->prev;
	} else {
		ctx->last = bit->prev;
	}
	
	// Protect against double free.
	bit->magic1 = 0;
	bit->magic2 = 0;
	
	// Free the memory.
	free(realmem);
}
