
#include "asm_postproc.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

typedef void(*asm_ppc_pass_t)(asm_ctx_t *ctx, uint8_t chunk_type, size_t chunk_len, uint8_t *chunk_data);

static void asm_ppc_pass1       (asm_ctx_t *ctx, uint8_t chunk_type, size_t chunk_len, uint8_t *chunk_data);
static void output_native_reduce(asm_ctx_t *ctx, uint8_t chunk_type, size_t chunk_len, uint8_t *chunk_data);

static inline void asm_ppc_iterate(asm_ctx_t *ctx, size_t n_sect, char **sect_ids, asm_sect_t **sects, asm_ppc_pass_t func, bool use_align) {
	// Iterate over the sections.
	for (size_t i = 0; i < n_sect; i++) {
		asm_sect_t *sect = sects[i];
		
		if (use_align) {
			ctx->pc = sects[i]->offset;
		} else {
			// Fix alignment.
			address_t offs = ctx->pc;
			if (sects[i]->align > 1) {
				address_t error = offs % sects[i]->align;
				if (error) {
					offs += sects[i]->align - error;
				}
				ctx->pc = offs;
				sects[i]->offset = offs;
			}
		
			if (sects[i]->align) {
				printf("%-9s (aligned %5d): 0x%04x\n", sect_ids[i], sects[i]->align, offs);
			} else {
				printf("%-9s (unligned     ): 0x%04x\n", sect_ids[i], offs);
			}
		}
		
		// Iterate over the chunks.
		size_t index = 0;
		while (index < sect->chunks_len) {
			size_t len = *(size_t *) (sect->chunks + index + 1);
			(*func)(ctx, sect->chunks[index], len, sect->chunks + index + sizeof(size_t) + 1);
			index += len + sizeof(size_t) + 1;
		}
	}
}

// Performs post-processing on the given context.
void asm_ppc(asm_ctx_t *ctx) {
	ctx->pc = 0;
	// Find the desired section order.
	size_t n_sect      = ctx->sections->numEntries;
	char **sect_ids    = ctx->sections->strings;
	asm_sect_t **sects = (asm_sect_t **) ctx->sections->values;
	// Alignment.
	asm_set_align(ctx, NULL, 256);
	// Pass 1: label resolution.
	asm_ppc_iterate(ctx, n_sect, sect_ids, sects, &asm_ppc_pass1, false);
}

// Pass 1: label resolution.
static void asm_ppc_pass1(asm_ctx_t *ctx, uint8_t chunk_type, size_t chunk_len, uint8_t *chunk_data) {
	// printf("%02x\n", chunk_type);
	if (chunk_type == ASM_CHUNK_ZERO) {
		ctx->pc += asm_read_numb(chunk_data, sizeof(address_t));
	} else if (chunk_type == ASM_CHUNK_DATA) {
		ctx->pc += chunk_len;
	} else if (chunk_type == ASM_CHUNK_LABEL_REF) {
		ctx->pc += 2;
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
			value -= sizeof(address_t) + ctx->pc;
		case (ASM_LABEL_REF_ABS_PTR):
			*len = sizeof(address_t);
			break;
		case (ASM_LABEL_REF_OFFS_WORD):
			value -= sizeof(memword_t) + ctx->pc;
			*len = sizeof(memword_t);
			break;
	}
	
	asm_write_numb(buf, value, *len);
	
	return true;
}

void output_native(asm_ctx_t *ctx) {
	ctx->pc = 0;
	// Find the desired section order.
	size_t n_sect      = ctx->sections->numEntries;
	char **sect_ids    = ctx->sections->strings;
	asm_sect_t **sects = (asm_sect_t **) ctx->sections->values;
	// Iterate to output.
	asm_ppc_iterate(ctx, n_sect, sect_ids, sects, &output_native_reduce, true);
}

static inline void output_native_padd(int fd, address_t n) {
	char *buf = malloc(256);
	memset(buf, 0, 256);
	while (n > 256) {
		// Write a bit at a time.
		write(fd, buf, 256);
		n -= 256;
	}
	if (n) {
		write(fd, buf, n);
	}
	free(buf);
}

// Reduce: write everything we know as a chunk of machine code.
static void output_native_reduce(asm_ctx_t *ctx, uint8_t chunk_type, size_t chunk_len, uint8_t *chunk_data) {
	long pos = lseek(ctx->out_fd, 0, SEEK_END);
	if (pos >= 0 && pos < ctx->pc) {
		output_native_padd(ctx->out_fd, ctx->pc - pos);
	}
	switch (chunk_type) {
		case ASM_CHUNK_DATA: {
			// Write the entire data immediately.
			write(ctx->out_fd, chunk_data, chunk_len);
			ctx->pc += chunk_len;
		} break;
		case ASM_CHUNK_ZERO: {
			// Write some zeroes.
			address_t n = asm_read_numb(chunk_data, sizeof(address_t));
			output_native_padd(ctx->out_fd, n);
			ctx->pc += n;
		} break;
		case ASM_CHUNK_LABEL_REF: {
			// Get my label.
			char buf[2];
			size_t len;
			asm_ppc_label(ctx, chunk_data, buf, &len);
			// Append it.
			write(ctx->out_fd, buf, len);
			ctx->pc += len;
		} break;
	}
}
