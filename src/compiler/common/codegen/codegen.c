
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "codegen.h"

#include "ir.h"
#include "ir_types.h"
#include "list.h"

#include <assert.h>



// Remove jumps that go the the next code block linearly.
static void cg_remove_jumps(ir_func_t *func) {
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        ir_flow_t *last_insn = (ir_flow_t *)code->insns.tail;
        if (last_insn && last_insn->base.type == IR_INSN_FLOW && last_insn->type == IR_FLOW_JUMP
            && last_insn->f_jump.target == (void *)code->node.next) {
            ir_insn_delete((ir_insn_t *)last_insn);
        }
    }
}

// Select machine instructions for all IR instructions.
static void cg_isel(backend_profile_t *profile, ir_code_t *code) {
}

// Convert an SSA-form IR function completely into executable machine code.
// All IR instructions are replaced, code order is decided by potentially re-ordering the code blocks from the
// functions, and unnecessary jumps are removed. When finished, the code blocks and instructions therein will be in
// order as written to the eventual executable file.
void codegen(backend_profile_t *profile, ir_func_t *func) {
    assert(func->enforce_ssa);
    cg_remove_jumps(func);
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        cg_isel(profile, code);
    }
}
