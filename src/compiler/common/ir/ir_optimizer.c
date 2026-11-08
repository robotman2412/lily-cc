
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir/ir_optimizer.h"

#include "arith128.h"
#include "assert.h"
#include "ir.h"
#include "ir/ir_interpreter.h"
#include "ir_serialization.h"
#include "ir_types.h"
#include "list.h"
#include "set.h"
#include "unreachable.h"
#include "vec.h"

#include <stdint.h>
#include <stdio.h>



// Run optimizations on some IR.
// Returns whether any code was changed.
bool ir_optimize(ir_func_t *func) {
    bool changed = false, loop;

    // Optimization passes that affect each other.
    do {
        loop     = false;
        loop    |= opt_const_prop(func);
        loop    |= opt_unused_vars(func);
        loop    |= opt_dead_code(func);
        loop    |= opt_branches(func);
        changed |= loop;
    } while (loop);

    // Standalone optimization passes.
    changed |= opt_strength_reduce(func);

    return changed;
}



static inline void mem2reg_replace_load(
    ir_func_t *func, ir_insn_t *insn, ir_var_t *replacement, ir_prim_t replacement_type, unsigned int offset
) {
    ir_insnloc_t loc         = IR_AFTER_INSN(insn);
    ir_sign_t    sign        = insn->operands[0].sign;
    ir_prim_t    access_type = insn->operands[0].mem.data_type;

    ir_var_t *shifted = replacement;
    if (offset != 0) {
        shifted          = ir_var_create(func, replacement_type, NULL);
        ir_const_t shamt = {
            .prim_type = replacement_type,
            .const128  = ui128(offset * 8),
        };
        loc = IR_AFTER_INSN(
            ir_add_expr2(loc, IR_RETVAL_VAR(shifted), IR_OP2_shru, IR_OPERAND_VAR(replacement), IR_OPERAND_CONST(shamt))
        );
    }

    if (ir_prim_is_float(access_type)) {
        ir_var_t *truncated = shifted;
        if (ir_prim_size(replacement_type) != ir_prim_size(access_type)) {
            truncated = ir_var_create(func, access_type == IR_PRIM_f64 ? IR_PRIM_i64 : IR_PRIM_i32, NULL);
            loc = IR_AFTER_INSN(ir_add_expr1(loc, IR_RETVAL_VAR(truncated), IR_OP1_zeroext, IR_OPERAND_VAR(shifted)));
        }

        ir_add_expr1(loc, insn->returns[0], IR_OP1_bitcast, IR_OPERAND_VAR(truncated));
    } else if (sign == IR_SIGN_SIGNEXT) {
        ir_add_expr1(loc, insn->returns[0], IR_OP1_signext, IR_OPERAND_VAR(shifted));
    } else {
        ir_add_expr1(loc, insn->returns[0], IR_OP1_zeroext, IR_OPERAND_VAR(shifted));
    }

    ir_insn_delete(insn);
}

static inline void mem2reg_replace_store(
    ir_func_t *func, ir_insn_t *insn, ir_var_t *replacement, ir_prim_t replacement_type, unsigned int offset
) {
    ir_insnloc_t loc         = IR_AFTER_INSN(insn);
    ir_prim_t    access_type = insn->operands[0].mem.data_type;

    // Convert into replacement format.
    ir_operand_t cast = insn->operands[1];
    if (ir_operand_prim(cast) != replacement_type) {
        ir_var_t *tmpvar = ir_var_create(func, replacement_type, NULL);
        if (ir_prim_is_float(ir_operand_prim(cast))) {
            loc = IR_AFTER_INSN(ir_add_expr1(loc, IR_RETVAL_VAR(tmpvar), IR_OP1_bitcast, cast));
        } else {
            loc = IR_AFTER_INSN(ir_add_expr1(loc, IR_RETVAL_VAR(tmpvar), IR_OP1_zeroext, cast));
        }
        cast = IR_OPERAND_VAR(tmpvar);
    }


    if (offset != 0 || ir_prim_size(replacement_type) != ir_prim_size(access_type)) {
        // Shift into place according to offset.
        ir_operand_t shifted = cast;
        if (offset != 0) {
            ir_var_t  *tmpvar = ir_var_create(func, replacement_type, NULL);
            ir_const_t shamt  = {
                .prim_type = replacement_type,
                .const128  = ui128(offset * 8),
            };
            loc = IR_AFTER_INSN(ir_add_expr2(loc, IR_RETVAL_VAR(tmpvar), IR_OP2_shl, cast, IR_OPERAND_CONST(shamt)));
            shifted = IR_OPERAND_VAR(tmpvar);
        }

        // Masked store into replacement.
        i128_t data_mask;
        switch (access_type) {
            case IR_PRIM_bool:
            case IR_PRIM_i8: data_mask = ui128(0xffu); break;
            case IR_PRIM_i16: data_mask = ui128(0xffffu); break;
            case IR_PRIM_f32:
            case IR_PRIM_i32: data_mask = ui128(0xffffffffu); break;
            case IR_PRIM_f64:
            case IR_PRIM_i64: data_mask = ui128(UINT64_MAX); break;
            case IR_PRIM_i128: data_mask = UI128_MAX; break;
            default: UNREACHABLE();
        }
        i128_t     cutout_mask = bneg128(shl128(data_mask, offset * 8));
        ir_const_t mask        = {.prim_type = replacement_type, .const128 = cutout_mask};

        loc = IR_AFTER_INSN(ir_add_expr2(
            loc,
            IR_RETVAL_VAR(replacement),
            IR_OP2_band,
            IR_OPERAND_VAR(replacement),
            IR_OPERAND_CONST(mask)
        ));
        ir_add_expr2(loc, IR_RETVAL_VAR(replacement), IR_OP2_bor, IR_OPERAND_VAR(replacement), shifted);

    } else {
        // Direct store to replacement.
        ir_add_expr1(loc, IR_RETVAL_VAR(replacement), IR_OP1_mov, cast);
    }

    ir_insn_delete(insn);
}

static bool mem2reg_frame(ir_func_t *func, ir_frame_t *frame) {
    if (frame->ref_at.len == 0) {
        // Nothing to change due to no instruction references.
        return false;
    }
    if (frame->arg_index >= 0) {
        // TODO: Could possibly be mem2reg'd with one store, but that requires a better algorithm.
        return false;
    }

    ir_prim_t max_int = IR_PRIM_bool;
    bool      has_int = false; // i8-i128 and bool, or offset != 0
    bool      has_f32 = false;
    bool      has_f64 = false;

    // The minimum and maximum offsets (both inclusive) that would be accessed.
    int64_t min_off = INT64_MAX;
    int64_t max_off = INT64_MIN;

    // Test for disqualifying references.
    vec_ir_insn_t insns = VEC_INIT;
    set_foreach(ir_insn_t, insn, &frame->ref_at) {
        switch (insn->type) {
            case IR_INSN_LOAD:
            case IR_INSN_STORE: {
                assert(
                    insn->operands[0].type == IR_OPERAND_TYPE_MEM && insn->operands[0].mem.base_type == IR_MEMBASE_FRAME
                );

                // Calculate min/max offsets for load/store.
                int64_t   offset = insn->operands[0].mem.offset;
                ir_prim_t data   = insn->operands[0].mem.data_type;
                int64_t   size   = ir_prim_size(data);
                if (INT64_MAX - size + 1 <= offset) {
                    // Offset would overflow.
                    goto fail;
                }
                int64_t end_offset = offset - 1 + size;
                if (min_off > offset) {
                    min_off = offset;
                }
                if (max_off < end_offset) {
                    max_off = end_offset;
                }

                // Loads/stores can be replaced automatically.
                if (ir_prim_is_integer(data) || data == IR_PRIM_bool) {
                    if (ir_prim_bits(max_int) < ir_prim_bits(data)) {
                        max_int = data;
                    }
                    has_int = true;
                } else if (data == IR_PRIM_f32) {
                    has_f32 = true;
                } else if (data == IR_PRIM_f64) {
                    has_f64 = true;
                } else {
                    fflush(stdout);
                    fprintf(stderr, "BUG: Load/store instruction has no data type:\n    ");
                    ir_insn_serialize(insn, NULL, stderr);
                    fprintf(stderr, "\n");
                    abort();
                }
                vec_push(&insns, insn);
            } break;
            default:
                // Anything else, especially `lea`, cannot be replaced automatically.
                // TODO: Operations such as `memset`, `memcpy` and `call` can be replaced in certain circumstances.
                goto fail;
        }
    }

    /*
    Together, `has_*`, `min_off` and `max_off` determine what primitive type the replacement variable is.

    If `min_off` != `max_off`, then the smallest integer type that will fit it is used.
    If `i128` can't store it, then mem2reg is aborted.

    Otherwise, if `has_int`, then the largest integer type loaded/stored is used.

    Otherwise, if `has_f64`, then `f64` is used.

    Otherwise, if `has_f32`, then `f32` is used.

    Otherwise, an assertion is raised as having no data types is a precondition violation on load/store.
    */
    ir_prim_t replacement_type;
    if (max_off > min_off + 15) {
        goto fail; // Size too large to optimize.
    }
    uint64_t size = max_off - min_off + 1;
    if (min_off != max_off) {
        if (size == 2) {
            replacement_type = IR_PRIM_i16;
        } else if (size <= 4) {
            replacement_type = IR_PRIM_i32;
        } else if (size <= 4) {
            replacement_type = IR_PRIM_i64;
        } else {
            replacement_type = IR_PRIM_i128;
        }
    } else if (has_int) {
        replacement_type = max_int;
    } else if (has_f64) {
        replacement_type = IR_PRIM_f64;
    } else if (has_f32) {
        replacement_type = IR_PRIM_f32;
    } else {
        UNREACHABLE();
    }

    ir_var_t *replacement = ir_var_create(func, replacement_type, NULL);

    // Commence mem2reg of all loads and stores.
    // TODO: There is no proper handling for big-endian values in the load/store replacement helpers.
    for (size_t i = 0; i < insns.len; i++) {
        ir_insn_t *insn = insns.arr[i];
        switch (insn->type) {
            case IR_INSN_LOAD:
                mem2reg_replace_load(func, insn, replacement, replacement_type, insn->operands[0].mem.offset - min_off);
                break;
            case IR_INSN_STORE:
                mem2reg_replace_store(
                    func,
                    insn,
                    replacement,
                    replacement_type,
                    insn->operands[0].mem.offset - min_off
                );
                break;
            default: UNREACHABLE();
        }
    }
    vec_clear(&insns);
    return true;

fail:
    vec_clear(&insns);
    return false;
}

// Optimization: Replace trivial stack frame accesses with copy instructions.
// Returns whether any code was changed.
bool opt_mem2reg(ir_func_t *func) {
    assert(func->enforce_ssa);
    func->enforce_ssa = false; // The replacement variables could cause non-SSA code.

    bool changed = false;
    dlist_foreach_node(ir_frame_t, frame, &func->frames_list) {
        changed |= mem2reg_frame(func, frame);
    }

    if (changed) {
        ir_func_to_ssa(func); // Fix replacement variables back into SSA form.
    } else {
        func->enforce_ssa = true;
    }

    return changed;
}

// Try to strength-reduce a single expression.
static bool strength_reduce_expr(ir_insn_t *expr) {
    if (expr->type != IR_INSN_EXPR2) {
        // The strength-reduced expressions are all binary exprs.
        return false;
    }

#define prim expr->returns[0].dest_var->prim_type
#define lhs  expr->operands[0]
#define rhs  expr->operands[1]
#define oper expr->op2

    if (prim == IR_PRIM_f32 || prim == IR_PRIM_f64) {
        return false;
    }

    if (oper == IR_OP2_mul && lhs.type == IR_OPERAND_TYPE_CONST && rhs.type != IR_OPERAND_TYPE_CONST) {
        ir_operand_t tmp = lhs;
        lhs              = rhs;
        rhs              = tmp;
    }
    if (rhs.type != IR_OPERAND_TYPE_CONST) {
        return false;
    }
    rhs.iconst       = ir_trim_const(rhs.iconst, true);
    ir_const_t p_rhs = ir_const_is_negative(rhs.iconst) ? ir_calc1(IR_OP1_neg, rhs.iconst) : rhs.iconst;

    if ((oper == IR_OP2_divu || oper == IR_OP2_divu) && ir_const_popcnt(rhs.iconst) == 1
        && !ir_const_is_negative(rhs.iconst)) {
        // Replace a division with a right shift.
        rhs.iconst.constl = ir_const_ctz(rhs.iconst);
        rhs.iconst.consth = 0;
        oper              = oper == IR_OP2_divu ? IR_OP2_shru : IR_OP2_shrs;
        return true;

    } else if ((oper == IR_OP2_remu || oper == IR_OP2_remu) && ir_const_popcnt(p_rhs) == 1) {
        // Replace a remainder with a bitmask.
        int    bits = ir_const_ctz(p_rhs);
        i128_t mask = add128(i128_pack(-1, -1), shl128(i128_pack(0, 1), bits));
        if (!(prim & 1)) {
            ir_op2_type_t shr = oper == IR_OP2_divu ? IR_OP2_shru : IR_OP2_shrs;

            // Add instructions to extract and copy the sign from the lhs to the output.
            ir_var_t *tmp1 = ir_var_create(expr->code->func, prim, NULL);
            ir_var_t *tmp2 = ir_var_create(expr->code->func, prim, NULL);
            ir_var_t *tmp3 = ir_var_create(expr->code->func, prim, NULL);
            ir_var_t *tmp4 = ir_var_create(expr->code->func, prim, NULL);

            ir_const_t shamt1 = {.prim_type = prim, .constl = ir_prim_size(prim) * 8 - 1};
            ir_add_expr2(IR_BEFORE_INSN(expr), IR_RETVAL_VAR(tmp1), shr, lhs, IR_OPERAND_CONST(shamt1));

            ir_const_t shamt2 = {.prim_type = prim, .constl = ir_prim_size(prim) * 8 - bits};
            ir_add_expr2(
                IR_BEFORE_INSN(expr),
                IR_RETVAL_VAR(tmp2),
                shr,
                IR_OPERAND_VAR(tmp1),
                IR_OPERAND_CONST(shamt2)
            );

            ir_add_expr2(IR_BEFORE_INSN(expr), IR_RETVAL_VAR(tmp3), IR_OP2_add, lhs, IR_OPERAND_VAR(tmp2));

            ir_retval_t dest = expr->returns[0];
            ir_insn_set_return(expr, 0, IR_RETVAL_VAR(tmp4));

            ir_add_expr2(IR_AFTER_INSN(expr), dest, IR_OP2_sub, IR_OPERAND_VAR(tmp4), IR_OPERAND_VAR(tmp2));
        }
        rhs.iconst.const128 = mask;
        oper                = IR_OP2_band;
        return true;

    } else if (oper == IR_OP2_mul && ir_const_popcnt(rhs.iconst) == 1 && !ir_const_is_negative(rhs.iconst)) {
        // Replace multiplication with a left shift.
        rhs.iconst.constl = ir_const_ctz(rhs.iconst);
        rhs.iconst.consth = 0;
        oper              = IR_OP2_shl;
        return true;
    }

#undef prim
#undef lhs
#undef rhs
#undef oper

    return false;
}

// Optimization: Strength reduction; replaces expensive with cheaper arithmetic where available.
bool opt_strength_reduce(ir_func_t *func) {
    bool reduced = false, loop;
    do {
        loop          = false;
        ir_var_t *var = (ir_var_t *)func->vars_list.head;
        while (var) {
            ir_var_t *next = (ir_var_t *)var->node.next;
            if (var->assigned_at.len != 1) {
                var = next;
                continue;
            }
            ir_insn_t *insn  = set_next(&var->assigned_at, NULL)->value;
            loop            |= strength_reduce_expr(insn);
            reduced         |= loop;
            var              = next;
        }
    } while (loop);
    return reduced;
}

static void mark_used_dfs(ir_var_t *var) {
    if (var->visited) {
        return;
    }
    var->visited = true;
    assert(var->assigned_at.len <= 1);
    if (var->assigned_at.len == 0) {
        return;
    }
    ir_insn_t *assign = set_next(&var->assigned_at, NULL)->value;

    if (assign->type != IR_INSN_COMBINATOR) {
        return;
    }

    for (size_t i = 0; i < assign->combinators_len; i++) {
        IR_FOR_OPERAND_VARS(assign->combinators[i].bind, var, mark_used_dfs(var););
    }
}

static void check_var_used(ir_var_t *var) {
    if (var->visited || !var->used_at.len) {
        return;
    }

    set_foreach(ir_insn_t, insn, &var->assigned_at) {
        if (insn->flags & IR_INSN_FLAG_SIDE_EFFECTS) {
            mark_used_dfs(var);
            return;
        }
    }

    set_foreach(ir_insn_t, insn, &var->used_at) {
        if (insn->type != IR_INSN_COMBINATOR) {
            mark_used_dfs(var);
            return;
        }
    }
}

// Optimization: Delete all variables and assignments to them whose value is never read.
// Returns whether any variables were deleted.
bool opt_unused_vars(ir_func_t *func) {
    bool deleted = false, loop;

    do {
        loop = false;
        dlist_foreach_node(ir_var_t, var, &func->vars_list) {
            var->visited = false;
        }

        dlist_foreach_node(ir_var_t, var, &func->vars_list) {
            check_var_used(var);
        }

        ir_var_t *var = container_of(func->vars_list.head, ir_var_t, node);
        while (var) {
            ir_var_t *next = container_of(var->node.next, ir_var_t, node);
            if (!var->visited) {
                deleted = true;
                loop    = true;
                ir_var_delete(var);
            }
            var = next;
        }
    } while (loop);

    return deleted;
}

// Optimization: Delete unused stack frames.
// Returns whether any frames were deleted.
bool opt_unused_frames(ir_func_t *func) {
    bool        deleted = false;
    ir_frame_t *frame   = (ir_frame_t *)func->frames_list.head;
    while (frame) {
        ir_frame_t *next = (ir_frame_t *)frame->node.next;
        if (!frame->ref_at.len && frame->arg_index < 0) {
            ir_frame_delete(frame);
            deleted = true;
        }
        frame = next;
    }

    return deleted;
}



// Dead code optimization depth-first search.
// Returns whether any code was changed or removed.
static bool dead_code_dfs(ir_code_t *code) {
    if (code->visited) {
        return false;
    }
    code->visited = true;

    // Walk instructions in this code block.
    bool       dead = false, changed = false;
    ir_insn_t *insn = container_of(code->insns.head, ir_insn_t, node);
    while (insn) {
        ir_insn_t *next = container_of(insn->node.next, ir_insn_t, node);
        if (dead) {
            // If we're in dead code, delete all instructions.
            changed = true;
            ir_insn_delete(insn);
        } else if (insn->type == IR_INSN_JUMP) {
            // If this is a jump, all following instructions will be dead.
            dead     = true;
            changed |= dead_code_dfs(insn->operands[0].mem.base_code);
        } else if (insn->type == IR_INSN_RETURN) {
            // If this is a return, all following instructions will be dead.
            dead = true;
        } else if (insn->type == IR_INSN_BRANCH) {
            if (insn->operands[1].type == IR_OPERAND_TYPE_CONST) {
                if (insn->operands[1].iconst.constl & 1) {
                    // If this is a branch with constant condition true, all following instructions will be dead.
                    dead     = true;
                    changed |= dead_code_dfs(insn->operands[0].mem.base_code);
                } else {
                    // If this is a branch with constant condition false, delete it.
                    ir_insn_delete(insn);
                }
            } else {
                // Check the potential branch target.
                changed |= dead_code_dfs(insn->operands[0].mem.base_code);
            }
        }

        insn = next;
    }

    return changed;
}

// Optimization: Delete code from dead paths.
// Returns whether any code was changed or removed.
bool opt_dead_code(ir_func_t *func) {
    bool changed = false, loop;
    do {
        // Mark code as not reachable.
        dlist_foreach_node(ir_code_t, code, &func->code_list) {
            code->visited = false;
        }

        // Walk the code for reachability.
        loop = dead_code_dfs(container_of(func->code_list.head, ir_code_t, node));

        // Delete all unreachable code blocks.
        ir_code_t *code = container_of(func->code_list.head, ir_code_t, node);
        while (code) {
            ir_code_t *next = container_of(code->node.next, ir_code_t, node);
            if (!code->visited) {
                ir_code_delete(code);
            }
            code = next;
        }

        // Recalculate predecessors and successors of the remaining graph.
        ir_func_recalc_flow(func);
        changed |= loop;
    } while (loop);
    return changed;
}



// Try to constant-propagate a single expression.
static bool const_prop_expr(ir_insn_t *expr) {
    if (expr->returns[0].type != IR_RETVAL_TYPE_VAR) {
        return false;
    }
    for (size_t i = 0; i < expr->operands_len; i++) {
        if (expr->operands[i].type != IR_OPERAND_TYPE_CONST && expr->operands[i].type != IR_OPERAND_TYPE_VAR) {
            return false;
        }
    }

    // Check for cast from bool to int and then back.
    // This optimization is specifically useful for the C frontend.
    if (expr->type == IR_INSN_EXPR1 && expr->op1 == IR_OP1_snez && expr->operands[0].type == IR_OPERAND_TYPE_VAR
        && expr->operands[0].var->assigned_at.len == 1) {
        ir_insn_t *pred = set_next(&expr->operands[0].var->assigned_at, NULL)->value;
        if (pred->type == IR_INSN_EXPR1 && pred->op1 == IR_OP1_mov && pred->operands[0].type == IR_OPERAND_TYPE_VAR
            && pred->operands[0].var->prim_type == IR_PRIM_bool) {
            ir_var_replace(expr->returns[0].dest_var, pred->operands[0]);
            ir_var_delete(expr->returns[0].dest_var);
            return true;
        }
    }

    if (expr->type == IR_INSN_COMBINATOR) {
        // Flatten phi-nodes with only a single predecessor or all identical bindings.
        for (size_t i = 1; i < expr->combinators_len; i++) {
            if (!ir_operand_identical(expr->combinators[0].bind, expr->combinators[i].bind)) {
                return false;
            }
        }
        ir_var_replace(expr->returns[0].dest_var, expr->combinators[0].bind);
        ir_var_delete(expr->returns[0].dest_var);
        return true;

    } else if (expr->type == IR_INSN_EXPR1 && expr->operands[0].type == IR_OPERAND_TYPE_CONST) {
        // Calculate unary expression at compile time.
        ir_const_t iconst;
        if (expr->op1 == IR_OP1_mov) {
            iconst = expr->operands[0].iconst;
        } else if (expr->op1 == IR_OP1_zeroext) {
            iconst = ir_cast(expr->returns[0].dest_var->prim_type, expr->operands[0].iconst, false);
        } else if (expr->op1 == IR_OP1_signext) {
            iconst = ir_cast(expr->returns[0].dest_var->prim_type, expr->operands[0].iconst, true);
        } else {
            iconst = ir_calc1(expr->op1, expr->operands[0].iconst);
        }
        ir_var_replace(expr->returns[0].dest_var, IR_OPERAND_CONST(iconst));
        ir_var_delete(expr->returns[0].dest_var);
        return true;

    } else if (
        expr->type == IR_INSN_EXPR2 && expr->operands[0].type == IR_OPERAND_TYPE_CONST
        && expr->operands[1].type == IR_OPERAND_TYPE_CONST
    ) {
        // Calculate binary expression at compile time.
        ir_const_t iconst = ir_calc2(expr->op2, expr->operands[0].iconst, expr->operands[1].iconst);
        ir_var_replace(expr->returns[0].dest_var, IR_OPERAND_CONST(iconst));
        ir_var_delete(expr->returns[0].dest_var);
        return true;

    } else if (
        expr->type == IR_INSN_EXPR1 && expr->operands[0].type == IR_OPERAND_TYPE_VAR
        && expr->returns[0].dest_var->prim_type == expr->operands[0].var->prim_type
        && (expr->op1 == IR_OP1_mov
            // Second clause checks for casting bool to bool again.
            || (expr->op1 == IR_OP1_snez && expr->returns[0].dest_var->prim_type == IR_PRIM_bool))
    ) {
        // Move between two variables of the same type; replace the destination.
        ir_var_replace(expr->returns[0].dest_var, IR_OPERAND_VAR(expr->operands[0].var));
        ir_var_delete(expr->returns[0].dest_var);
        return true;

    } else if (
        expr->type == IR_INSN_EXPR2 && expr->op2 == IR_OP2_mul
        && ((expr->operands[1].type == IR_OPERAND_TYPE_CONST && expr->operands[1].iconst.consth == 0
             && expr->operands[1].iconst.constl == 0)
            || (expr->operands[0].type == IR_OPERAND_TYPE_CONST && expr->operands[0].iconst.consth == 0
                && expr->operands[0].iconst.constl == 0))
    ) {
        // Multiply by zero; replace with constant zero.
        ir_var_replace(
            expr->returns[0].dest_var,
            (ir_operand_t){
                .type   = IR_OPERAND_TYPE_CONST,
                .iconst = {
                    .prim_type = expr->returns[0].dest_var->prim_type,
                    .constl    = 0,
                    .consth    = 0,
                },
            }
        );
        ir_var_delete(expr->returns[0].dest_var);
        return true;

    } else if (
        expr->type == IR_INSN_EXPR2 && (expr->op2 == IR_OP2_mul || expr->op2 == IR_OP2_divu)
        && expr->operands[1].type == IR_OPERAND_TYPE_CONST && expr->operands[1].iconst.consth == 0
        && expr->operands[1].iconst.constl == 1
    ) {
        // Multiply / divide by one (rhs version); replace with variable.
        ir_var_replace(expr->returns[0].dest_var, expr->operands[0]);
        ir_var_delete(expr->returns[0].dest_var);
        return true;

    } else if (
        expr->type == IR_INSN_EXPR2 && expr->op2 == IR_OP2_mul && expr->operands[0].type == IR_OPERAND_TYPE_CONST
        && expr->operands[0].iconst.consth == 0 && expr->operands[0].iconst.constl == 1
    ) {
        // Multiply by one (lhs version); replace with variable.
        ir_var_replace(expr->returns[0].dest_var, expr->operands[1]);
        ir_var_delete(expr->returns[0].dest_var);
        return true;

    } else {
        return false;
    }
}

// Optimization: Propagate constants and useless copies.
// Returns whether any code was changed.
bool opt_const_prop(ir_func_t *func) {
    bool propagated = false, loop;
    do {
        loop          = false;
        ir_var_t *var = container_of(func->vars_list.head, ir_var_t, node);
        while (var) {
            ir_var_t *next = container_of(var->node.next, ir_var_t, node);
            if (var->assigned_at.len != 1) {
                var = next;
                continue;
            }
            set_ent_t const *ent  = set_next(&var->assigned_at, NULL);
            loop                 |= const_prop_expr(ent->value);
            propagated           |= loop;
            var                   = next;
        }
    } while (loop);
    return propagated;
}



// Combine two code blocks end-to-end.
static void merge_code(ir_code_t *first, ir_code_t *second) {
    // The very last instruction should be the one and only jump.
    ir_insn_t *last_jmp = container_of(first->insns.tail, ir_insn_t, node);
    assert(last_jmp);
    assert(last_jmp->type == IR_INSN_JUMP || last_jmp->type == IR_INSN_BRANCH);
    if (last_jmp->type == IR_INSN_JUMP) {
        assert(last_jmp->operands[0].mem.base_code == second);
    } else {
        assert(last_jmp->operands[0].mem.base_code == second);
        assert(last_jmp->operands[1].type == IR_OPERAND_TYPE_CONST);
        assert(last_jmp->operands[1].iconst.prim_type == IR_PRIM_bool);
        assert(last_jmp->operands[1].iconst.constl & 1);
    }
    ir_insn_delete((ir_insn_t *)last_jmp);

    // Transfer all instructions from second to first.
    dlist_foreach_node(ir_insn_t, insn, &second->insns) {
        insn->code = first;
    }
    dlist_concat(&first->insns, &second->insns);

    // Update predecessor-successor relations.
    set_foreach(ir_code_t, succ, &second->succ) {
        set_remove(&succ->pred, second);
        set_add(&succ->pred, first);
    }
    set_clear(&first->succ);
    set_clear(&second->pred);
    first->succ  = second->succ;
    second->succ = PTR_SET_EMPTY;

    // Delete the now empty second node.
    ir_code_delete(second);
}

// Depth-first search function that optimizes branches.
// Returns whether any code was changed.
static bool branch_opt_dfs(ir_code_t *code) {
    if (code->visited) {
        return false;
    }
    code->visited = true;

    bool changed = false;
    while (code->succ.len == 1) {
        ir_code_t *succ = set_next(&code->succ, NULL)->value;
        if (succ != code && succ->pred.len == 1) {
            // If this is a 1:1 link, combine into one block.
            merge_code(code, succ);
            changed = true;
        } else {
            // Cannot merge here.
            break;
        }
    }

    set_foreach(ir_code_t, succ, &code->succ) {
        // Recursively check successors.
        changed |= branch_opt_dfs(succ);
    }

    return changed;
}

// Optimization: Remove redundant branches.
// Returns whether any code was changed.
bool opt_branches(ir_func_t *func) {
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        code->visited = false;
    }
    return branch_opt_dfs(container_of(func->code_list.head, ir_code_t, node));
}
