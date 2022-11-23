
#ifndef ASM_POSTPROC_H
#define ASM_POSTPROC_H

#include "asm.h"

typedef void(*asm_ppc_pass_t)(asm_ctx_t *ctx, uint8_t chunk_type, size_t chunk_len, uint8_t *chunk_data, void *args);

// Iterates over sections and chunks in ctx and calls a function for each chunk.
void asm_ppc_iterate(asm_ctx_t *ctx, size_t n_sect, char **sect_ids, asm_sect_t **sects, asm_ppc_pass_t func, void *func_args, bool use_align);

// Pass 1: label resolution.
void asm_ppc_pass1(asm_ctx_t *ctx, uint8_t chunk_type, size_t chunk_len, uint8_t *chunk_data, void *args);

// Optional addr2line dump pass.
// Prints by address order.
// Argument is `int *` initialised to 1 -- last linenumber printed.
void asm_ppc_addrdump(asm_ctx_t *ctx, uint8_t chunk_type, size_t chunk_len, uint8_t *chunk_data, void *args);

// Prints the remainder of the lines for asm_ppc_addrdump.
void asm_fini_addrdump(asm_ctx_t *ctx, int last_line);

// Post-processes the label reference for outputting.
bool asm_ppc_label(asm_ctx_t *ctx, uint8_t *chunk, uint8_t *buf, size_t *len);

// Addr2line file dump pass.
void asm_ppc_addr2line(asm_ctx_t *ctx, uint8_t chunk_type, size_t chunk_len, uint8_t *chunk_data, void *args);

// Outputs in the target architecture's native format.
void output_native(asm_ctx_t *ctx);

#endif //ASM_POSTPROC_H
