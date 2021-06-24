
#include <strmap.h>
#include <malloc.h>
#include <string.h>

// Creates an empty map.
void map_create(map_t *map) {
	map->numEntries = 0;
	map->capacity = MAP_DEFAULT_CAPACITY;
	map->strings = (char **) malloc(sizeof(char **) * map->capacity);
	map->values = (void *) malloc(sizeof(void *) * map->capacity);
}

// Deletes a map.
void map_delete(map_t *map) {
	map->numEntries = 0;
	map->capacity = 0;
	free(map->strings);
	free(map->values);
}

// Deletes a map and every value.
void map_delete_with_values(map_t *map) {
	for (int i = 0; i < map->numEntries; i++) {
		free(map->values[i]);
	}
	map_delete(map);
}

// Finds key in map.
// Returns -1 if not found.
static inline int map_lkup(map_t *map, char *key) {
	for (int i = 0; i < map->numEntries; i++) {
		if (!strcmp(map->strings[i], key)) {
			return i;
		}
	}
	return -1;
}

// Gets key from map.
// Returns null if no such key.
void *map_get(map_t *map, char *key) {
	int i = map_lkup(map, key);
	if (i >= 0) {
		return map->values[i];
	} else {
		return 0;
	}
}

// Puts val in map at key.
// Providing null for val removes the item.
// Returns null or replaced item.
// Will copy the provided string.
// Will NOT copy the provided item.
void *map_set(map_t *map, char *key, void *val) {
	if (!val) return map_remove(map, key);
	int i = map_lkup(map, key);
	if (i >= 0) {
		void *ret = map->values[i];
		map->values[i] = val;
		return ret;
	} else {
		if (map->numEntries >= map->capacity) {
			map->capacity += MAP_CAPACITY_INCREMENT;
			map->strings = realloc(map->strings, sizeof(char *) * map->capacity);
			map->values = realloc(map->values, sizeof(void *) * map->capacity);
		}
		map->strings[map->numEntries] = strdup(key);
		map->values[map->numEntries] = val;
		map->numEntries ++;
		return NULL;
	}
}

// Removes key from map.
// Returns null or removed item.
void *map_remove(map_t *map, char *key) {
	int i = map_lkup(map, key);
	if (i >= 0) {
		free(map->strings[i]);
		void *ret = map->values[i];
		map->numEntries --;
		if (i != map->numEntries) {
			map->strings[i] = map->strings[map->numEntries];
			map->values[i] = map->values[map->numEntries];
		}
		return ret;
	} else {
		return NULL;
	}
}
