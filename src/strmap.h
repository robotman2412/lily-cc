
#ifndef STRMAP_H
#define STRMAP_H

#include <stdint.h>
#include <stddef.h>

typedef struct map {
	size_t numEntries;
	size_t capacity;
	char **strings;
	void **values;
} map_t;

#define MAP_DEFAULT_CAPACITY 4
#define MAP_CAPACITY_INCREMENT 4

// Creates an empty map.
void map_create(map_t *map);

// Deletes a map.
void map_delete(map_t *map);

// Deletes a map and every value.
void map_delete_with_values(map_t *map);

// Gets key from map.
// Returns null if no such key.
void *map_get(map_t *map, char *key);

// Puts val in map at key.
// Providing null for val removes the item.
// Returns null or replaced item.
// Does not copy the provided item.
void *map_set(map_t *map, char *key, void *val);

// Removes key from map.
// Returns null or removed item.
void *map_remove(map_t *map, char *key);

// Returns the number of keys in map.
#define map_size(map) (map->numEntries)

#endif // STRMAP_H
