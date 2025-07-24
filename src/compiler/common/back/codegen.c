
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "codegen.h"

#include "cand_tree.h"
#include "insn_proto.h"
#include "ir.h"
#include "ir_types.h"
#include "list.h"

#include <assert.h>



// Remove jumps that go the the next code block linearly.
static void cg_remove_jumps(ir_func_t *func) {
    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        ir_insn_t *last_insn = (ir_insn_t *)code->insns.tail;
        if (last_insn && last_insn->type == IR_INSN_JUMP
            && last_insn->operands[0].mem.base_code == (void *)code->node.next) {
            ir_insn_delete((ir_insn_t *)last_insn);
        }
    }
}

// Select machine instructions for all IR instructions.
static void cg_isel(backend_profile_t *profile, ir_code_t *code) {
    ir_insn_t *cur = container_of(code->insns.tail, ir_insn_t, node);
    while (cur) {
        ir_operand_t        params[8];
        insn_proto_t const *proto = profile->backend->isel(profile, cur, params);
        assert(proto != NULL);
        ir_insn_t *mach = insn_proto_substitute(proto, cur, params);
        cur             = container_of(mach->node.previous, ir_insn_t, node);
    }
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
