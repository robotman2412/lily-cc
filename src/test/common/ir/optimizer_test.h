
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "ir.h"
#include "testcase.h"

// These scans deliberately do not use the compiler's reference-walking macros or
// cached use/definition/reference sets to determine what is referenced.
static bool opt_test_has_var(ir_func_t *func, ir_var_t const *target) {
    dlist_foreach_node(ir_var_t, var, &func->vars_list) {
        if (var == target) {
            return true;
        }
    }
    return false;
}

static bool opt_test_has_frame(ir_func_t *func, ir_frame_t const *target) {
    dlist_foreach_node(ir_frame_t, frame, &func->frames_list) {
        if (frame == target) {
            return true;
        }
    }
    return false;
}

static bool opt_test_has_code(ir_func_t *func, ir_code_t const *target) {
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        if (code == target) {
            return true;
        }
    }
    return false;
}

static bool opt_test_operand_reads(ir_operand_t operand, ir_var_t const *var) {
    return (operand.type == IR_OPERAND_TYPE_VAR && operand.var == var)
           || (operand.type == IR_OPERAND_TYPE_MEM && operand.mem.base_type == IR_MEMBASE_VAR
               && operand.mem.base_var == var);
}

static bool opt_test_insn_reads(ir_insn_t *insn, ir_var_t const *var) {
    if (insn->type == IR_INSN_COMBINATOR) {
        for (size_t i = 0; i < insn->combinators_len; i++) {
            if (opt_test_operand_reads(insn->combinators[i].bind, var)) {
                return true;
            }
        }
    } else {
        for (size_t i = 0; i < insn->operands_len; i++) {
            if (opt_test_operand_reads(insn->operands[i], var)) {
                return true;
            }
        }
    }
    return false;
}

static bool opt_test_insn_defines(ir_insn_t *insn, ir_var_t const *var) {
    for (size_t i = 0; i < insn->returns_len; i++) {
        if (insn->returns[i].type == IR_RETVAL_TYPE_VAR && insn->returns[i].dest_var == var) {
            return true;
        }
    }
    return false;
}

static bool opt_test_operand_refs(ir_operand_t operand, ir_frame_t const *frame) {
    return (operand.type == IR_OPERAND_TYPE_STRUCT && operand.struct_frame == frame)
           || (operand.type == IR_OPERAND_TYPE_MEM && operand.mem.base_type == IR_MEMBASE_FRAME
               && operand.mem.base_frame == frame);
}

static bool opt_test_insn_refs(ir_insn_t *insn, ir_frame_t const *frame) {
    for (size_t i = 0; i < insn->returns_len; i++) {
        if (insn->returns[i].type == IR_RETVAL_TYPE_STRUCT && insn->returns[i].dest_struct == frame) {
            return true;
        }
    }
    if (insn->type == IR_INSN_COMBINATOR) {
        for (size_t i = 0; i < insn->combinators_len; i++) {
            if (opt_test_operand_refs(insn->combinators[i].bind, frame)) {
                return true;
            }
        }
    } else {
        for (size_t i = 0; i < insn->operands_len; i++) {
            if (opt_test_operand_refs(insn->operands[i], frame)) {
                return true;
            }
        }
    }
    return false;
}

// Count referencing instructions, rather than operand occurrences, to match set semantics.
static size_t opt_test_reads(ir_func_t *func, ir_var_t const *var) {
    size_t count = 0;
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        dlist_foreach_node(ir_insn_t, insn, &code->insns) {
            count += opt_test_insn_reads(insn, var);
        }
    }
    return count;
}

static size_t opt_test_defs(ir_func_t *func, ir_var_t const *var) {
    size_t count = 0;
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        dlist_foreach_node(ir_insn_t, insn, &code->insns) {
            count += opt_test_insn_defines(insn, var);
        }
    }
    return count;
}

static size_t opt_test_frame_refs(ir_func_t *func, ir_frame_t const *frame) {
    size_t count = 0;
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        dlist_foreach_node(ir_insn_t, insn, &code->insns) {
            count += opt_test_insn_refs(insn, frame);
        }
    }
    return count;
}

static int opt_test_validate_operand(ir_func_t *func, ir_operand_t operand) {
    if (operand.type == IR_OPERAND_TYPE_VAR) {
        TEST_ASSERT(opt_test_has_var(func, operand.var), "%s: operand refers to missing variable", func->name);
    } else if (operand.type == IR_OPERAND_TYPE_STRUCT) {
        TEST_ASSERT(opt_test_has_frame(func, operand.struct_frame), "%s: operand refers to missing frame", func->name);
    } else if (operand.type == IR_OPERAND_TYPE_MEM) {
        if (operand.mem.base_type == IR_MEMBASE_VAR) {
            TEST_ASSERT(opt_test_has_var(func, operand.mem.base_var), "%s: missing address variable", func->name);
        } else if (operand.mem.base_type == IR_MEMBASE_FRAME) {
            TEST_ASSERT(opt_test_has_frame(func, operand.mem.base_frame), "%s: missing address frame", func->name);
        } else if (operand.mem.base_type == IR_MEMBASE_CODE) {
            TEST_ASSERT(opt_test_has_code(func, operand.mem.base_code), "%s: missing target block", func->name);
        }
    }
    TEST_OK();
}

static int opt_test_validate(ir_func_t *func) {
    TEST_ASSERT(opt_test_has_code(func, func->entry), "%s: missing entry block", func->name);
    for (size_t i = 0; i < func->args_len; i++) {
        if (func->args[i].arg_type == IR_FUNCARG_TYPE_VAR) {
            ir_var_t *var = func->args[i].var;
            TEST_ASSERT(opt_test_has_var(func, var), "%s: missing argument variable", func->name);
            TEST_EXPECT_INT(var->arg_index, (ptrdiff_t)i, "%s", func->name);
        } else if (func->args[i].arg_type == IR_FUNCARG_TYPE_STRUCT) {
            ir_frame_t *frame = func->args[i].struct_frame;
            TEST_ASSERT(opt_test_has_frame(func, frame), "%s: missing argument frame", func->name);
            TEST_EXPECT_INT(frame->arg_index, (ptrdiff_t)i, "%s", func->name);
        }
    }
    if (func->retval_ptr) {
        TEST_ASSERT(opt_test_has_var(func, func->retval_ptr), "%s: missing return pointer", func->name);
    }
    if (func->call_frame) {
        TEST_ASSERT(opt_test_has_frame(func, func->call_frame), "%s: missing call frame", func->name);
    }
    if (func->link_frame) {
        TEST_ASSERT(opt_test_has_frame(func, func->link_frame), "%s: missing link frame", func->name);
    }
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        TEST_ASSERT(code->func == func, "%s: wrong block owner", func->name);
        dlist_foreach_node(ir_insn_t, insn, &code->insns) {
            TEST_ASSERT(insn->code == code, "%s: wrong instruction owner", func->name);
            for (size_t i = 0; i < insn->returns_len; i++) {
                if (insn->returns[i].type == IR_RETVAL_TYPE_VAR) {
                    TEST_ASSERT(
                        opt_test_has_var(func, insn->returns[i].dest_var),
                        "%s: missing result variable",
                        func->name
                    );
                } else if (insn->returns[i].type == IR_RETVAL_TYPE_STRUCT) {
                    TEST_ASSERT(
                        opt_test_has_frame(func, insn->returns[i].dest_struct),
                        "%s: missing result frame",
                        func->name
                    );
                }
            }
            if (insn->type == IR_INSN_COMBINATOR) {
                for (size_t i = 0; i < insn->combinators_len; i++) {
                    TEST_ASSERT(
                        opt_test_has_code(func, insn->combinators[i].pred),
                        "%s: missing combinator predecessor",
                        func->name
                    );
                    TEST_EXPECT_INT(opt_test_validate_operand(func, insn->combinators[i].bind), 0);
                }
            } else {
                for (size_t i = 0; i < insn->operands_len; i++) {
                    TEST_EXPECT_INT(opt_test_validate_operand(func, insn->operands[i]), 0);
                }
            }
        }
    }
    dlist_foreach_node(ir_var_t, var, &func->vars_list) {
        TEST_ASSERT(var->func == func, "%s: wrong variable owner", func->name);
        if (var->arg_index >= 0) {
            TEST_COMPARE_UINT((size_t)var->arg_index, <, func->args_len, "%s", func->name);
            TEST_EXPECT_INT(func->args[var->arg_index].arg_type, IR_FUNCARG_TYPE_VAR, "%s", func->name);
            TEST_ASSERT(func->args[var->arg_index].var == var, "%s: wrong argument descriptor", func->name);
        }
        TEST_EXPECT_UINT(var->used_at.len, opt_test_reads(func, var), "%s: cached reads for %s", func->name, var->name);
        TEST_EXPECT_UINT(
            var->assigned_at.len,
            opt_test_defs(func, var),
            "%s: cached definitions for %s",
            func->name,
            var->name
        );
        dlist_foreach_node(ir_code_t, code, &func->code_list) {
            dlist_foreach_node(ir_insn_t, insn, &code->insns) {
                TEST_EXPECT_INT(
                    set_contains(&var->used_at, insn),
                    opt_test_insn_reads(insn, var),
                    "%s: read membership",
                    func->name
                );
                TEST_EXPECT_INT(
                    set_contains(&var->assigned_at, insn),
                    opt_test_insn_defines(insn, var),
                    "%s: definition membership",
                    func->name
                );
            }
        }
    }
    dlist_foreach_node(ir_frame_t, frame, &func->frames_list) {
        TEST_ASSERT(frame->func == func, "%s: wrong frame owner", func->name);
        if (frame->arg_index >= 0) {
            TEST_COMPARE_UINT((size_t)frame->arg_index, <, func->args_len, "%s", func->name);
            TEST_EXPECT_INT(func->args[frame->arg_index].arg_type, IR_FUNCARG_TYPE_STRUCT, "%s", func->name);
            TEST_ASSERT(
                func->args[frame->arg_index].struct_frame == frame,
                "%s: wrong frame argument descriptor",
                func->name
            );
        }
        TEST_EXPECT_UINT(
            frame->ref_at.len,
            opt_test_frame_refs(func, frame),
            "%s: cached frame references for %s",
            func->name,
            frame->name
        );
        dlist_foreach_node(ir_code_t, code, &func->code_list) {
            dlist_foreach_node(ir_insn_t, insn, &code->insns) {
                TEST_EXPECT_INT(
                    set_contains(&frame->ref_at, insn),
                    opt_test_insn_refs(insn, frame),
                    "%s: frame reference membership",
                    func->name
                );
            }
        }
    }
    TEST_OK();
}
