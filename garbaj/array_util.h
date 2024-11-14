
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctxalloc.h>

// Comparator function for two elements.
// Used in binary search.
typedef int (*arr_cmp_t)(const void *, const void *);

// Concatenate an element to a generic array.
// This macro assumes `arrayVar` is a pointer of `Type`, and can be written do by `arrayVar = ...`.
#define array_len_cap_concat(allocator, Type, arrayVar, capVar, lenVar, insertVar) do { \
		/* Make capacity */ \
		if (capVar <= lenVar) { \
			capVar *= 2; \
			if (capVar < 2) capVar = 2; \
			Type *memory = xrealloc(allocator, arrayVar, sizeof(Type) * capVar); \
			if (!memory) {printf("Out of memory\n"); abort();} \
			arrayVar = memory; \
		} \
		\
		/* Insert element */ \
		arrayVar[lenVar++] = insertVar; \
	} while(0)

// Concatenate an element to a generic array.
// This macro assumes `arrayVar` is a pointer of `Type`, and can be written do by `arrayVar = ...`.
#define array_len_concat(allocator, Type, arrayVar, lenVar, insertVar) do { \
		/* Make capacity */ \
		Type *memory = xrealloc(allocator, arrayVar, sizeof(Type) * (lenVar + 1)); \
		if (!memory) {printf("Out of memory\n"); abort();} \
		arrayVar = memory; \
		\
		/* Insert element */ \
		arrayVar[lenVar++] = insertVar; \
	} while(0)



// Underlying function that performs array binary search.
// Returns NULL on empty array or no exact match.
void *array_binary_search(size_t elem_size, void *array, size_t len, arr_cmp_t comparator, const void *find);
// Underlying function that performs array binary search.
// Returns NULL on empty array, closest otherwise.
void *array_binary_search_closest(size_t elem_size, void *array, size_t len, arr_cmp_t comparator, const void *find);

// Perform a binary search on a generic array.
// This macro assumes `array` is a pointer of `Type`.
// Returns NULL on empty array or no exact match.
#define array_find(Type, array, len, comparator, findVar) \
	({ Type tmp = (findVar); (Type *) array_binary_search(sizeof(Type), array, len, (arr_cmp_t) (comparator), &tmp); })

// Perform a binary search on a generic array.
// This macro assumes `array` is a pointer of `Type`.
// Returns a pointer to the closest match, or NULL for empty array.
#define array_find_closest(Type, array, len, comparator, findVar) \
	({ Type tmp = (findVar); (Type *) array_binary_search_closest(sizeof(Type), array, len, (arr_cmp_t) (comparator), &tmp); })
