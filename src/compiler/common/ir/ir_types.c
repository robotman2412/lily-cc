
#include "ir_types.h"

uint8_t const ir_prim_sizes[] = {
    [IR_PRIM_s8]   = 1,
    [IR_PRIM_u8]   = 1,
    [IR_PRIM_s16]  = 2,
    [IR_PRIM_u16]  = 2,
    [IR_PRIM_s32]  = 4,
    [IR_PRIM_u32]  = 4,
    [IR_PRIM_s64]  = 8,
    [IR_PRIM_u64]  = 8,
    [IR_PRIM_s128] = 16,
    [IR_PRIM_u128] = 16,
    [IR_PRIM_bool] = 1,
    [IR_PRIM_f32]  = 4,
    [IR_PRIM_f64]  = 8,
};

char const *const ir_prim_names[] = {
#define IR_PRIM_DEF(prim) #prim,
#include "defs/ir_primitives.inc"
};

char const *const ir_op2_names[] = {
#define IR_OP2_DEF(x) #x,
#include "defs/ir_op2.inc"
};

char const *const ir_op1_names[] = {
#define IR_OP1_DEF(x) #x,
#include "defs/ir_op1.inc"
};

char const *const ir_flow_names[] = {
    [IR_FLOW_JUMP]        = "jump",
    [IR_FLOW_BRANCH]      = "branch",
    [IR_FLOW_CALL_DIRECT] = "call_direct",
    [IR_FLOW_CALL_PTR]    = "call_ptr",
    [IR_FLOW_RETURN]      = "return",
};
