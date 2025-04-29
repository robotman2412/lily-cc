
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "isel_tree.h"

#include "insn_proto.h"
#include "ir_interpreter.h"
#include "ir_types.h"
#include "set.h"
#include "strong_malloc.h"

#include <stdlib.h>



// Generate an instruction selection tree matching a (sub)tree from an instruction prototype.
static isel_tree_t *isel_copy_subtree(expr_tree_t const *proto_tree, insn_proto_t const *proto) {
    if (!proto_tree) {
        return NULL;
    }

    isel_tree_t *node = strong_calloc(1, sizeof(isel_tree_t));
    node->type        = proto_tree->type;

    if (proto_tree->type == EXPR_TREE_ICONST) {
        node->iconst     = proto_tree->iconst;
        node->protos_len = 1;
        node->protos     = strong_calloc(1, sizeof(void *));
        node->protos[0]  = proto;

    } else if (proto_tree->type == EXPR_TREE_OPERAND) {
        // Nothing to do.
        node->protos_len = 1;
        node->protos     = strong_calloc(1, sizeof(void *));
        node->protos[0]  = proto;

    } else if (proto_tree->type == EXPR_TREE_IR_INSN) {
        // Deep copy.
        node->expr.insn_type = proto_tree->expr.insn_type;
        switch (proto_tree->expr.insn_type) {
            case IR_INSN_EXPR:
                node->expr.expr.op1 = proto_tree->expr.expr.op1;
                node->expr.expr.lhs = isel_copy_subtree(proto_tree->expr.expr.lhs, proto);
                node->expr.expr.rhs = isel_copy_subtree(proto_tree->expr.expr.rhs, proto);
                break;

            case IR_INSN_FLOW:
                node->expr.flow.type  = proto_tree->expr.flow.type;
                node->expr.flow.value = isel_copy_subtree(proto_tree->expr.flow.value, proto);
                break;

            case IR_INSN_MEM:
                node->expr.mem.type  = proto_tree->expr.mem.type;
                node->expr.mem.ptr   = isel_copy_subtree(proto_tree->expr.mem.ptr, proto);
                node->expr.mem.value = isel_copy_subtree(proto_tree->expr.mem.value, proto);
                break;

            default: abort();
        }

    } else {
        abort();
    }

    return node;
}

// Inserts a prototype into the tree's list of options.
static void isel_insert_option(isel_tree_t *tree, insn_proto_t const *proto) {
    tree->protos                     = strong_realloc(tree->protos, (tree->protos_len + 1) * sizeof(void *));
    tree->protos[tree->protos_len++] = proto;
}

// Add an instruction to an existing tree, assuming it's modifiable.
static void isel_insert_insn(isel_tree_t *tree, expr_tree_t const *proto_tree, insn_proto_t const *proto) {
    if (!tree || !proto_tree)
        return;
    isel_tree_t **next_ptr = &tree->next;

    // Loop over all nodes on this layer.
    for (; tree; tree = tree->next, next_ptr = &tree->next) {
        // Test whether the current level matches this node.
        if (tree->type != proto_tree->type)
            continue;
        if (proto_tree->type == EXPR_TREE_ICONST) {
            if (!ir_const_lenient_identical(tree->iconst, proto_tree->iconst))
                continue;
            // Matched an identical constant.
            isel_insert_option(tree, proto);
            return;

        } else if (proto_tree->type == EXPR_TREE_OPERAND) {
            // Matched an operand here.
            isel_insert_option(tree, proto);
            return;

        } else if (proto_tree->type == EXPR_TREE_IR_INSN) {
            if (tree->expr.insn_type != proto_tree->expr.insn_type)
                continue;
            switch (tree->expr.insn_type) {
                case IR_INSN_EXPR:
                    if (tree->expr.expr.type != proto_tree->expr.expr.type)
                        continue;
                    if (tree->expr.expr.op1 != proto_tree->expr.expr.op1)
                        continue;

                    // Expression matched; insert its subtrees.
                    isel_insert_insn((void *)tree->expr.expr.lhs, proto_tree->expr.expr.lhs, proto);
                    isel_insert_insn((void *)tree->expr.expr.rhs, proto_tree->expr.expr.rhs, proto);
                    return;

                case IR_INSN_FLOW:
                    if (tree->expr.flow.type != proto_tree->expr.flow.type)
                        continue;

                    // Flow control matched; insert its subtree.
                    isel_insert_insn(tree->expr.flow.value, proto_tree->expr.flow.value, proto);
                    return;

                case IR_INSN_MEM:
                    if (tree->expr.mem.type != proto_tree->expr.mem.type)
                        continue;

                    // Memory matched; insert its subtree.
                    isel_insert_insn(tree->expr.mem.ptr, proto_tree->expr.mem.ptr, proto);
                    isel_insert_insn(tree->expr.mem.value, proto_tree->expr.mem.value, proto);
                    return;
            }
        }
    }

    // If no levels matched, add a new subtree here.
    *next_ptr = isel_copy_subtree(proto_tree, proto);
}

// Generate an instruction selection tree given an instruction set.
isel_tree_t *isel_tree_generate(size_t protos_len, insn_proto_t const *const *protos) {
    isel_tree_t *initial = isel_copy_subtree(protos[0]->tree, protos[0]);
    for (size_t i = 0; i < protos_len; i++) {
        isel_insert_insn(initial, protos[i]->tree, protos[i]);
    }
    return initial;
}



// Match an instruction selection tree against IR in SSA form.
insn_proto_t const *isel(isel_tree_t const *tree, ir_insn_t const *ir_insn) {
    set_t candidates = PTR_SET_EMPTY;

    // Walk the tree.

    // Eliminate candidates whose requirements aren't satisfied.
    set_ent_t const *ent = set_next(&candidates, NULL);
    while (ent) {
        set_ent_t const *next = set_next(&candidates, ent);
        if (!insn_proto_match(ent->value, ir_insn)) {
            set_remove(&candidates, ent->value);
        }
        ent = next;
    }

    // Select the instruction that consumes most IR.
}
