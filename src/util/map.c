
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "map.h"

#include <stdlib.h>
#include <string.h>



// Remove all entries from a map.
void map_clear(map_t *map) {
    for (size_t i = 0; i < MAP_BUCKETS; i++) {
        dlist_node_t *node = dlist_pop_front(&map->buckets[i]);
        while (node) {
            map_ent_t *ent = (map_ent_t *)node;
            map->key_del(ent->key);
            free(ent);
            node = dlist_pop_front(&map->buckets[i]);
        }
    }
}

// Get an item from the map.
void *map_get(map_t const *map, void const *key) {
    // Figure out which bucket the key is in.
    uint32_t hash   = map->key_hash(key);
    size_t   bucket = hash % MAP_BUCKETS;

    // Walk the list of items in this bucket.
    dlist_node_t *node = map->buckets[bucket].head;
    while (node) {
        map_ent_t *ent = (map_ent_t *)node;
        // Both hash and string compare must be equal.
        if (ent->hash == hash && !map->key_cmp(ent->key, key)) {
            return ent->value;
        }
        // Go to the next item in the bucket.
        node = node->next;
    }

    // The bucket did not contain the key.
    return NULL;
}

// Insert an item into the map.
bool map_set(map_t *map, void const *key, void const *value) {
    // Figure out which bucket the key is in.
    uint32_t hash   = map->key_hash(key);
    size_t   bucket = hash % MAP_BUCKETS;

    // Walk the list of items in this bucket.
    dlist_node_t *node = map->buckets[bucket].head;
    while (node) {
        map_ent_t *ent = (map_ent_t *)node;
        // Both hash and string compare must be equal.
        if (ent->hash == hash && !map->key_cmp(ent->key, key)) {
            if (value) {
                // Overwrite existing value.
                ent->value = (void *)value;
            } else {
                // Remove existing value.
                dlist_remove(&map->buckets[bucket], node);
                map->key_del(ent->key);
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
    ent->node = DLIST_NODE_EMPTY;
    ent->key  = map->key_dup(key);
    if (!ent->key) {
        free(ent);
        return false;
    }
    ent->value = (void *)value;
    ent->hash  = hash;

    // Add the new item to the bucket.
    dlist_append(&map->buckets[bucket], &ent->node);
    map->len++;
    return true;
}

// Remove an item from the map.
bool map_remove(map_t *map, void const *key) {
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
