
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "cand_tree.h"

#include "backend.h"
#include "insn_proto.h"
#include "ir_interpreter.h"
#include "ir_types.h"
#include "set.h"
#include "strong_malloc.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>



// Add instruction selection candidates based on an IR operand.
static void tree_isel_add_candidates_operand(set_t *candidates, cand_tree_t const *tree, ir_operand_t ir_operand);
// Add instruction selection candidates based on an IR instruction.
static void tree_isel_add_candidates_insn(set_t *candidates, cand_tree_t const *tree, ir_insn_t const *ir_insn);

// Recursive function to match instruction prototypes against an IR operand.
// Returns how many IR instructions and operands would be replaced by this prototype.
// Returns 0 if the candidate instruction cannot be applied here.
static size_t tree_isel_match_expr_tree_operand(
    backend_profile_t   *profile,
    ir_operand_t const **proto_operands_out,
    match_tree_t const  *tree,
    ir_operand_t const  *ir_operand,
    ir_code_t const     *parent_code
);
// Recursive function to match instruction prototypes against an IR instruction.
// Returns how many IR instructions and operands would be replaced by this prototype.
// Returns 0 if the candidate instruction cannot be applied here.
static size_t tree_isel_match_expr_tree_insn(
    backend_profile_t   *profile,
    ir_operand_t const **proto_operands_out,
    match_tree_t const  *tree,
    ir_insn_t const     *ir_insn,
    ir_code_t const     *parent_code
);



// Generate an instruction selection tree matching a (sub)tree from an instruction prototype.
static cand_tree_t *isel_copy_subtree(match_tree_t const *proto_tree, insn_sub_t const *proto) {
    if (!proto_tree) {
        return NULL;
    }

    cand_tree_t *node = strong_calloc(1, sizeof(cand_tree_t));
    node->type        = proto_tree->type;

    if (proto_tree->type == EXPR_TREE_ICONST) {
        node->iconst   = proto_tree->iconst;
        node->subs_len = 1;
        node->subs     = strong_calloc(1, sizeof(void *));
        node->subs[0]  = proto;

    } else if (proto_tree->type == EXPR_TREE_OPERAND) {
        // Nothing to do.
        node->subs_len = 1;
        node->subs     = strong_calloc(1, sizeof(void *));
        node->subs[0]  = proto;

    } else if (proto_tree->type == EXPR_TREE_IR_INSN) {
        // Deep copy.
        node->insn.type         = proto_tree->insn.type;
        node->insn.children_len = proto_tree->insn.children_len;
        node->insn.children     = strong_calloc(node->insn.children_len, sizeof(void *));
        if (node->insn.type == IR_INSN_EXPR1) {
            node->insn.op1 = proto_tree->insn.op1;
        } else if (node->insn.type == IR_INSN_EXPR2) {
            node->insn.op2 = proto_tree->insn.op2;
        }
        for (size_t i = 0; i < node->insn.children_len; i++) {
            node->insn.children[i] = isel_copy_subtree(proto_tree->insn.children[i], proto);
        }

    } else {
        abort();
    }

    return node;
}

// Inserts a prototype into the tree's list of options.
static void isel_insert_option(cand_tree_t *tree, insn_sub_t const *proto) {
    tree->subs                   = strong_realloc(tree->subs, (tree->subs_len + 1) * sizeof(void *));
    tree->subs[tree->subs_len++] = proto;
}

// Add an instruction to an existing tree, assuming it's modifiable.
static void isel_insert_insn(cand_tree_t *tree, match_tree_t const *proto_tree, insn_sub_t const *proto) {
    if (!tree || !proto_tree)
        return;
    cand_tree_t **next_ptr = &tree->next;

    // Loop over all nodes on this layer.
    for (; tree; next_ptr = &tree->next, tree = tree->next) {
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
            if (tree->insn.type != proto_tree->insn.type || tree->insn.children_len != proto_tree->insn.children_len
                || (tree->insn.type == IR_INSN_EXPR1 && tree->insn.op1 != proto_tree->insn.op1)
                || (tree->insn.type == IR_INSN_EXPR2 && tree->insn.op2 != proto_tree->insn.op2)) {
                continue;
            }
            for (size_t i = 0; i < tree->insn.children_len; i++) {
                isel_insert_insn(tree->insn.children[i], proto_tree->insn.children[i], proto);
            }
            return;
        }
    }

    // If no levels matched, add a new subtree here.
    *next_ptr = isel_copy_subtree(proto_tree, proto);
}

// Generate an instruction selection tree given an instruction set.
cand_tree_t *cand_tree_generate(size_t subs_len, insn_sub_t const *const *subs) {
    cand_tree_t *initial = isel_copy_subtree(subs[0]->match_tree, subs[0]);
    for (size_t i = 0; i < subs_len; i++) {
        isel_insert_insn(initial, subs[i]->match_tree, subs[i]);
    }
    return initial;
}



// Delete an instruction selection tree.
void cand_tree_delete(cand_tree_t *tree) {
    while (tree) {
        free(tree->subs);
        if (tree->type == EXPR_TREE_IR_INSN) {
            for (size_t i = 0; i < tree->insn.children_len; i++) {
                cand_tree_delete(tree->insn.children[i]);
            }
            free(tree->insn.children);
        }
        cand_tree_t *next = tree->next;
        free(tree);
        tree = next;
    }
}



// Helper for `tree_isel_match_expr_tree_operand` to set `proto_operands_out`.
static bool tree_isel_set_proto_operand(
    ir_operand_t const **proto_operands_out, match_tree_t const *tree, ir_operand_t const *ir_operand
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
    backend_profile_t   *profile,
    ir_operand_t const **proto_operands_out,
    match_tree_t const  *tree,
    ir_operand_t const  *ir_operand,
    ir_code_t const     *parent_code
) {
    if (tree->type == EXPR_TREE_OPERAND) {
        return tree_isel_set_proto_operand(proto_operands_out, tree, ir_operand);
    } else if (tree->type == EXPR_TREE_ICONST) {
        return ir_operand->type == IR_OPERAND_TYPE_CONST
               && tree_isel_set_proto_operand(proto_operands_out, tree, ir_operand);
    } else if (ir_operand->type == IR_OPERAND_TYPE_CONST) {
        return 0;
    } else {
        set_ent_t const *ent = set_next(&ir_operand->var->assigned_at, NULL);
        if (ent) {
            return tree_isel_match_expr_tree_insn(profile, proto_operands_out, tree, ent->value, parent_code);
        } else {
            return 1;
        }
    }
}

// Recursive function to match instruction prototypes against an IR instruction.
// Returns how many IR instructions and operands would be replaced by this prototype.
// Returns 0 if the candidate instruction cannot be applied here.
static size_t tree_isel_match_expr_tree_insn(
    backend_profile_t   *profile,
    ir_operand_t const **proto_operands_out,
    match_tree_t const  *tree,
    ir_insn_t const     *ir_insn,
    ir_code_t const     *parent_code
) {
    if (tree->type != EXPR_TREE_IR_INSN || tree->insn.type != ir_insn->type
        || (tree->insn.type == IR_INSN_EXPR1 && tree->insn.op1 != ir_insn->op1)
        || (tree->insn.type == IR_INSN_EXPR2 && tree->insn.op2 != ir_insn->op2)) {
        return 0;
    }

    size_t total = 1;
    for (size_t i = 0; i < tree->insn.children_len; i++) {
        size_t res = tree_isel_match_expr_tree_operand(
            profile,
            proto_operands_out,
            tree->insn.children[i],
            &ir_insn->operands[i],
            parent_code
        );
        if (res == 0) {
            return 0;
        }
        total += res;
    }
    return total;
}

// Test whether a candidate instruction prototypes matches the IR.
// Returns how many IR instructions and operands would be replaced by this prototype.
// Returns 0 if the candidate instruction cannot be applied here.
static size_t tree_isel_match_proto(
    backend_profile_t   *profile,
    insn_sub_t const    *proto,
    ir_insn_t const     *ir_insn,
    ir_code_t const     *parent_code,
    ir_operand_t const **ir_operands,
    bool                *ir_to_regs
) {
    if (ir_insn->code != parent_code) {
        return 0;
    }

    // First collect all operands.
    size_t match_size = tree_isel_match_expr_tree_insn(profile, ir_operands, proto->match_tree, ir_insn, parent_code);
    if (!match_size) {
        return 0;
    }

    // Then verify operand rules.
    for (size_t i = 0; i < proto->operands_len; i++) {
        assert(ir_operands[i] != NULL);
        operand_rule_t rule = proto->operands[i];

        // A non-imm location is (also) available.
        bool allow_nonconst = rule.location_kinds.reg || rule.location_kinds.mem_access;

        if (ir_operands[i]->type == IR_OPERAND_TYPE_MEM) {
            // TODO: Validate memory rules.
        } else if (ir_operands[i]->type == IR_OPERAND_TYPE_VAR) {
            // Validate register rules.
            if (!rule.location_kinds.reg) {
                return 0;
            }
            switch (ir_operands[i]->var->prim_type) {
                case IR_PRIM_s8:
                case IR_PRIM_u8:
                case IR_PRIM_s16:
                case IR_PRIM_u16:
                case IR_PRIM_s32:
                case IR_PRIM_u32:
                case IR_PRIM_s64:
                case IR_PRIM_u64:
                case IR_PRIM_s128:
                case IR_PRIM_u128:
                    if (rule.operand_sizes.sizeptr) {
                        rule.operand_sizes.val |= 1u << profile->ptr_bits;
                    }
                    if (rule.operand_sizes.sizeword) {
                        rule.operand_sizes.val |= 1u << profile->gpr_bits;
                    }
                    uint8_t bits_exp    = ir_operands[i]->var->prim_type >> 1;
                    bool    is_unsigned = ir_operands[i]->var->prim_type & 1;
                    if (!(rule.operand_sizes.val & (1u << bits_exp))) {
                        return 0;
                    }
                    if (!(is_unsigned ? rule.operand_kinds.uint : rule.operand_kinds.sint)) {
                        return 0;
                    }
                    break;
                case IR_PRIM_bool:
                    if (!rule.operand_kinds.bool_) {
                        return 0;
                    }
                    break;
                case IR_PRIM_f32:
                    if (!rule.operand_kinds.f32) {
                        return 0;
                    }
                    break;
                case IR_PRIM_f64:
                    if (!rule.operand_kinds.f64) {
                        return 0;
                    }
                    break;
                default: __builtin_unreachable();
            }
        } else if (ir_operands[i]->type == IR_OPERAND_TYPE_CONST) {
            // Validate constant rules.
            bool imm_ok = rule.location_kinds.imm;
            switch (ir_operands[i]->iconst.prim_type) {
                case IR_PRIM_f32:
                    if (!rule.operand_kinds.f32) {
                        imm_ok = false;
                    }
                    break;
                case IR_PRIM_f64:
                    if (!rule.operand_kinds.f64) {
                        imm_ok = false;
                    }
                    break;
                case IR_PRIM_bool:
                    if (!rule.operand_kinds.sint && !rule.operand_kinds.uint) {
                        imm_ok = false;
                    }
                    break;
                case IR_PRIM_s128:
                case IR_PRIM_u128:
                    // TODO: Logic to check 128-bit constants.
                    imm_ok = false;
                    break;
                default: {
                    bool is_unsigned = ir_operands[i]->iconst.prim_type & 1;
                    if (!is_unsigned && !rule.operand_kinds.sint) {
                        imm_ok = false;
                    }

                    int64_t val           = ir_trim_const(ir_operands[i]->iconst).constl;
                    int     unsigned_bits = 64 - __builtin_clzll(val);
                    int     signed_bits   = val < 0 ? 64 - __builtin_clzll(~val) : unsigned_bits + 1;

                    if (is_unsigned && unsigned_bits + !rule.operand_kinds.uint > rule.const_bits) {
                        imm_ok = false;
                    } else if (!is_unsigned && signed_bits > rule.const_bits) {
                        imm_ok = false;
                    }
                } break;
            }

            if (imm_ok) {
                // Give a "bonus point" if it can be stored in the constants successfully over registers.
                match_size += 1;
            } else {
                // Mark it as needing promotion to a register.
                ir_to_regs[i] = true;
                if (!allow_nonconst) {
                    return 0;
                }
            }
        }
    }

    return match_size;
}



// Add instruction selection candidates based on an IR operand.
static void tree_isel_add_candidates_operand(set_t *candidates, cand_tree_t const *tree, ir_operand_t ir_operand) {
    for (cand_tree_t const *cur = tree; cur; cur = cur->next) {
        if ((cur->type == EXPR_TREE_ICONST && ir_operand.type == IR_OPERAND_TYPE_CONST
             && ir_const_lenient_identical(cur->iconst, ir_operand.iconst))
            || cur->type == EXPR_TREE_OPERAND) {
            for (size_t i = 0; i < cur->subs_len; i++) {
                set_add(candidates, cur->subs[i]);
            }
        }
    }

    if (ir_operand.type == IR_OPERAND_TYPE_VAR) {
        set_ent_t const *ent = set_next(&ir_operand.var->assigned_at, NULL);
        if (ent) {
            tree_isel_add_candidates_insn(candidates, tree, ent->value);
        }
    }
}

// Add instruction selection candidates based on an IR instruction.
static void tree_isel_add_candidates_insn(set_t *candidates, cand_tree_t const *tree, ir_insn_t const *ir_insn) {
    for (; tree; tree = tree->next) {
        if (tree->type != EXPR_TREE_IR_INSN || tree->insn.type != ir_insn->type
            || (tree->insn.type == IR_INSN_EXPR1 && tree->insn.op1 != ir_insn->op1)
            || (tree->insn.type == IR_INSN_EXPR2 && tree->insn.op2 != ir_insn->op2)) {
            continue;
        }
        for (size_t i = 0; i < tree->insn.children_len; i++) {
            tree_isel_add_candidates_operand(candidates, tree->insn.children[i], ir_insn->operands[i]);
        }
    }
}



// Match an instruction selection tree against IR in SSA form.
isel_t
    cand_tree_isel(backend_profile_t *profile, cand_tree_t const *tree, ir_insn_t const *ir_insn, size_t operands_cap) {
    set_t candidates = PTR_SET_EMPTY;

    // Walk the tree, adding matching prototypes.
    tree_isel_add_candidates_insn(&candidates, tree, ir_insn);

    // Select the instruction that consumes most IR.
    size_t               max_size     = 0;
    insn_sub_t const    *best_fit     = NULL;
    ir_operand_t const **best_operand = NULL;
    bool                *best_reg     = NULL;

    set_foreach(insn_sub_t, proto, &candidates) {
        ir_operand_t const **operand_tmp = strong_calloc(operands_cap, sizeof(void *));
        bool                *reg_tmp     = strong_calloc(operands_cap, sizeof(bool));
        size_t proto_size = tree_isel_match_proto(profile, proto, ir_insn, ir_insn->code, operand_tmp, reg_tmp);
        if (proto_size > max_size) {
            max_size = proto_size;
            best_fit = proto;
            free(best_operand);
            free(best_reg);
            best_operand = operand_tmp;
            best_reg     = reg_tmp;
        } else {
            free(operand_tmp);
            free(reg_tmp);
        }
    }

    isel_t res = {0};

    if (best_fit) {
        res.sub      = best_fit;
        res.operands = calloc(best_fit->operands_len, sizeof(ir_operand_t));
        for (size_t i = 0; i < best_fit->operands_len; i++) {
            res.operands[i] = *best_operand[i];
        }
        res.operand_regs = best_reg;
        free(best_operand);
    }

    set_clear(&candidates);

    return res;
}
