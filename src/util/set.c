
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "set.h"

#include <stdlib.h>
#include <string.h>



// Get the hash of a value.
uint32_t set_hash_value(char const *value) {
    // This isn't much of a well-designed hash function but at least it works.
    uint32_t hash = 0xa81f208a;
    while (*value) {
        hash *= 1346882387;
        hash += *value * 2281519393;
        value++;
    }
    return hash;
}


// Remove all entries from a set.
void set_clear(set_t *set) {
    for (size_t i = 0; i < SET_BUCKETS; i++) {
        dlist_node_t *node = dlist_pop_front(&set->buckets[i]);
        while (node) {
            set_ent_t *ent = (set_ent_t *)node;
            free(ent->value);
            free(ent);
            node = dlist_pop_front(&set->buckets[i]);
        }
    }
}

// Get an item from the set.
bool set_contains(set_t const *set, char const *value) {
    // Figure out which bucket the value is in.
    uint32_t hash   = set_hash_value(value);
    size_t   bucket = hash % SET_BUCKETS;

    // Walk the list of items in this bucket.
    dlist_node_t *node = set->buckets[bucket].head;
    while (node) {
        set_ent_t *ent = (set_ent_t *)node;
        // Both hash and string compare must be equal.
        if (ent->hash == hash && !strcmp(ent->value, value)) {
            return true;
        }
        // Go to the next item in the bucket.
        node = node->next;
    }

    // The bucket did not contain the value.
    return false;
}

// Insert an item into the set.
bool set_add(set_t *set, char const *value) {
    // Figure out which bucket the value is in.
    uint32_t hash   = set_hash_value(value);
    size_t   bucket = hash % SET_BUCKETS;

    // Walk the list of items in this bucket.
    dlist_node_t *node = set->buckets[bucket].head;
    while (node) {
        set_ent_t *ent = (set_ent_t *)node;
        // Both hash and string compare must be equal.
        if (ent->hash == hash && !strcmp(ent->value, value)) {
            // There is an existing value.
            return false;
        }
        node = node->next;
    }

    // Allocate a new item.
    set_ent_t *ent = malloc(sizeof(set_ent_t));
    if (!ent) {
        return false;
    }
    ent->value = strdup(value);
    if (!ent->value) {
        free(ent);
        return false;
    }
    ent->value = value;
    ent->hash  = hash;

    // Add the new item to the bucket.
    dlist_append(&set->buckets[bucket], &ent->node);
    set->len++;
    return true;
}

// Remove an item from the set.
bool set_remove(set_t *set, char const *value) {
    // Figure out which bucket the value is in.
    uint32_t hash   = set_hash_value(value);
    size_t   bucket = hash % SET_BUCKETS;

    // Walk the list of items in this bucket.
    dlist_node_t *node = set->buckets[bucket].head;
    while (node) {
        set_ent_t *ent = (set_ent_t *)node;
        // Both hash and string compare must be equal.
        if (ent->hash == hash && !strcmp(ent->value, value)) {
            // There is an existing value; remove it.
            dlist_remove(&set->buckets[bucket], node);
            free(ent->value);
            free(ent);
            return true;
        }
        node = node->next;
    }

    return false;
}

// Get next item in the set (or first if `ent` is NULL).
set_ent_t const *set_next(set_t const *set, set_ent_t const *ent) {
    size_t bucket;
    if (!ent) {
        bucket = 0;
    } else if (ent->node.next) {
        return (set_ent_t const *)ent->node.next;
    } else {
        bucket = ent->hash % SET_BUCKETS + 1;
    }
    while (bucket < SET_BUCKETS) {
        if (set->buckets[bucket].head) {
            return (set_ent_t const *)set->buckets[bucket].head;
        }
        bucket++;
    }
    return NULL;
}
