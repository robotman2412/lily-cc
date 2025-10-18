
#include "ir_types.h"

#include "arith128.h"

#include <endian.h>
#include <string.h>

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
#define IR_OP2_DEF(x) [IR_OP2_##x] = #x,
#include "defs/ir_op2.inc"
};

char const *const ir_op1_names[] = {
#define IR_OP1_DEF(x) [IR_OP1_##x] = #x,
#include "defs/ir_op1.inc"
};



// Convert an IR constant into bytes.
// Assumes the caller allocates enough space.
void ir_const_to_blob(ir_const_t iconst, uint8_t *blob, bool big_endian) {
    size_t size = ir_prim_sizes[iconst.prim_type];
    if (big_endian != (BYTE_ORDER == BIG_ENDIAN)) {
        uint8_t tmp[16];
        memcpy(tmp, &iconst.const128, size);
        for (size_t i = 0; i < size; i++) {
            blob[size - i - 1] = tmp[i];
        }
    } else {
        memcpy(blob, &iconst.const128, size);
    }
}

// Convert an IR constant from bytes.
// Assumes the caller guarantees the correctness of `blob`.
ir_const_t ir_const_from_blob(ir_prim_t prim, uint8_t const *blob, bool big_endian) {
    ir_const_t iconst = {0};
    iconst.prim_type  = prim;
    size_t size       = ir_prim_sizes[iconst.prim_type];
    if (big_endian != (BYTE_ORDER == BIG_ENDIAN)) {
        uint8_t tmp[16];
        for (size_t i = 0; i < size; i++) {
            tmp[i] = blob[size - i - 1];
        }
        memcpy(&iconst.const128, tmp, size);
    } else {
        memcpy(&iconst.const128, blob, size);
    }
    return iconst;
}
