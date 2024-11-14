
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "map.h"

#include <stdlib.h>
#include <string.h>



// Get the hash of a key.
uint32_t map_hash_key(char const *key) {
    // This isn't much of a well-designed hash function but at least it works.
    uint32_t hash = 0xa81f208a;
    while (*key) {
        hash *= 1346882387;
        hash += *key * 2281519393;
        key++;
    }
    return hash;
}


// Remove all entries from a map.
void map_clear(map_t *map) {
    for (size_t i = 0; i < MAP_BUCKETS; i++) {
        dlist_node_t *node = dlist_pop_front(&map->buckets[i]);
        while (node) {
            map_ent_t *ent = (map_ent_t *)node;
            free(ent->key);
            free(ent);
            node = dlist_pop_front(&map->buckets[i]);
        }
    }
}

// Get an item from the map.
void *map_get(map_t const *map, char const *key) {
    // Figure out which bucket the key is in.
    uint32_t hash   = map_hash_key(key);
    size_t   bucket = hash % MAP_BUCKETS;

    // Walk the list of items in this bucket.
    dlist_node_t *node = map->buckets[bucket].head;
    while (node) {
        map_ent_t *ent = (map_ent_t *)node;
        // Both hash and string compare must be equal.
        if (ent->hash == hash && !strcmp(ent->key, key)) {
            return ent->value;
        }
        // Go to the next item in the bucket.
        node = node->next;
    }

    // The bucket did not contain the key.
    return NULL;
}

// Insert an item into the map.
bool map_set(map_t *map, char const *key, void *value) {
    // Figure out which bucket the key is in.
    uint32_t hash   = map_hash_key(key);
    size_t   bucket = hash % MAP_BUCKETS;

    // Walk the list of items in this bucket.
    dlist_node_t *node = map->buckets[bucket].head;
    while (node) {
        map_ent_t *ent = (map_ent_t *)node;
        // Both hash and string compare must be equal.
        if (ent->hash == hash && !strcmp(ent->key, key)) {
            if (value) {
                // Overwrite existing value.
                ent->value = value;
            } else {
                // Remove existing value.
                dlist_remove(&map->buckets[bucket], node);
                free(ent->key);
                free(ent);
                map->len--;
            }
            // Successfully set the item.
            return true;
        }
        node = node->next;
    }

    if (!value) {
        // No item to be removed.
        return false;
    }

    // Allocate a new item.
    map_ent_t *ent = malloc(sizeof(map_ent_t));
    if (!ent) {
        return false;
    }
    ent->key = strdup(key);
    if (!ent->key) {
        free(ent);
        return false;
    }
    ent->value = value;
    ent->hash  = hash;

    // Add the new item to the bucket.
    dlist_append(&map->buckets[bucket], &ent->node);
    map->len++;
    return true;
}

// Remove an item from the map.
bool map_remove(map_t *map, char const *key) {
    return map_set(map, key, NULL);
}

// Get next item in the map (or first if `ent` is NULL).
map_ent_t const *map_next(map_t const *map, map_ent_t const *ent) {
    size_t bucket;
    if (!ent) {
        bucket = 0;
    } else if (ent->node.next) {
        return (map_ent_t const *)ent->node.next;
    } else {
        bucket = ent->hash % MAP_BUCKETS + 1;
    }
    while (bucket < MAP_BUCKETS) {
        if (map->buckets[bucket].head) {
            return (map_ent_t const *)map->buckets[bucket].head;
        }
        bucket++;
    }
    return NULL;
}
