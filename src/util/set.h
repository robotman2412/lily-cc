
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "list.h"



// Number of buckets in the hash set.
#define SET_BUCKETS 16
// Create an empty hash set.
#define SET_EMPTY   ((set_t){0})


// Hash set.
typedef struct set     set_t;
// Hash set entry.
typedef struct set_ent set_ent_t;



// Hash set.
struct set {
    // Hash buckets; array of linked list of set_ent_t.
    dlist_t buckets[SET_BUCKETS];
    // Current number of elements.
    size_t  len;
};

// Hash set entry.
struct set_ent {
    // Linked list node.
    dlist_node_t node;
    // Hash of value.
    uint32_t     hash;
    // Value.
    char        *value;
};



// Get the hash of a value.
uint32_t set_hash_value(char const *value);

// Remove all entries from a set.
void             set_clear(set_t *set);
// Test if an item is in the set.
bool             set_contains(set_t const *set, char const *value) __attribute__((pure));
// Insert an item into the set.
bool             set_add(set_t *set, char const *value);
// Remove an item from the set.
bool             set_remove(set_t *set, char const *value);
// Get next item in the set (or first if `ent` is NULL).
set_ent_t const *set_next(set_t const *set, set_ent_t const *ent);
