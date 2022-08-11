
#include "asm_postproc.h"
#include "pixie-16_options.h"

static inline void output_native_padd(FILE *fd, address_t n) {
	char *buf = xalloc(global_alloc, 256);
	memset(buf, 0, 256);
	while (n > 256) {
		// Write a bit at a time.
		fwrite(buf, 1, 256, fd);
		n -= 256;
	}
	if (n) {
		fwrite(buf, 1, n, fd);
	}
	xfree(global_alloc, buf);
}

// Reduce: write everything we know as a chunk of machine code.
static void output_native_reduce(asm_ctx_t *ctx, uint8_t chunk_type, size_t chunk_len, uint8_t *chunk_data, void *args) {
	fseek(ctx->out_fd, 0, SEEK_END);
	long pos = ftell(ctx->out_fd);
	if (pos >= 0 && pos < ctx->pc) {
		output_native_padd(ctx->out_fd, ctx->pc - pos);
	}
	switch (chunk_type) {
		case ASM_CHUNK_DATA: {
			// Write the entire data immediately.
			fwrite(chunk_data, 1, chunk_len, ctx->out_fd);
			ctx->pc += chunk_len / sizeof(memword_t);
		} break;
		case ASM_CHUNK_ZERO: {
			// Write some zeroes.
			address_t n = asm_read_numb(chunk_data, sizeof(address_t));
			output_native_padd(ctx->out_fd, n * sizeof(memword_t));
			ctx->pc += n;
		} break;
		case ASM_CHUNK_LABEL_REF: {
			// Get my label.
			char buf[sizeof(memword_t) * ADDRESS_TO_MEMWORDS];
			size_t len;
			asm_ppc_label(ctx, chunk_data, buf, &len);
			// Append it.
			fwrite(buf, 1, len, ctx->out_fd);
			ctx->pc += ADDRESS_TO_MEMWORDS;
		} break;
	}
}

void output_native(asm_ctx_t *ctx) {
    
    if (entrypoint) {
        // Insert entrypoints section.
        asm_use_sect(ctx, ".entrypoints", ASM_NOT_ALIGNED);
        
        // IRQ vector.
        asm_write_label(ctx, "__px16_vectors.irq");
        if (irqvector) {
			DEBUG_GEN("  .db %s\n", irqvector);
            asm_write_label_ref(ctx, irqvector, 0, ASM_LABEL_REF_ABS_PTR);
		} else {
			DEBUG_GEN("  .db %s\n", entrypoint);
            asm_write_label_ref(ctx, entrypoint, 0, ASM_LABEL_REF_ABS_PTR);
		}
		
        // NMI vector.
        asm_write_label(ctx, "__px16_vectors.nmi");
        if (irqvector) {
			DEBUG_GEN("  .db %s\n", nmivector);
            asm_write_label_ref(ctx, nmivector, 0, ASM_LABEL_REF_ABS_PTR);
		} else {
			DEBUG_GEN("  .db %s\n", entrypoint);
            asm_write_label_ref(ctx, entrypoint, 0, ASM_LABEL_REF_ABS_PTR);
		}
        
        // Entry vector.
        asm_write_label(ctx, "__px16_vectors.entry");
		DEBUG_GEN("  .db %s\n", entrypoint);
        asm_write_label_ref(ctx, entrypoint, 0, ASM_LABEL_REF_ABS_PTR);
    }
	
	if (irqvector && !entrypoint) {
		printf("Warning: -mirqhandler without -mentrypoint: -mirqhandler ignored.\n");
	}
	if (nmivector && !entrypoint) {
		printf("Warning: -mnmihandler without -mentrypoint: -mnmihandler ignored.\n");
	}
	if (entrypoint && !irqvector) {
		printf("Warning: -mentrypoint without -mirqhandler: IRQs unhandled.\n");
	}
	if (entrypoint && !nmivector) {
		printf("Warning: -mentrypoint without -mnmihandler: NMIs unhandled.\n");
	}
    
	// Find the desired section order:
    //  .entrypoints, .text, .rodata, .data, .bss, (rest)
	size_t n_sect      = ctx->sections->numEntries;
	char **sect_ids    = xalloc(ctx->allocator, n_sect * sizeof(char *));
	asm_sect_t **sects = xalloc(ctx->allocator, n_sect * sizeof(asm_sect_t *));
    
    if (entrypoint) {
        // TODO: Smarter section ordering.
        sect_ids[0] = ".entrypoints";
        sect_ids[1] = ".text";
        sect_ids[2] = ".rodata";
        sect_ids[3] = ".data";
        sect_ids[4] = ".bss";
    } else {
        // TODO: Smarter section ordering.
        sect_ids[0] = ".text";
        sect_ids[1] = ".rodata";
        sect_ids[2] = ".data";
        sect_ids[3] = ".bss";
    }
    
    // Get sections for IDs.
    for (int i = 0; i < n_sect; i++) {
        sects[i] = map_get(ctx->sections, sect_ids[i]);
    }
	
	
	// Pass 1: label resolution.
	ctx->pc = 0;
	asm_ppc_iterate(ctx, n_sect, sect_ids, sects, &asm_ppc_pass1, NULL, false);
	// Pass 2: binary generation.
	ctx->pc = 0;
	asm_ppc_iterate(ctx, n_sect, sect_ids, sects, &output_native_reduce, NULL, true);
	// Pass 3: do an address dump for easy debugging.
	int theindex = 1;
	asm_ppc_iterate(ctx, n_sect, sect_ids, sects, &asm_ppc_addrdump, &theindex, true);
	asm_fini_addrdump(ctx, theindex);
    
	// Enforce vector addresses.
	if (entrypoint) {
		asm_label_def_t *def = map_get(ctx->labels, "__px16_vectors.irq");
		if (def->address != 0) printf("\033[93mWarning: address of __px16_vectors.irq is not 0, your program might not work\033[0m\n");
		def = map_get(ctx->labels, "__px16_vectors.nmi");
		if (def->address != 1) printf("\033[93mWarning: address of __px16_vectors.nmi is not 1, your program might not work.\033[0m\n");
		def = map_get(ctx->labels, "__px16_vectors.entry");
		if (def->address != 2) printf("\033[93mWarning: address of __px16_vectors.entry is not 2, your program might not work.\033[0m\n");
	}
	
    // Clean up.
    xfree(ctx->allocator, sect_ids);
    xfree(ctx->allocator, sects);
}
