
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "cand_tree.h"

#include "insn_proto.h"
#include "ir_interpreter.h"
#include "ir_types.h"
#include "strong_malloc.h"

#include <assert.h>
#include <stdlib.h>



// Add instruction selection candidates based on an IR operand.
static void tree_isel_add_candidates_operand(set_t *candidates, cand_tree_t const *tree, ir_operand_t ir_operand);
// Add instruction selection candidates based on an IR instruction.
static void tree_isel_add_candidates_insn(set_t *candidates, cand_tree_t const *tree, ir_insn_t const *ir_insn);

// Recursive function to match instruction prototypes against an IR operand.
// Returns how many IR instructions and operands would be replaced by this prototype.
// Returns 0 if the candidate instruction cannot be applied here.
static size_t tree_isel_match_expr_tree_operand(
    ir_operand_t const **proto_operands_out,
    expr_tree_t const   *tree,
    ir_operand_t const  *ir_operand,
    ir_code_t const     *parent_code
);
// Recursive function to match instruction prototypes against an IR instruction.
// Returns how many IR instructions and operands would be replaced by this prototype.
// Returns 0 if the candidate instruction cannot be applied here.
static size_t tree_isel_match_expr_tree_insn(
    ir_operand_t const **proto_operands_out,
    expr_tree_t const   *tree,
    ir_insn_t const     *ir_insn,
    ir_code_t const     *parent_code
);



// Generate an instruction selection tree matching a (sub)tree from an instruction prototype.
static cand_tree_t *isel_copy_subtree(expr_tree_t const *proto_tree, insn_proto_t const *proto) {
    if (!proto_tree) {
        return NULL;
    }

    cand_tree_t *node = strong_calloc(1, sizeof(cand_tree_t));
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

            case IR_INSN_FLOW: node->expr.flow.value = isel_copy_subtree(proto_tree->expr.flow.value, proto); break;

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
static void isel_insert_option(cand_tree_t *tree, insn_proto_t const *proto) {
    tree->protos                     = strong_realloc(tree->protos, (tree->protos_len + 1) * sizeof(void *));
    tree->protos[tree->protos_len++] = proto;
}

// Add an instruction to an existing tree, assuming it's modifiable.
static void isel_insert_insn(cand_tree_t *tree, expr_tree_t const *proto_tree, insn_proto_t const *proto) {
    if (!tree || !proto_tree)
        return;
    cand_tree_t **next_ptr = &tree->next;

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
cand_tree_t *cand_tree_generate(size_t protos_len, insn_proto_t const *const *protos) {
    cand_tree_t *initial = isel_copy_subtree(protos[0]->tree, protos[0]);
    for (size_t i = 0; i < protos_len; i++) {
        isel_insert_insn(initial, protos[i]->tree, protos[i]);
    }
    return initial;
}



// Helper for `tree_isel_match_expr_tree_operand` to set `proto_operands_out`.
static bool tree_isel_set_proto_operand(
    ir_operand_t const **proto_operands_out, expr_tree_t const *tree, ir_operand_t const *ir_operand
) {
    if (proto_operands_out[tree->operand_index]
        && !ir_operand_lenient_identical(*proto_operands_out[tree->operand_index], *ir_operand)) {
        return false;
    }
    proto_operands_out[tree->operand_index] = ir_operand;
    return true;
}

// Recursive function to match instruction prototypes against an IR operand.
// Returns how many IR instructions and operands would be replaced by this prototype.
// Returns 0 if the candidate instruction cannot be applied here.
static size_t tree_isel_match_expr_tree_operand(
    ir_operand_t const **proto_operands_out,
    expr_tree_t const   *tree,
    ir_operand_t const  *ir_operand,
    ir_code_t const     *parent_code
) {
    if (tree->type == EXPR_TREE_OPERAND) {
        return tree_isel_set_proto_operand(proto_operands_out, tree, ir_operand);
    } else if (tree->type == EXPR_TREE_ICONST) {
        return ir_operand->is_const && tree_isel_set_proto_operand(proto_operands_out, tree, ir_operand);
    } else if (ir_operand->is_const) {
        return 0;
    } else {
        return tree_isel_match_expr_tree_insn(
            proto_operands_out,
            tree,
            (ir_insn_t const *)ir_operand->var->assigned_at.head,
            parent_code
        );
    }
}

// Recursive function to match instruction prototypes against an IR instruction.
// Returns how many IR instructions and operands would be replaced by this prototype.
// Returns 0 if the candidate instruction cannot be applied here.
static size_t tree_isel_match_expr_tree_insn(
    ir_operand_t const **proto_operands_out,
    expr_tree_t const   *tree,
    ir_insn_t const     *ir_insn,
    ir_code_t const     *parent_code
) {
    if (tree->type != EXPR_TREE_IR_INSN || tree->expr.insn_type != ir_insn->type) {
        return 0;
    }

    switch (tree->expr.insn_type) {
        case IR_INSN_EXPR: {
            ir_expr_t const *expr = (ir_expr_t const *)ir_insn;
            if (expr->type != tree->expr.expr.type || expr->e_unary.oper != tree->expr.expr.op1) {
                return 0;
            }
            if (expr->type == IR_EXPR_UNARY) {
                size_t a = tree_isel_match_expr_tree_operand(
                    proto_operands_out,
                    tree->expr.expr.lhs,
                    &expr->e_unary.value,
                    parent_code
                );
                return a ? a + 1 : 0;
            } else {
                size_t a = tree_isel_match_expr_tree_operand(
                    proto_operands_out,
                    tree->expr.expr.lhs,
                    &expr->e_binary.lhs,
                    parent_code
                );
                size_t b = tree_isel_match_expr_tree_operand(
                    proto_operands_out,
                    tree->expr.expr.lhs,
                    &expr->e_binary.rhs,
                    parent_code
                );
                return a && b ? a + b + 1 : 0;
            }
        } break;

        case IR_INSN_FLOW: {
            ir_flow_t const *flow = (ir_flow_t const *)ir_insn;
            size_t           a    = tree_isel_match_expr_tree_operand(
                proto_operands_out,
                tree->expr.flow.value,
                &flow->f_branch.cond,
                parent_code
            );
            return a ? a + 1 : 0;
        } break;

        case IR_INSN_MEM: {
            ir_mem_t const *mem = (ir_mem_t const *)ir_insn;
            if (mem->type != tree->expr.mem.type) {
                return 0;
            }
            if (tree->expr.mem.type == IR_MEM_STORE) {
                size_t a = tree_isel_match_expr_tree_operand(
                    proto_operands_out,
                    tree->expr.mem.ptr,
                    &mem->m_store.addr,
                    parent_code
                );
                size_t b = tree_isel_match_expr_tree_operand(
                    proto_operands_out,
                    tree->expr.mem.value,
                    &mem->m_store.src,
                    parent_code
                );
                return a && b ? a + b + 1 : 0;
            } else if (tree->expr.mem.type == IR_MEM_LOAD) {
                size_t a = tree_isel_match_expr_tree_operand(
                    proto_operands_out,
                    tree->expr.mem.ptr,
                    &mem->m_load.addr,
                    parent_code
                );
                return a ? a + 1 : 0;
            } else if (tree->expr.mem.type == IR_MEM_LEA_STACK || tree->expr.mem.type == IR_MEM_LEA_SYMBOL) {
                return 1;
            }
        } break;
    }

    abort();
}

// Test whether a candidate instruction prototypes matches the IR.
// Returns how many IR instructions and operands would be replaced by this prototype.
// Returns 0 if the candidate instruction cannot be applied here.
static size_t tree_isel_match_proto(insn_proto_t const *proto, ir_insn_t const *ir_insn, ir_code_t const *parent_code) {
    if (ir_insn->parent != parent_code) {
        return 0;
    }

    // First collect all operands.
    ir_operand_t const *ir_operands[8] = {0};
    size_t              match_size     = tree_isel_match_expr_tree_insn(ir_operands, proto->tree, ir_insn, parent_code);
    if (!match_size)
        return 0;

    // Then verify operand rules.
    for (size_t i = 0; i < proto->operands_len; i++) {
        assert(ir_operands[i] != NULL);
        operand_rule_t rule = proto->operands[i];

        if (ir_operands[i]->is_const) {
            // Validate constant rules.
            switch (ir_operands[i]->iconst.prim_type) {
                case IR_PRIM_f32:
                    if (!rule.operand_kinds.f32)
                        return false;
                case IR_PRIM_f64:
                    if (!rule.operand_kinds.f64)
                        return false;
                case IR_PRIM_bool:
                    if (!rule.operand_kinds.sint && !rule.operand_kinds.uint)
                        return false;
                case IR_PRIM_s128:
                case IR_PRIM_u128:
                    // TODO: Logic to check 128-bit constants.
                    return false;
                default: {
                    bool is_unsigned = ir_operands[i]->iconst.prim_type & 1;
                    if (!is_unsigned && !rule.operand_kinds.sint)
                        return false;

                    int64_t val           = ir_trim_const(ir_operands[i]->iconst).constl;
                    int     unsigned_bits = 64 - __builtin_clzll(val);
                    int     signed_bits   = val < 0 ? 64 - __builtin_clzll(~val) : unsigned_bits + 1;

                    if (is_unsigned && unsigned_bits + !rule.operand_kinds.uint > rule.const_bits)
                        return false;
                    else if (!is_unsigned && signed_bits > rule.const_bits)
                        return false;
                } break;
            }
        }
    }

    return true;
}



// Add instruction selection candidates based on an IR operand.
static void tree_isel_add_candidates_operand(set_t *candidates, cand_tree_t const *tree, ir_operand_t ir_operand) {
    for (; tree; tree = tree->next) {
        if ((tree->type == EXPR_TREE_ICONST && ir_operand.is_const
             && ir_const_lenient_identical(tree->iconst, ir_operand.iconst))
            || tree->type == EXPR_TREE_OPERAND) {
            for (size_t i = 0; i < tree->protos_len; i++) {
                set_add(candidates, tree->protos[i]);
            }
        }
    }

    if (!ir_operand.is_const) {
        tree_isel_add_candidates_insn(candidates, tree, (ir_insn_t const *)ir_operand.var->assigned_at.head);
    }
}

// Add instruction selection candidates based on an IR instruction.
static void tree_isel_add_candidates_insn(set_t *candidates, cand_tree_t const *tree, ir_insn_t const *ir_insn) {
    for (; tree; tree = tree->next) {
        if (tree->type != EXPR_TREE_IR_INSN || tree->expr.insn_type != ir_insn->type)
            continue;
        switch (tree->expr.insn_type) {
            case IR_INSN_EXPR: {
                ir_expr_t const *ir_expr = (ir_expr_t const *)ir_insn;
                if (tree->expr.expr.type != ir_expr->type || tree->expr.expr.op1 != ir_expr->e_unary.oper)
                    continue;
                if (tree->expr.expr.type == IR_EXPR_UNARY) {
                    tree_isel_add_candidates_operand(candidates, tree->expr.expr.lhs, ir_expr->e_unary.value);
                } else {
                    tree_isel_add_candidates_operand(candidates, tree->expr.expr.lhs, ir_expr->e_binary.lhs);
                    tree_isel_add_candidates_operand(candidates, tree->expr.expr.rhs, ir_expr->e_binary.rhs);
                }
            } break;

            case IR_INSN_FLOW: {
                ir_flow_t const *ir_flow = (ir_flow_t const *)ir_insn;
                tree_isel_add_candidates_operand(candidates, tree->expr.flow.value, ir_flow->f_branch.cond);
            } break;

            case IR_INSN_MEM: {
                ir_mem_t const *ir_mem = (ir_mem_t const *)ir_insn;
                if (tree->expr.mem.type != ir_mem->type)
                    continue;
                if (tree->expr.mem.type == IR_MEM_STORE) {
                    tree_isel_add_candidates_operand(candidates, tree->expr.mem.ptr, ir_mem->m_store.addr);
                    tree_isel_add_candidates_operand(candidates, tree->expr.mem.value, ir_mem->m_store.src);
                } else if (tree->expr.mem.type == IR_MEM_LOAD) {
                    tree_isel_add_candidates_operand(candidates, tree->expr.mem.ptr, ir_mem->m_load.addr);
                } else if (tree->expr.mem.type == IR_MEM_LEA_STACK || tree->expr.mem.type == IR_MEM_LEA_SYMBOL) {
                    for (size_t i = 0; i < tree->protos_len; i++) {
                        set_add(candidates, tree->protos[i]);
                    }
                }
            } break;
        }
    }
}



// Match an instruction selection tree against IR in SSA form.
insn_proto_t const *cand_tree_isel(cand_tree_t const *tree, ir_insn_t const *ir_insn) {
    set_t candidates = PTR_SET_EMPTY;

    // Walk the tree, adding matching prototypes.
    tree_isel_add_candidates_insn(&candidates, tree, ir_insn);

    // Select the instruction that consumes most IR.
    size_t              max_size = 0;
    insn_proto_t const *best_fit = NULL;

    set_foreach(insn_proto_t, proto, &candidates) {
        size_t proto_size = tree_isel_match_proto(proto, ir_insn, ir_insn->parent);
        if (proto_size > max_size) {
            max_size = proto_size;
            best_fit = proto;
        }
    }

    return best_fit;
}
