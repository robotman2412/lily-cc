
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
			DEBUG_ASM("Loading offset for %s as %04x\n", sect_ids[i], ctx->pc);
		} else {
			// Fix alignment.
			address_t offs = ctx->pc;
			if (sects[i]->align > 1) {
				address_t error = offs % sects[i]->align;
				if (error) {
					offs += sects[i]->align - error;
				}
				ctx->pc = offs;
				DEBUG_ASM("Realigning offset for %s to %04x\n", sect_ids[i], offs);
				sects[i]->offset = offs;
			} else {
				DEBUG_ASM("Setting offset for %s to %04x\n", sect_ids[i], ctx->pc);
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
			(*func)(ctx, sect, sect->chunks[index], len, sect->chunks + index + sizeof(size_t) + 1, func_args);
			index += len + sizeof(size_t) + 1;
		}
	}
}

// Pass 1: label resolution.
void asm_ppc_pass1(asm_ctx_t *ctx, asm_sect_t *sect, uint8_t chunk_type, size_t chunk_len, uint8_t *chunk_data, void *args) {
	bool dump_addr = (bool) args;
	if (chunk_type == ASM_CHUNK_ZERO) {
		// A chunk that indicates zeroes (usualy for .bss).
		size_t n = asm_read_numb(chunk_data, sizeof(address_t));
		ctx->pc += n;
		// Count towards size.
		sect->size += chunk_len / sizeof(memword_t);
	} else if (chunk_type == ASM_CHUNK_DATA) {
		// A chunk with raw output data.
		ctx->pc += chunk_len / sizeof(memword_t);
		// Count towards size.
		sect->size += chunk_len / sizeof(memword_t);
	} else if (chunk_type == ASM_CHUNK_LABEL_REF) {
		// A label reference.
		ctx->pc += ADDRESS_TO_MEMWORDS;
		// Count towards size.
		sect->size += ADDRESS_TO_MEMWORDS;
	} else if (chunk_type == ASM_CHUNK_LABEL) {
		// Look up the label.
		char *label = chunk_data;
		asm_label_def_t *def = map_get(ctx->labels, label);
		// And assign the current PC to it.
		def->address = ctx->pc;
		printf("%-20s @ %04x\n", label, def->address);
	} else if (chunk_type == ASM_CHUNK_EQU) {
		// Get address.
		address_t addr = asm_read_numb(chunk_data, sizeof(address_t));
		// Look up the label.
		char *label = chunk_data + sizeof(address_t);
		asm_label_def_t *def = map_get(ctx->labels, label);
		// And assign the equation result to it.
		def->address = addr;
		printf("%-20s = %04x\n", label, def->address);
	} else if (chunk_type == ASM_CHUNK_POS) {
		// A position chuck (usually for addr2line purposes).
		(*(address_t *) chunk_data) = ctx->pc;
	}
}

// Optional addr2line dump pass.
// Prints by address order.
// Argument is `int *` initialised to 1 -- last linenumber printed.
void asm_ppc_addrdump(asm_ctx_t *ctx, asm_sect_t *sect, uint8_t chunk_type, size_t chunk_len, uint8_t *chunk_data, void *args) {
	int *last_line = args;
	if (chunk_type == ASM_CHUNK_POS) {
		// A position chuck (usually for addr2line purposes).
		address_t addr = *(address_t *) chunk_data;
		pos_t pos = {
			.x0       = asm_read_numb(chunk_data + sizeof(address_t),                                    sizeof(int)),
			.y0       = asm_read_numb(chunk_data + sizeof(address_t) +   sizeof(int),                    sizeof(int)),
			.x1       = asm_read_numb(chunk_data + sizeof(address_t) + 2*sizeof(int),                    sizeof(int)),
			.y1       = asm_read_numb(chunk_data + sizeof(address_t) + 3*sizeof(int),                    sizeof(int)),
			.index0   = asm_read_numb(chunk_data + sizeof(address_t) + 4*sizeof(int),                    sizeof(size_t)),
			.index1   = asm_read_numb(chunk_data + sizeof(address_t) + 4*sizeof(int) +   sizeof(size_t), sizeof(size_t)),
			.filename =               chunk_data + sizeof(address_t) + 4*sizeof(int) + 2*sizeof(size_t),
		};
		
		// Spacer and hex dump formats.
		#if ADDR_BITS <= 4
		const char *spacer  = "    ";
		const char *linefmt = "    %01x | ";
		#elif ADDR_BITS <= 8
		const char *spacer  = "    ";
		const char *linefmt = "   %02x | ";
		#elif ADDR_BITS <= 16
		const char *spacer  = "    ";
		const char *linefmt = " %04x | ";
		#elif ADDR_BITS <= 32
		const char *spacer  = "        ";
		const char *linefmt = " %08x | ";
		#else // 64
		const char *spacer  = "                ";
		const char *linefmt = " %016x | ";
		#endif
		
		// Dump source before this chunk.
		for (int line = *last_line + 1; line < pos.y0; line ++) {
			printf(" %s | ", spacer);
			print_line(ctx->tokeniser_ctx, line);
		}
		
		if (*last_line < pos.y0) {
			// Dump initial line of this chunk.
			printf(linefmt, addr);
			print_line(ctx->tokeniser_ctx, pos.y0);
			*last_line = pos.y0;
		}
	}
}

// Prints the remainder of the lines for asm_ppc_addrdump.
void asm_fini_addrdump(asm_ctx_t *ctx, int last_line) {
	// Spacer and hex dump formats.
	#if ADDR_BITS <= 4
	const char *spacer  = "    ";
	const char *linefmt = "    %01x | ";
	#elif ADDR_BITS <= 8
	const char *spacer  = "    ";
	const char *linefmt = "   %02x | ";
	#elif ADDR_BITS <= 16
	const char *spacer  = "    ";
	const char *linefmt = " %04x | ";
	#elif ADDR_BITS <= 32
	const char *spacer  = "        ";
	const char *linefmt = " %08x | ";
	#else // 64
	const char *spacer  = "                ";
	const char *linefmt = " %016x | ";
	#endif
	
	// Dump source until end.
	int end = pos_empty(ctx->tokeniser_ctx).y1;
	for (int line = last_line + 1; line < end; line ++) {
		printf(" %s | ", spacer);
		print_line(ctx->tokeniser_ctx, line);
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

// Escapes spaces in the file path.
// Also replaces backslash with double backslash.
char *escapespaces(const char *in) {
	size_t in_len  = strlen(in);
	size_t out_len = in_len;
	
	// Count amount of spaces and backslashes in the device.
	for (size_t i = 0; in[i]; i++) {
		if (in[i] == ' ' || in[i] == '\\') {
			out_len ++;
		}
	}
	
	// If no special, return input.
	if (out_len == in_len) {
		return strdup(in);
	}
	
	// Build new string.
	char   *out = malloc(out_len + 1);
	size_t  out_ptr = 0;
	for (size_t i = 0; i < in_len; i++) {
		if (in[i] == ' ' || in[i] == '\\') {
			out[out_ptr++] = '\\';
		}
		out[out_ptr++] = in[i];
	}
	out[out_ptr] = 0;
	
	return out;
}

// Addr2line file dump pass.
void asm_ppc_addr2line(asm_ctx_t *ctx, asm_sect_t *sect, uint8_t chunk_type, size_t chunk_len, uint8_t *chunk_data, void *args) {
	if (chunk_type == ASM_CHUNK_POS) {
		// A position chuck (usually for addr2line purposes).
		address_t addr = *(address_t *) chunk_data;
		pos_t pos = {
			.x0       = asm_read_numb(chunk_data + sizeof(address_t),                                    sizeof(int)),
			.y0       = asm_read_numb(chunk_data + sizeof(address_t) +   sizeof(int),                    sizeof(int)),
			.x1       = asm_read_numb(chunk_data + sizeof(address_t) + 2*sizeof(int),                    sizeof(int)),
			.y1       = asm_read_numb(chunk_data + sizeof(address_t) + 3*sizeof(int),                    sizeof(int)),
			.index0   = asm_read_numb(chunk_data + sizeof(address_t) + 4*sizeof(int),                    sizeof(size_t)),
			.index1   = asm_read_numb(chunk_data + sizeof(address_t) + 4*sizeof(int) +   sizeof(size_t), sizeof(size_t)),
			.filename =               chunk_data + sizeof(address_t) + 4*sizeof(int) + 2*sizeof(size_t),
		};
		
		char *absfile = realpath(pos.filename, NULL);
		char *absesc  = escapespaces(absfile);
		char *rawesc  = escapespaces(pos.filename);
		
		// Printf this to the line dump file.
		// Format: "pos", absolute filename, relative filename, X0,Y0, X1,Y1
		fprintf(ctx->out_addr2line, "pos %s %s %x %d,%d %d,%d\n",
			absesc,
			rawesc,
			addr,
			pos.x0, pos.y0,
			pos.x1, pos.y1
		);
		
		free(absfile);
		free(absesc);
		free(rawesc);
		
	} else if (chunk_type == ASM_CHUNK_LABEL) {
		// Look up the label.
		char *label = chunk_data;
		asm_label_def_t *def = map_get(ctx->labels, label);
		if (def->is_defined) {
			char *nameesc = escapespaces(label);
			
			// Format: "label", label name, address
			fprintf(ctx->out_addr2line, "label %s %x\n",
				nameesc, def->address
			);
			
			free(nameesc);
		}
	}
}

// Addr2line section dump function.
// Adds sections to the dump file.
void asm_sects_addr2line(asm_ctx_t *ctx) {
	for (size_t i = 0; i < ctx->sections->numEntries; i++) {
		asm_sect_t *sect = (asm_sect_t *) ctx->sections->values[i];
		char *nameesc = escapespaces(ctx->sections->strings[i]);
		
		// Format: "sect", section name, address, size
		fprintf(ctx->out_addr2line, "sect %s %x %x\n",
			nameesc,
			sect->offset,
			sect->size
		);
		
		free(nameesc);
	}
}
