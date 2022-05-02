
#ifndef ASM_POSTPROC_H
#define ASM_POSTPROC_H

#include "asm.h"

typedef void(*asm_ppc_pass_t)(asm_ctx_t *ctx, uint8_t chunk_type, size_t chunk_len, uint8_t *chunk_data);

// Performs post-processing on the given context.
void asm_ppc(asm_ctx_t *ctx);

// Post-processes the label reference for outputting.
bool asm_ppc_label(asm_ctx_t *ctx, uint8_t *chunk, uint8_t *buf, size_t *len);

// Outputs in the target architecture's native format.
void output_native(asm_ctx_t *ctx);

#endif //ASM_POSTPROC_H
