
#include "array_util.h"

// Underlying function that performs array binary search.
// Returns NULL on empty array or no exact match.
void *array_binary_search(size_t elem_size, void *array, size_t len, arr_cmp_t comparator, const void *find) {
	void *closest = array_binary_search_closest(elem_size, array, len, comparator, find);
	if (closest && !comparator(closest, find)) return closest;
	else return NULL;
}

// Underlying function that performs array binary search.
// Returns NULL on empty array, closest otherwise.
void *array_binary_search_closest(size_t elem_size, void *array, size_t len, arr_cmp_t comparator, const void *find) {
	// Edge case: Empty array.
	if (!len) return NULL;
	
	void *closest = NULL;
	while (len) {
		// Bisect the array.
		size_t center_idx = len / 2;
		void *center_addr = (void *) ((size_t) array + elem_size * center_idx);
		closest = center_addr;
		
		// Compare at this location.
		int res = comparator(find, center_addr);
		if (!res) return center_addr;
		
		// Pick which side to search next.
		void *next_array;
		size_t next_len;
		if (res > 0) {
			// Select right half.
			next_array = (void *) ((size_t) center_addr + elem_size);
			next_len   = len - center_idx - 1;
			
		} else /* res < 0 */ {
			// Select left half.
			next_array = array;
			next_len   = center_idx;
		}
		
		// Copy the indices for next loop.
		array = next_array;
		len   = next_len;
	}
	
	// No exact match, return closest.
	return closest;
}

