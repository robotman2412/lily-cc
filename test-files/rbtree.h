#pragma once

#include <stdint.h>

#define CONTAINER_OF(PTR, TYPE, MEMBER)                                                                                           \
({                                                                                                                                \
    static_assert(__builtin_types_compatible_p(typeof(((TYPE*) 0)->MEMBER), typeof(*PTR)), "member type does not match pointer"); \
    (TYPE*) (((uintptr_t) (PTR)) - __builtin_offsetof(TYPE, MEMBER));                                                             \
})

#define RBTREE_SUCCESSOR_OR_NULL(node)   ((node) ? rbtree_successor(node) : nullptr)
#define RBTREE_PREDECESSOR_OR_NULL(node) ((node) ? rbtree_predecessor(node) : nullptr)

#define RBTREE_FOR_EACH(tree, it)                            \
    for (rbtree_node_t *(it) = rbtree_minimum((tree).root);  \
         (it);                                               \
         (it) = rbtree_successor((it)))

#define RBTREE_FOR_EACH_REVERSE(tree, it)                    \
    for (rbtree_node_t *(it) = rbtree_maximum((tree).root);  \
         (it);                                               \
         (it) = rbtree_predecessor((it)))

#define RBTREE_FOR_EACH_SAFE(tree, it, nx)                      \
    for (rbtree_node_t *(it) = rbtree_minimum((tree).root),     \
                       *(nx) = RBTREE_SUCCESSOR_OR_NULL(it);    \
         (it);                                                  \
         (it) = (nx),                                           \
         (nx) = RBTREE_SUCCESSOR_OR_NULL(nx))

#define RBTREE_FOR_EACH_REVERSE_SAFE(tree, it, px)              \
    for (rbtree_node_t *(it) = rbtree_maximum((tree).root),     \
                       *(px) = RBTREE_PREDECESSOR_OR_NULL(it);  \
         (it);                                                  \
         (it) = (px),                                           \
         (px) = RBTREE_PREDECESSOR_OR_NULL(px))

typedef enum rbtree_direction_t {
    RB_LEFT,
    RB_RIGHT,
} rbtree_direction_t;

typedef enum {
    RB_SEARCH_TYPE_EXACT,
    RB_SEARCH_TYPE_NEAREST,
    RB_SEARCH_TYPE_NEAREST_LT,
    RB_SEARCH_TYPE_NEAREST_LTE,
    RB_SEARCH_TYPE_NEAREST_GT,
    RB_SEARCH_TYPE_NEAREST_GTE,
} rbtree_search_type_t;

typedef struct rbtree rbtree_t;
typedef struct rbtree_node rbtree_node_t;

typedef struct rbtree {
    rbtree_node_t* root;
    uint64_t (*value_of_node)(rbtree_node_t* node);
} rbtree_t;

typedef struct rbtree_node {
    uintptr_t parent;
    union {
        struct {
            rbtree_node_t* left;
            rbtree_node_t* right;
        };
        rbtree_node_t* children[2];
    };
} rbtree_node_t;

extern rbtree_node_t* rbtree_search(rbtree_t* tree, uint64_t query, rbtree_search_type_t type);
extern rbtree_node_t* rbtree_insert(rbtree_t* tree, rbtree_node_t* node);
extern rbtree_node_t* rbtree_remove(rbtree_t* tree, rbtree_node_t* node);

extern rbtree_node_t* rbtree_minimum(rbtree_node_t* node);
extern rbtree_node_t* rbtree_maximum(rbtree_node_t* node);
extern rbtree_node_t* rbtree_successor(rbtree_node_t* node);
extern rbtree_node_t* rbtree_predecessor(rbtree_node_t* node);
