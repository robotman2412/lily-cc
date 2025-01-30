
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "list.h"



// Number of buckets in the hash map.
#define MAP_BUCKETS 16
// Create an empty hash map.
#define MAP_EMPTY   ((map_t){0})


// Hash map.
typedef struct map     map_t;
// Hash map entry.
typedef struct map_ent map_ent_t;


// Hash map.
struct map {
    // Hash buckets; array of linked list of map_ent_t.
    dlist_t buckets[MAP_BUCKETS];
    // Current number of elements.
    size_t  len;
};

// Hash map entry.
struct map_ent {
    // Linked list node.
    dlist_node_t node;
    // Hash of key.
    uint32_t     hash;
    // Key.
    char        *key;
    // Value.
    void        *value;
};



// Get the hash of a key.
uint32_t map_hash_key(char const *key);

// Remove all entries from a map.
void             map_clear(map_t *map);
// Get an item from the map.
void            *map_get(map_t const *map, char const *key) __attribute__((pure));
// Insert an item into the map.
bool             map_set(map_t *map, char const *key, void *value);
// Remove an item from the map.
bool             map_remove(map_t *map, char const *key);
// Get next item in the map (or first if `ent` is NULL).
map_ent_t const *map_next(map_t const *map, map_ent_t const *ent);
