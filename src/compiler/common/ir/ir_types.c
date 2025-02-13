
#include "ir_types.h"

uint8_t const ir_prim_sizes[] = {
    [IR_PRIM_S8]   = 1,
    [IR_PRIM_U8]   = 1,
    [IR_PRIM_S16]  = 2,
    [IR_PRIM_U16]  = 2,
    [IR_PRIM_S32]  = 4,
    [IR_PRIM_U32]  = 4,
    [IR_PRIM_S64]  = 8,
    [IR_PRIM_U64]  = 8,
    [IR_PRIM_S128] = 16,
    [IR_PRIM_U128] = 16,
    [IR_PRIM_BOOL] = 1,
    [IR_PRIM_F32]  = 4,
    [IR_PRIM_F64]  = 8,
};

char const *const ir_prim_names[] = {
    [IR_PRIM_BOOL] = "bool",
    [IR_PRIM_S8]   = "s8",
    [IR_PRIM_U8]   = "u8",
    [IR_PRIM_S16]  = "s16",
    [IR_PRIM_U16]  = "u16",
    [IR_PRIM_S32]  = "s32",
    [IR_PRIM_U32]  = "u32",
    [IR_PRIM_S64]  = "s64",
    [IR_PRIM_U64]  = "u64",
    [IR_PRIM_S128] = "s128",
    [IR_PRIM_U128] = "u128",
    [IR_PRIM_F32]  = "f32",
    [IR_PRIM_F64]  = "f64",
};

char const *const ir_op2_names[] = {
    [IR_OP2_SGT]  = "sgt",
    [IR_OP2_SLE]  = "sle",
    [IR_OP2_SLT]  = "slt",
    [IR_OP2_SGE]  = "sge",
    [IR_OP2_SEQ]  = "seq",
    [IR_OP2_SNE]  = "sne",
    [IR_OP2_ADD]  = "add",
    [IR_OP2_SUB]  = "sub",
    [IR_OP2_MUL]  = "mul",
    [IR_OP2_DIV]  = "div",
    [IR_OP2_MOD]  = "mod",
    [IR_OP2_SHL]  = "shl",
    [IR_OP2_SHR]  = "shr",
    [IR_OP2_BAND] = "band",
    [IR_OP2_BOR]  = "bor",
    [IR_OP2_BXOR] = "bxor",
};

char const *const ir_op1_names[] = {
    [IR_OP1_MOV]  = "mov",
    [IR_OP1_SEQZ] = "seqz",
    [IR_OP1_SNEZ] = "snez",
    [IR_OP1_NEG]  = "neg",
    [IR_OP1_BNEG] = "bneg",
};

char const *const ir_flow_names[] = {
    [IR_FLOW_JUMP]        = "jump",
    [IR_FLOW_BRANCH]      = "branch",
    [IR_FLOW_CALL_DIRECT] = "call_direct",
    [IR_FLOW_CALL_PTR]    = "call_ptr",
    [IR_FLOW_RETURN]      = "return",
};
