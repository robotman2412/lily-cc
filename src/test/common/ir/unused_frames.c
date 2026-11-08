
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir.h"
#include "ir_optimizer.h"
#include "optimizer_test.h"
#include "testcase.h"

#include <string.h>



static ir_frame_t *find_frame(ir_func_t *func, char const *name) {
    dlist_foreach_node(ir_frame_t, frame, &func->frames_list) {
        if (!strcmp(frame->name, name)) {
            return frame;
        }
    }
    return NULL;
}

// Inspect the descriptors themselves instead of treating arg_index as the oracle.
static size_t argument_references(ir_func_t *func, ir_frame_t *frame) {
    size_t count = 0;
    for (size_t i = 0; i < func->args_len; i++) {
        if (func->args[i].arg_type == IR_FUNCARG_TYPE_STRUCT && func->args[i].struct_frame == frame) {
            count++;
        }
    }
    return count;
}

static int remove_empty_frames(void) {
    ir_func_t *func = ir_func_create("remove_empty_frames", "entry", 0);
    // Adjacent deletions exercise all positions, including the list becoming empty.
    ir_frame_create(func, 4, 4, "empty_first");
    ir_frame_create(func, 8, 8, "empty_middle");
    ir_frame_create(func, 16, 16, "empty_last");
    ir_add_return0(IR_APPEND(func->entry));
    dlist_foreach_node(ir_frame_t, frame, &func->frames_list) {
        TEST_EXPECT_UINT(opt_test_frame_refs(func, frame), 0u);
        TEST_EXPECT_UINT(argument_references(func, frame), 0u);
    }
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_ASSERT(opt_unused_frames(func));
    TEST_EXPECT_UINT(func->frames_list.len, 0u);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_ASSERT(!opt_unused_frames(func));
    ir_func_delete(func);
    TEST_OK();
}

static int retain_memory_references(void) {
    ir_func_t *func = ir_func_create("retain_memory_references", "entry", 0);
    ir_frame_create(func, 4, 4, "unused_before");
    ir_frame_t *load_frame = ir_frame_create(func, 4, 4, "load_frame");
    ir_frame_create(func, 4, 4, "unused_between0");
    ir_frame_t *store_frame = ir_frame_create(func, 4, 4, "store_frame");
    ir_frame_create(func, 4, 4, "unused_between1");
    ir_frame_t *lea_frame = ir_frame_create(func, 4, 4, "lea_frame");
    ir_frame_create(func, 4, 4, "unused_after");
    ir_var_t *value   = ir_var_create(func, IR_PRIM_i32, "value");
    ir_var_t *address = ir_var_create(func, IR_PRIM_i64, "address");
    // Initialize the loaded bytes; this test is about reference retention, not undefined values.
    ir_add_store(
        IR_APPEND(func->entry),
        IR_OPERAND_CONST(IR_CONST_U32(17)),
        IR_MEMREF(IR_PRIM_i32, IR_BADDR_FRAME(load_frame))
    );
    ir_add_load(IR_APPEND(func->entry), IR_RETVAL_VAR(value), IR_MEMREF(IR_PRIM_i32, IR_BADDR_FRAME(load_frame)));
    ir_add_store(IR_APPEND(func->entry), IR_OPERAND_VAR(value), IR_MEMREF(IR_PRIM_i32, IR_BADDR_FRAME(store_frame)));
    ir_add_lea_stack(IR_APPEND(func->entry), IR_RETVAL_VAR(address), lea_frame, 0);
    func->rettype.type      = IR_FUNCRET_PRIM;
    func->rettype.prim_type = IR_PRIM_i64;
    ir_add_return1(IR_APPEND(func->entry), IR_OPERAND_VAR(address));
    TEST_EXPECT_UINT(opt_test_frame_refs(func, load_frame), 2u);
    TEST_EXPECT_UINT(opt_test_frame_refs(func, store_frame), 1u);
    TEST_EXPECT_UINT(opt_test_frame_refs(func, lea_frame), 1u);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_ASSERT(opt_unused_frames(func));
    TEST_EXPECT_UINT(func->frames_list.len, 3u);
    TEST_ASSERT(find_frame(func, "load_frame"));
    TEST_ASSERT(find_frame(func, "store_frame"));
    TEST_ASSERT(find_frame(func, "lea_frame"));
    TEST_ASSERT(!find_frame(func, "unused_before"));
    TEST_ASSERT(!find_frame(func, "unused_between0"));
    TEST_ASSERT(!find_frame(func, "unused_between1"));
    TEST_ASSERT(!find_frame(func, "unused_after"));
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_ASSERT(!opt_unused_frames(func));
    ir_func_delete(func);
    TEST_OK();
}

static int retain_struct_references(void) {
    ir_func_t  *func           = ir_func_create("retain_struct_references", "entry", 1);
    ir_frame_t *input          = ir_frame_create(func, 8, 8, "struct_input");
    input->arg_index           = 0;
    func->args[0].arg_type     = IR_FUNCARG_TYPE_STRUCT;
    func->args[0].struct_frame = input;
    ir_frame_t *output         = ir_frame_create(func, 8, 8, "struct_output");
    ir_frame_create(func, 8, 8, "unused");
    ir_operand_t args[] = {IR_OPERAND_STRUCT(input)};
    ir_add_call(
        IR_APPEND(func->entry),
        IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM("transform_struct")),
        true,
        IR_RETVAL_STRUCT(output),
        1,
        args
    );
    ir_add_return0(IR_APPEND(func->entry));
    TEST_EXPECT_UINT(opt_test_frame_refs(func, input), 1u);
    TEST_EXPECT_UINT(opt_test_frame_refs(func, output), 1u);
    TEST_EXPECT_UINT(argument_references(func, output), 0u);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_ASSERT(opt_unused_frames(func));
    TEST_ASSERT(find_frame(func, "struct_input"));
    TEST_ASSERT(find_frame(func, "struct_output"));
    TEST_ASSERT(!find_frame(func, "unused"));
    TEST_EXPECT_UINT(func->frames_list.len, 2u);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    ir_func_delete(func);
    TEST_OK();
}

static int retain_argument_without_instructions(void) {
    ir_func_t  *func           = ir_func_create("retain_argument_without_instructions", "entry", 2);
    ir_frame_t *first          = ir_frame_create(func, 8, 8, "first_argument");
    ir_frame_t *second         = ir_frame_create(func, 16, 8, "second_argument");
    first->arg_index           = 0;
    second->arg_index          = 1;
    func->args[0].arg_type     = IR_FUNCARG_TYPE_STRUCT;
    func->args[0].struct_frame = first;
    func->args[1].arg_type     = IR_FUNCARG_TYPE_STRUCT;
    func->args[1].struct_frame = second;
    ir_frame_create(func, 4, 4, "unused_local");
    ir_add_return0(IR_APPEND(func->entry));
    TEST_EXPECT_UINT(opt_test_frame_refs(func, first), 0u);
    TEST_EXPECT_UINT(opt_test_frame_refs(func, second), 0u);
    TEST_EXPECT_UINT(argument_references(func, first), 1u);
    TEST_EXPECT_UINT(argument_references(func, second), 1u);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_ASSERT(opt_unused_frames(func));
    TEST_EXPECT_UINT(func->frames_list.len, 2u);
    TEST_ASSERT(find_frame(func, "first_argument") == func->args[0].struct_frame);
    TEST_ASSERT(find_frame(func, "second_argument") == func->args[1].struct_frame);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_ASSERT(!opt_unused_frames(func));
    ir_func_delete(func);
    TEST_OK();
}

static int cleanup_after_dead_code(void) {
    ir_func_t  *func  = ir_func_create("cleanup_after_dead_code", "entry", 0);
    ir_code_t  *dead  = ir_code_create(func, "dead");
    ir_frame_t *frame = ir_frame_create(func, 4, 4, "dead_only");
    // The entry returns immediately; the disconnected block has the only reference.
    ir_add_return0(IR_APPEND(func->entry));
    ir_add_store(IR_APPEND(dead), IR_OPERAND_CONST(IR_CONST_U32(23)), IR_MEMREF(IR_PRIM_i32, IR_BADDR_FRAME(frame)));
    ir_add_return0(IR_APPEND(dead));
    TEST_EXPECT_UINT(func->entry->insns.len, 1u);
    ir_insn_t *entry_ret = (ir_insn_t *)func->entry->insns.head;
    TEST_EXPECT_INT(entry_ret->type, IR_INSN_RETURN);
    TEST_EXPECT_UINT(opt_test_frame_refs(func, frame), 1u);
    TEST_EXPECT_UINT(argument_references(func, frame), 0u);
    ir_func_recalc_flow(func);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_ASSERT(!opt_unused_frames(func));
    TEST_ASSERT(opt_dead_code(func));
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    frame = find_frame(func, "dead_only");
    TEST_ASSERT(frame, "dead-code elimination should leave frame cleanup to its own pass");
    TEST_EXPECT_UINT(opt_test_frame_refs(func, frame), 0u);
    TEST_EXPECT_UINT(argument_references(func, frame), 0u);
    TEST_ASSERT(opt_unused_frames(func));
    TEST_ASSERT(!find_frame(func, "dead_only"));
    TEST_EXPECT_UINT(func->frames_list.len, 0u);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    ir_func_delete(func);
    TEST_OK();
}

static int retain_referenced_function_frames(void) {
    ir_func_t  *func   = ir_func_create("retain_referenced_function_frames", "entry", 0);
    ir_frame_t *call   = ir_frame_create(func, 16, 8, "call_frame");
    ir_frame_t *link   = ir_frame_create(func, 16, 8, "link_frame");
    call->flags       |= IR_FRAME_FLAG_CALL_FRAME;
    func->call_frame   = call;
    func->link_frame   = link;
    ir_var_t *address  = ir_var_create(func, IR_PRIM_i64, "link_address");
    ir_add_callframe_enter(IR_APPEND(func->entry), call);
    ir_add_lea_stack(IR_APPEND(func->entry), IR_RETVAL_VAR(address), link, 0);
    ir_add_callframe_exit(IR_APPEND(func->entry), call);
    func->rettype.type      = IR_FUNCRET_PRIM;
    func->rettype.prim_type = IR_PRIM_i64;
    ir_add_return1(IR_APPEND(func->entry), IR_OPERAND_VAR(address));
    TEST_EXPECT_UINT(opt_test_frame_refs(func, call), 2u);
    TEST_EXPECT_UINT(opt_test_frame_refs(func, link), 1u);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    TEST_ASSERT(!opt_unused_frames(func));
    TEST_ASSERT(find_frame(func, "call_frame") == func->call_frame);
    TEST_ASSERT(find_frame(func, "link_frame") == func->link_frame);
    TEST_EXPECT_INT(opt_test_validate(func), 0);
    ir_func_delete(func);
    TEST_OK();
}

int main(void) {
    int result  = 0;
    result     |= remove_empty_frames();
    result     |= retain_memory_references();
    result     |= retain_struct_references();
    result     |= retain_argument_without_instructions();
    result     |= cleanup_after_dead_code();
    result     |= retain_referenced_function_frames();
    return result;
}
