
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <ctxalloc.h>

// Concatenate an element to a generic array.
// This macro assumes `arrayVar` is a pointer of `Type`, and can be written do by `arrayVar = ...`.
// Returns true on success.
#define array_len_cap_concat(allocator, Type, arrayVar, capVar, lenVar, insertVar) do { \
		/* Make capacity */ \
		if (capVar <= lenVar) { \
			capVar *= 2; \
			Type *memory = xrealloc(allocator, arrayVar, sizeof(Type) * capVar); \
			if (!memory) {printf("Out of memory\n"); abort();} \
			arrayVar = memory; \
		} \
		\
		/* Insert element */ \
		arrayVar[lenVar++] = insertVar; \
	} while(0)
