
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "set.h"

#include "hash.h"
#include "lilycc_malloc.h"
#include "list.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



// Vtable for string sets.
set_vtable_t const str_set_vtable = {
    .val_cmp  = (int (*)(void const *, void const *))strcmp,
    .val_del  = lilycc_free,
    .val_dup  = (void *(*)(void const *))lilycc_strdup,
    .val_hash = (uint32_t (*)(void const *))hash_cstr,
};

// Vtable for pointer sets.
set_vtable_t const ptr_set_vtable = {
    .val_hash = hash_ptr,
    .val_cmp  = cmp_ptr,
    .val_dup  = dup_nop,
    .val_del  = del_nop,
};

// Threshold of max items per bucket.
// Note: The number is estimated by total size as opposed to actually counting.
#define SET_MAX_BUCKET_SIZE 4



// Change the amount of buckets that a set has.
static void set_resize(set_t *set, size_t new_buckets_len) {
    if (new_buckets_len == set->buckets_len) {
        return;
    }

    // Try to allocate memory for the new buckets.
    dlist_t *new_buckets = lilycc_calloc(new_buckets_len, sizeof(dlist_t));

    // Sort entries into the new correct buckets.
    for (size_t i = 0; i < set->buckets_len; i++) {
        set_ent_t *ent;
        while ((ent = container_of(dlist_pop_front(&set->buckets[i]), set_ent_t, node))) {
            dlist_append(&new_buckets[ent->hash & (new_buckets_len - 1)], &ent->node);
        }
    }

    // Update the set's pointers.
    lilycc_free(set->buckets);
    set->buckets     = new_buckets;
    set->buckets_len = new_buckets_len;
}

// Try to resize the set according to the current occupancy (but don't fail if OOM).
static void set_auto_resize(set_t *set) {
    if (set->len == 0) {
        lilycc_free(set->buckets);
        set->buckets     = NULL;
        set->buckets_len = 0;
        return;
    }

    // Calculate how many buckets this set would need.
    size_t min_buckets = (set->len + SET_MAX_BUCKET_SIZE - 1) / SET_MAX_BUCKET_SIZE;
    // Round up to the next power of 2.
    if (min_buckets & (min_buckets - 1)) {
        min_buckets = 1lu << (1 + 8 * sizeof(size_t) - __builtin_clzl(min_buckets));
    }

    set_resize(set, min_buckets);
}

// Remove all entries from a set.
void set_clear(set_t *set) {
    for (size_t i = 0; i < set->buckets_len; i++) {
        dlist_node_t *node = dlist_pop_front(&set->buckets[i]);
        while (node) {
            set_ent_t *ent = (set_ent_t *)node;
            set->vtable->val_del(ent->value);
            lilycc_free(ent);
            node = dlist_pop_front(&set->buckets[i]);
        }
    }
    lilycc_free(set->buckets);
    set->buckets     = NULL;
    set->buckets_len = 0;
    set->len         = 0;
}

// Get an item from the set.
set_get_t set_get(set_t const *set, void const *value) {
    if (!set->len) {
        return (set_get_t){false, NULL};
    }

    // Figure out which bucket the value is in.
    uint32_t hash   = set->vtable->val_hash(value);
    size_t   bucket = hash & (set->buckets_len - 1);

    // Walk the list of items in this bucket.
    dlist_node_t *node = set->buckets[bucket].head;
    while (node) {
        set_ent_t *ent = (set_ent_t *)node;
        // Both hash and string compare must be equal.
        if (ent->hash == hash && !set->vtable->val_cmp(ent->value, value)) {
            return (set_get_t){true, ent->value};
        }
        // Go to the next item in the bucket.
        node = node->next;
    }

    // The bucket did not contain the value.
    return (set_get_t){false, NULL};
}

// Insert an item into the set.
bool set_add(set_t *set, void const *value) {
    if (!set->buckets_len) {
        set_resize(set, 2);
    }

    // Figure out which bucket the value is in.
    uint32_t hash   = set->vtable->val_hash(value);
    size_t   bucket = hash & (set->buckets_len - 1);

    // Walk the list of items in this bucket.
    dlist_node_t *node = set->buckets[bucket].head;
    while (node) {
        set_ent_t *ent = (set_ent_t *)node;
        // Both hash and string compare must be equal.
        if (ent->hash == hash && !set->vtable->val_cmp(ent->value, value)) {
            // There is an existing value.
            return false;
        }
        node = node->next;
    }

    // Allocate a new item.
    set_ent_t *ent = lilycc_malloc(sizeof(set_ent_t));
    ent->node      = DLIST_NODE_EMPTY;
    ent->value     = set->vtable->val_dup(value);
    ent->hash      = hash;

    // Add the new item to the bucket.
    dlist_append(&set->buckets[bucket], &ent->node);
    set->len++;

    set_auto_resize(set);
    return true;
}

// Add all items from another set to this one.
// Returns how many items, if any, are added.
size_t set_addall(set_t *set, set_t const *other) {
    if (set->vtable != other->vtable) {
        fprintf(stderr, "Error: Sets contain different types\n");
        abort();
    }
    size_t added = 0;

    if (!other->len) {
        return 0;
    }

    for (size_t x = 0; x < other->buckets_len; x++) {
        dlist_foreach_node(set_ent_t, ent, &other->buckets[x]) {
            added += set_add(set, ent->value);
        }
    }

    return added;
}

// Remove all items in another set from this one.
size_t set_removeall(set_t *set, set_t const *other) {
    if (set->vtable != other->vtable) {
        fprintf(stderr, "Error: Sets contain different types\n");
        abort();
    }
    size_t removed = 0;

    if (!other->len) {
        return 0;
    }

    for (size_t x = 0; x < other->buckets_len; x++) {
        dlist_foreach_node(set_ent_t, ent, &other->buckets[x]) {
            removed += set_remove(set, ent->value);
        }
    }

    return removed;
}

// Retain all items also in the other set.
// Returns how many items, if any, are removed.
size_t set_intersect(set_t *set, set_t const *other) {
    if (set->vtable != other->vtable) {
        fprintf(stderr, "Error: Sets contain different types\n");
        abort();
    }
    size_t removed = 0;

    for (size_t i = 0; i < set->buckets_len; i++) {
        set_ent_t *ent = (void *)set->buckets[i].head;
        while (ent) {
            bool       keep = set_contains(other, ent->value);
            set_ent_t *next = (void *)ent->node.next;
            if (!keep) {
                dlist_remove(&set->buckets[i], &ent->node);
                set->vtable->val_del(ent->value);
                lilycc_free(ent);
                removed++;
            }
            ent = next;
        }
    }

    set_auto_resize(set);
    return removed;
}

// Remove an item from the set.
bool set_remove(set_t *set, void const *value) {
    if (!set->len) {
        return false;
    }

    // Figure out which bucket the value is in.
    uint32_t hash   = set->vtable->val_hash(value);
    size_t   bucket = hash & (set->buckets_len - 1);

    // Walk the list of items in this bucket.
    dlist_node_t *node = set->buckets[bucket].head;
    while (node) {
        set_ent_t *ent = (set_ent_t *)node;
        // Both hash and string compare must be equal.
        if (ent->hash == hash && !set->vtable->val_cmp(ent->value, value)) {
            // There is an existing value; remove it.
            dlist_remove(&set->buckets[bucket], node);
            set->vtable->val_del(ent->value);
            lilycc_free(ent);
            set->len--;
            set_auto_resize(set);
            return true;
        }
        node = node->next;
    }

    return false;
}

// Get next item in the set (or first if `ent` is NULL).
set_ent_t const *set_next(set_t const *set, set_ent_t const *ent) {
    if (!set->len) {
        return NULL;
    }
    size_t bucket;
    if (!ent) {
        bucket = 0;
    } else if (ent->node.next) {
        return (set_ent_t const *)ent->node.next;
    } else {
        bucket = (ent->hash & (set->buckets_len - 1)) + 1;
    }
    while (bucket < set->buckets_len) {
        if (set->buckets[bucket].head) {
            return (set_ent_t const *)set->buckets[bucket].head;
        }
        bucket++;
    }
    return NULL;
}



#ifndef NDEBUG
// Print the pointer of all items in a set.
void set_dump(set_t const *set) {
    printf("Set %p has %zu item%s\n", set, set->len, set->len == 0 ? "s." : set->len == 1 ? ":" : "s:");
    set_foreach(void, item, set) {
        printf("    %p\n", item);
    }
}
#endif
