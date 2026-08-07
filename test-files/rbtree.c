#include <stdint.h>
#include <stddef.h>

#include "rbtree.h"

#define RB_RED   0
#define RB_BLACK 1

#define rb_get_parent(node) ((rbtree_node_t *)(((node->parent) & ~(uintptr_t)1)))
#define rb_get_parent_const(node) ((const rbtree_node_t *)(((node->parent) & ~(uintptr_t)1)))
#define rb_set_parent(node, nparent) ((node->parent) = ((node->parent) & 1) | (uintptr_t)(nparent))
#define rb_get_color(node) ((node->parent) & (uintptr_t)1 ? RB_BLACK : RB_RED)
#define rb_set_color(p, color) ((p->parent) = ((uintptr_t)(p->parent) & ~(uintptr_t)1) | ((color) == RB_BLACK))

static inline rbtree_direction_t rbtree_direction(const rbtree_node_t* node) {
    return node == rb_get_parent_const(node)->right ? RB_RIGHT : RB_LEFT;
}

rbtree_node_t* rbtree_minimum(rbtree_node_t* node) {
    if (!node) return nullptr;
    while (node->left != nullptr)
        node = node->left;
    return node;
}

rbtree_node_t* rbtree_maximum(rbtree_node_t* node) {
    if (!node) return nullptr;
    while (node->right != nullptr)
        node = node->right;
    return node;
}

rbtree_node_t* rbtree_successor(rbtree_node_t* node) {
    if (node->right != nullptr)
        return rbtree_minimum(node->right);
    rbtree_node_t* parent = rb_get_parent(node);
    while (parent != nullptr && node == parent->right) {
        node = parent;
        parent = rb_get_parent(parent);
    }
    return parent;
}

rbtree_node_t* rbtree_predecessor(rbtree_node_t* node) {
    if (node->left != nullptr)
        return rbtree_maximum(node->left);
    rbtree_node_t* parent = rb_get_parent(node);
    while (parent != nullptr && node == parent->left) {
        node = parent;
        parent = rb_get_parent(parent);
    }
    return parent;
}

static rbtree_node_t* rbtree_rotate_subtree(rbtree_t* tree, rbtree_node_t* sub, rbtree_direction_t dir) {
    rbtree_node_t* sub_parent = rb_get_parent(sub);
    rbtree_node_t* new_root = sub->children[1 - dir];
    rbtree_node_t* new_child = new_root->children[dir];

    sub->children[1 - dir] = new_child;

    if (new_child)
        rb_set_parent(new_child, sub);

    new_root->children[dir] = sub;
    rb_set_parent(new_root, sub_parent);
    rb_set_parent(sub, new_root);

    if (sub_parent)
        sub_parent->children[sub == sub_parent->right] = new_root;
    else
        tree->root = new_root;

    return new_root;
}

static rbtree_node_t* rbtree_search_exact(rbtree_t* tree, uint64_t query) {
    rbtree_node_t* node = tree->root;
    while (node != nullptr) {
        uint64_t val = tree->value_of_node(node);
        if (query < val)
            node = node->left;
        else if (query > val)
            node = node->right;
        else
            return node;
    }
    return nullptr;
}

static rbtree_node_t* rbtree_search_nearest(rbtree_t* tree, uint64_t query) {
    rbtree_node_t* node = tree->root;
    rbtree_node_t* nearest = nullptr;
    uint64_t best = UINT64_MAX;
    while (node != nullptr) {
        uint64_t val = tree->value_of_node(node);
        uint64_t dist = val > query ? val - query : query - val;
        if (dist < best) {
            best = dist;
            nearest = node;
        }
        if (query < val)
            node = node->left;
        else if (query > val)
            node = node->right;
        else
            return node;
    }
    return nearest;
}

static rbtree_node_t* rbtree_search_lesser(rbtree_t* tree, uint64_t query, bool find_equal) {
    rbtree_node_t* node = tree->root;
    rbtree_node_t* candidate = nullptr;
    while (node != nullptr) {
        uint64_t val = tree->value_of_node(node);
        if (val < query) {
            candidate = node;
            node = node->right;
        } else if (val > query) {
            node = node->left;
        } else {
            if (find_equal) return node;
            candidate = node->left ? rbtree_maximum(node->left) : candidate;
            return candidate;
        }
    }
    return candidate;
}

static rbtree_node_t* rbtree_search_greater(rbtree_t* tree, uint64_t query, bool find_equal) {
    rbtree_node_t* node = tree->root;
    rbtree_node_t* candidate = nullptr;
    while (node != nullptr) {
        uint64_t val = tree->value_of_node(node);
        if (val > query) {
            candidate = node;
            node = node->left;
        } else if (val < query) {
            node = node->right;
        } else {
            if (find_equal) return node;
            candidate = node->right ? rbtree_minimum(node->right) : candidate;
            return candidate;
        }
    }
    return candidate;
}

rbtree_node_t* rbtree_search(rbtree_t* tree, uint64_t query, rbtree_search_type_t type) {
    switch (type) {
        case RB_SEARCH_TYPE_EXACT:       return rbtree_search_exact(tree, query);
        case RB_SEARCH_TYPE_NEAREST:     return rbtree_search_nearest(tree, query);
        case RB_SEARCH_TYPE_NEAREST_LT:  return rbtree_search_lesser(tree, query, false);
        case RB_SEARCH_TYPE_NEAREST_LTE: return rbtree_search_lesser(tree, query, true);
        case RB_SEARCH_TYPE_NEAREST_GT:  return rbtree_search_greater(tree, query, false);
        case RB_SEARCH_TYPE_NEAREST_GTE: return rbtree_search_greater(tree, query, true);
    }
}

static void rbtree_insert_fixup(rbtree_t* tree, rbtree_node_t* node, rbtree_node_t* parent, rbtree_direction_t dir) {
    rb_set_color(node, RB_RED);
    rb_set_parent(node, parent);

    if (!parent) {
        tree->root = node;
        rb_set_color(node, RB_BLACK);
        return;
    }

    do {
        // case 1
        if (rb_get_color(parent) == RB_BLACK) {
            return;
        }

        rbtree_node_t* grandparent = rb_get_parent(parent);

        // case 4
        if (!grandparent) {
            rb_set_color(parent, RB_BLACK);
            return;
        }

        dir = rbtree_direction(parent);
        rbtree_node_t* uncle = grandparent->children[1 - dir];
        if (!uncle || rb_get_color(uncle) == RB_BLACK) {
            // case 5
            if (node == parent->children[1 - dir]) {
                rbtree_rotate_subtree(tree, parent, dir);
                node = parent;
                parent = grandparent->children[dir];
            }

            // case 6
            rbtree_rotate_subtree(tree, grandparent, 1 - dir);
            rb_set_color(parent, RB_BLACK);
            rb_set_color(grandparent, RB_RED);
            return;
        }

        // case 2
        rb_set_color(parent, RB_BLACK);
        rb_set_color(uncle, RB_BLACK);
        rb_set_color(grandparent, RB_RED);
        node = grandparent;

    } while ((parent = rb_get_parent(node)));

    // case 3
    rb_set_color(tree->root, RB_BLACK);
    return;
}

rbtree_node_t* rbtree_insert(rbtree_t* tree, rbtree_node_t* node) {
    node->left = nullptr;
    node->right = nullptr;
    rb_set_parent(node, 0);

    rbtree_node_t* parent = nullptr;
    rbtree_node_t* cur = tree->root;
    rbtree_direction_t dir = RB_LEFT;

    uint64_t key = tree->value_of_node(node);

    while (cur) {
        parent = cur;
        uint64_t cur_val = tree->value_of_node(cur);

        if (key < cur_val) {
            dir = RB_LEFT;
            cur = cur->left;
        } else {
            dir = RB_RIGHT;
            cur = cur->right;
        }
    }

    if (!parent) {
        tree->root = node;
        rb_set_parent(node, 0);
    } else {
        rb_set_parent(node, parent);
        parent->children[dir] = node;
    }

    rbtree_insert_fixup(tree, node, parent, dir);

    return node;
}

static void rbtree_remove_fixup(rbtree_t* tree, rbtree_node_t* node, rbtree_direction_t dir) {
    rbtree_node_t* parent = rb_get_parent(node);

    if (!parent) {
        rb_set_color(node, RB_BLACK);
        return;
    }

    rbtree_node_t* sibling;
    rbtree_node_t* close_nephew;
    rbtree_node_t* distant_nephew;

    do {
        sibling = parent->children[1 - dir];
        distant_nephew = sibling->children[1 - dir];
        close_nephew = sibling->children[dir];
        if (rb_get_color(sibling) == RB_RED) {
            // case 3
            rbtree_rotate_subtree(tree, parent, dir);
            rb_set_color(parent, RB_RED);
            rb_set_color(sibling, RB_BLACK);
            sibling = close_nephew;

            distant_nephew = sibling->children[1 - dir];
            if (distant_nephew && rb_get_color(distant_nephew) == RB_RED) {
                goto case_6;
            }
            close_nephew = sibling->children[dir];
            if (close_nephew && rb_get_color(close_nephew) == RB_RED) {
                goto case_5;
            }

            // case 4
            rb_set_color(sibling, RB_RED);
            rb_set_color(parent, RB_BLACK);
            return;
        }

        if (distant_nephew && rb_get_color(distant_nephew) == RB_RED)
            goto case_6;

        if (close_nephew && rb_get_color(close_nephew) == RB_RED)
            goto case_5;

        // case 4
        if (rb_get_color(parent) == RB_RED) {
            rb_set_color(sibling, RB_RED);
            rb_set_color(parent, RB_BLACK);
            return;
        }

        // case 2
        rb_set_color(sibling, RB_RED);
        node = parent;
        if (!rb_get_parent(node)) break;
        dir = rbtree_direction(node);

    } while ((parent = rb_get_parent(node)));

    // case 1
    return;

case_5:
    rbtree_rotate_subtree(tree, sibling, 1 - dir);
    rb_set_color(sibling, RB_RED);
    rb_set_color(close_nephew, RB_BLACK);
    distant_nephew = sibling;
    sibling = close_nephew;

case_6:
    rbtree_rotate_subtree(tree, parent, dir);
    rb_set_color(sibling, rb_get_color(parent));
    rb_set_color(parent, RB_BLACK);
    rb_set_color(distant_nephew, RB_BLACK);
    return;
}

rbtree_node_t* rbtree_remove(rbtree_t* tree, rbtree_node_t* node) {
    rbtree_node_t* parent = rb_get_parent(node);
    rbtree_node_t* replacement = nullptr;
    rbtree_direction_t dir = RB_LEFT;

    if (node->left && node->right) {
        rbtree_node_t* succ = rbtree_minimum(node->right);
        rbtree_node_t* succ_parent = rb_get_parent(succ);
        rbtree_node_t* succ_right  = succ->right;

        succ->left = node->left;
        rb_set_parent(succ->left, succ);

        rbtree_node_t* node_parent = rb_get_parent(node);
        rb_set_parent(succ, node_parent);
        if (!node_parent)
            tree->root = succ;
        else
            node_parent->children[node == node_parent->right] = succ;

        if (succ_parent == node) {
            succ->right = node;
            rb_set_parent(node, succ);
        } else {
            succ->right = node->right;
            rb_set_parent(succ->right, succ);
            succ_parent->left = node;
            rb_set_parent(node, succ_parent);
        }

        node->left  = nullptr;
        node->right = succ_right;
        if (node->right) rb_set_parent(node->right, node);

        int tmp = rb_get_color(node);
        rb_set_color(node, rb_get_color(succ));
        rb_set_color(succ, tmp);

        parent = rb_get_parent(node);
    }

    replacement = node->left ? node->left : node->right;

    if (replacement)
        rb_set_parent(replacement, parent);

    if (!parent) {
        tree->root = replacement;
    } else {
        dir = rbtree_direction(node);
        parent->children[dir] = replacement;
    }

    if (rb_get_color(node) == RB_BLACK) {
        if (replacement && rb_get_color(replacement) == RB_RED) {
            rb_set_color(replacement, RB_BLACK);
        } else if (replacement) {
            rbtree_remove_fixup(tree, replacement, dir);
        } else if (parent) {
            rbtree_remove_fixup(tree, node, dir);
        }
    }

    return replacement;
}
