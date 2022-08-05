
#include "asm_postproc.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

void asm_ppc_iterate(asm_ctx_t *ctx, size_t n_sect, char **sect_ids, asm_sect_t **sects, asm_ppc_pass_t func, void *func_args, bool use_align) {
	// Iterate over the sections.
	for (size_t i = 0; i < n_sect; i++) {
		asm_sect_t *sect = sects[i];
		
		if (use_align) {
			ctx->pc = sects[i]->offset;
			DEBUG_GEN("Loading offset for %s as %04x\n", sect_ids[i], ctx->pc);
		} else {
			// Fix alignment.
			address_t offs = ctx->pc;
			if (sects[i]->align > 1) {
				address_t error = offs % sects[i]->align;
				if (error) {
					offs += sects[i]->align - error;
				}
				ctx->pc = offs;
				DEBUG_GEN("Realigning offset for %s to %04x\n", sect_ids[i], offs);
				sects[i]->offset = offs;
			} else {
				DEBUG_GEN("Setting offset for %s to %04x\n", sect_ids[i], ctx->pc);
				sects[i]->offset = ctx->pc;
			}
		
			if (sects[i]->align) {
				printf("%-9s (aligned %5d): 0x%04x\n", sect_ids[i], sects[i]->align, offs);
			} else {
				printf("%-9s (unaligned    ): 0x%04x\n", sect_ids[i], offs);
			}
		}
		
		// Iterate over the chunks.
		size_t index = 0;
		while (index < sect->chunks_len) {
			size_t len = *(size_t *) (sect->chunks + index + 1);
			(*func)(ctx, sect->chunks[index], len, sect->chunks + index + sizeof(size_t) + 1, func_args);
			index += len + sizeof(size_t) + 1;
		}
	}
}

// Pass 1: label resolution.
void asm_ppc_pass1(asm_ctx_t *ctx, uint8_t chunk_type, size_t chunk_len, uint8_t *chunk_data, void *args) {
	// printf("%02x\n", chunk_type);
	if (chunk_type == ASM_CHUNK_ZERO) {
		size_t n = asm_read_numb(chunk_data, sizeof(address_t));
		ctx->pc += n;
	} else if (chunk_type == ASM_CHUNK_DATA) {
		ctx->pc += chunk_len / sizeof(memword_t);
	} else if (chunk_type == ASM_CHUNK_LABEL_REF) {
		ctx->pc += ADDRESS_TO_MEMWORDS;
	} else if (chunk_type == ASM_CHUNK_LABEL) {
		char *label = chunk_data;
		asm_label_def_t *def = map_get(ctx->labels, label);
		def->address = ctx->pc;
		printf("%s = %04x\n", label, def->address);
	}
}

// Post-processes the label reference for outputting.
bool asm_ppc_label(asm_ctx_t *ctx, uint8_t *chunk, uint8_t *buf, size_t *len) {
	// Extrach chunk data.
	char *label = chunk + sizeof(address_t) + 1;
	asm_label_ref_t mode = *chunk;
	address_t offs = asm_read_numb(chunk + 1, sizeof(address_t));
	
	// Check whether we know it's value.
	asm_label_def_t *def = map_get(ctx->labels, label);
	if (!def) return false;
	address_t value = def->address + offs;
	
	switch (mode) {
		case (ASM_LABEL_REF_OFFS_PTR):
			value -= ctx->pc;
			*len = sizeof(address_t);
			break;
			
		case (ASM_LABEL_REF_ABS_PTR):
			*len = sizeof(address_t);
			break;
			
		case (ASM_LABEL_REF_OFFS_WORD):
			value -= ctx->pc;
			*len = sizeof(memword_t);
			break;
	}
	
	asm_write_numb(buf, value, *len);
	
	return true;
}
