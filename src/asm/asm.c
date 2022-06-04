
#include "asm.h"
#include "ctxalloc_warn.h"
#include <string.h>

static inline asm_sect_t *asm_create_sect (asm_ctx_t  *ctx,  char *id,   address_t align);
static inline void        asm_align_sect  (asm_sect_t *sect, address_t align);
static        void        asm_append_chunk(asm_ctx_t  *ctx,  char  type);
static inline void        asm_append_raw  (asm_ctx_t  *ctx,  char *data, size_t len);
static inline void        asm_append      (asm_ctx_t  *ctx,  char *data, size_t len);

// Initialises the context.
void asm_init(asm_ctx_t *ctx) {
	// Sections.
	ctx->allocator   = alloc_create(ALLOC_NO_PARENT);
	ctx->sections    = (map_t *)      xalloc(ctx->allocator, sizeof(map_t));
	map_create(ctx->sections);
	// Scopeth.
	map_create(&ctx->functions);
	ctx->global_scope.parent    = NULL;
	ctx->global_scope.num       = 0;
	ctx->global_scope.local_num = 0;
	map_create(&ctx->global_scope.vars);
	ctx->current_scope = &ctx->global_scope;
	for (reg_t i = 0; i < NUM_REGS; i++) {
		ctx->current_scope->reg_usage[i] = NULL;
	}
	// Labels.
	ctx->last_global_label = NULL;
	ctx->labels      = (map_t *)      xalloc(ctx->allocator, sizeof(map_t));
	map_create(ctx->labels);
	// Sections.
	// Compiled machine code
	asm_use_sect   (ctx, ".text",   ASM_NOT_ALIGNED);
	// Read-only initialised data
	asm_create_sect(ctx, ".rodata", ASM_NOT_ALIGNED);
	// Initialised data
	asm_create_sect(ctx, ".data",   ASM_NOT_ALIGNED);
	// Uninitialised data
	asm_create_sect(ctx, ".bss",    ASM_NOT_ALIGNED);
}

// Append more data is the RAW way.
static inline void asm_append_raw(asm_ctx_t *ctx, char *data, size_t len) {
	asm_sect_t *sect = ctx->current_section;
	if (sect->chunks_capacity < sect->chunks_len + len) {
		// Expand capacity.
		sect->chunk_len = (size_t *) ((size_t) sect->chunk_len - (size_t) sect->chunks);
		sect->chunks_capacity = sect->chunks_len + len + 128;
		sect->chunks    = xrealloc(ctx->allocator, sect->chunks, sect->chunks_capacity);
		sect->chunk_len = (size_t *) ((size_t) sect->chunk_len + (size_t) sect->chunks);
	}
	// Append data.
	if (data) {
		memcpy(sect->chunks + sect->chunks_len, data, len);
	} else {
		memset(sect->chunks + sect->chunks_len, 0, len);
	}
	// Set new length.
	sect->chunks_len += len;
}

// Append more data to the current chunk.
static inline void asm_append(asm_ctx_t *ctx, char *data, size_t len) {
	asm_append_raw(ctx, data, len);
	// Be careful here!
	*ctx->current_section->chunk_len += len;
}

// Append a chunk of a certain type.
static inline void asm_append_chunk(asm_ctx_t *ctx, char type) {
	if (*ctx->current_section->chunk_len) {
		// Add some stuff.
		asm_append_raw(ctx, &type, 1);
		asm_append_raw(ctx, NULL,  sizeof(size_t));
		// New length pointer.
		ctx->current_section->chunk_len = (size_t *) (ctx->current_section->chunks + ctx->current_section->chunks_len - sizeof(size_t));
	} else {
		// Change the chunk instead of adding another.
		char *ptr = (char *) ctx->current_section->chunk_len - 1;
		*ptr = type;
	}
	// Set the new length.
	*ctx->current_section->chunk_len = 0;
}

// Creates the section, optionally aligned.
// Any alignment, even outside of powers of two accepted.
static inline asm_sect_t *asm_create_sect(asm_ctx_t *ctx, char *id, address_t align) {
	asm_sect_t *sect      = (asm_sect_t *) xalloc(ctx->allocator, sizeof(asm_sect_t));
	sect->chunks          = (uint8_t *)    xalloc(ctx->allocator, 256);
	sect->chunks_capacity = 256;
	sect->chunks_len      = sizeof(size_t) + 1;
	sect->chunk_len       = (void *) sect->chunks + 1;
	sect->align           = align;
	*sect->chunks         = ASM_CHUNK_DATA;
	*sect->chunk_len      = 0;
	map_set(ctx->sections, id, sect);
	return sect;
}

// Aligns the section to be an integer multiple of align.
// Any alignment, even outside of powers of two accepted.
static inline void asm_align_sect(asm_sect_t *sect, address_t align) {
	if (align) {
		if (!sect->align) {
			// Set alignment.
			sect->align = align;
		} else if (sect->align % align != 0 && align % sect->align != 0) {
			// Update alignment to be compatible with former and new requirements.
			sect->align *= align;
		}
		// Otherwise, no action required.
	}
}

// Uses or creates the section.
// Applies the alignment to the section.
// Any alignment, even outside of powers of two accepted.
void asm_use_sect(asm_ctx_t *ctx, char *id, address_t align) {
	asm_sect_t *sect = map_get(ctx->sections, id);
	if (!sect) {
		// Create a new section with alignment.
		sect = asm_create_sect(ctx, id, align);
	} else {
		asm_align_sect(sect, align);
	}
	ctx->current_section = sect;
	ctx->current_section_id = id;
	DEBUG_GEN("  .section \"%s\"\n", id);
	DEBUG_ASM("s    .section \"%s\"\n", id);
}

// Updates the alignment of the given section, or all if null.
// Any alignment, even outside of powers of two accepted.
// Does not implicitly create sections.
void asm_set_align(asm_ctx_t *ctx, char *id, address_t align) {
	if (id) {
		// Align a specific section.
		asm_sect_t *sect = map_get(ctx->sections, id);
		if (sect) asm_align_sect(sect, align);
	} else {
		// Align all sections.
		for (size_t i = 0; i < map_size(ctx->sections); i++) {
			asm_align_sect(ctx->sections->values[i], align);
		}
	}
}

// Writes memory words to the current chunk.
void asm_write_memwords(asm_ctx_t *ctx, memword_t *data, size_t memwords) {
	for (size_t i = 0; i < memwords; i++) {
		asm_write_memword(ctx, data[i]);
	}
}

// Writes memory words to the current chunk.
void asm_write_memword(asm_ctx_t *ctx, memword_t data) {
	asm_write_num(ctx, data, sizeof(memword_t));
}

// Writes words to the current chunk.
void asm_write_word(asm_ctx_t *ctx, word_t data) {
	asm_write_num(ctx, data, WORDS_TO_MEMWORDS * sizeof(memword_t));
}

// Writes addresses to the current chunk.
void asm_write_address(asm_ctx_t *ctx, address_t data) {
	asm_write_num(ctx, data, ADDRESS_TO_MEMWORDS * sizeof(memword_t));
}

// Writes a number of arbitrary size to the current chunk.
void asm_write_num(asm_ctx_t *ctx, size_t data, size_t bytes) {
	if (bytes > 1) {
		unsigned char buf[bytes];
		// Encode endianness.
#if IS_LITTLE_ENDIAN
		// For little endian.
		for (size_t i = 0; i < bytes; i++) {
			buf[i] = data >> (i * 8);
		}
#else
		// For big endian.
		for (size_t i = 0; i < bytes; i++) {
			buf[bytes - i - 1] = data >> (i * 8);
		}
#endif
		// Write the result.
		asm_append(ctx, buf, bytes);
#ifdef DEBUG_ASSEMBLER
		DEBUG_ASM("+   ");
		for (size_t i = 0; i < bytes; i++) {
			printf(" %02x", buf[i]);
		}
		printf(" (%lx)\n", data);
#endif
	} else {
		// No endian-conversion for single bytes.
		asm_append(ctx, (char *) &data, 1);
		DEBUG_ASM("+    %02lx\n", data);
	}
}

// Gets a new label based on asm_ctx::last_label_no.
char *asm_get_label(asm_ctx_t *ctx) {
	if (ctx->current_func) {
		char *label = (char *) xalloc(ctx->allocator, strlen(ctx->current_func->ident.strval) + 3 + ADDR_BITS / 4);
		sprintf(label, "%s.L%x", ctx->current_func->ident.strval, ctx->last_label_no);
		ctx->last_label_no ++;
		return label;
	} else {
		char *label = (char *) xalloc(ctx->allocator, 2 + ADDR_BITS / 4);
		sprintf(label, "L%x", ctx->last_label_no);
		ctx->last_label_no ++;
		return label;
	}
}

static inline asm_label_def_t *get_or_create_label(asm_ctx_t *ctx, char *label) {
	asm_label_def_t *val = map_get(ctx->labels, label);
	if (!val) {
		val = xalloc(ctx->allocator, sizeof(asm_label_def_t));
		*val = (asm_label_def_t) {
			.address    = 0,
			.is_defined = false,
			.source     = xstrdup(ctx->allocator, label),
			.value      = xstrdup(ctx->allocator, label)
		};
		map_set(ctx->labels, label, val);
	}
	return val;
}

// Writes label definitions to the current chunk.
static void asm_write_label0(asm_ctx_t *ctx, char *label) {
	DEBUG_GEN("%s:\n", label);
	asm_label_def_t *def = get_or_create_label(ctx, label);
	def->is_defined = true;
	// New label chunk.
	asm_append_chunk(ctx, ASM_CHUNK_LABEL);
	// Label string.
	asm_append(ctx, label, strlen(label) + 1);
	// New data chunk.
	asm_append_chunk(ctx, ASM_CHUNK_DATA);
	DEBUG_ASM("d  %s:\n", label);
}

// Writes label definitions to the current chunk.
void asm_write_label(asm_ctx_t *ctx, char *label) {
	if (*label != '.') {
		// This is a global label.
		if (ctx->last_global_label) xfree(ctx->allocator, ctx->last_global_label);
		ctx->last_global_label = xstrdup(ctx->allocator, label);
		// Next!
		asm_write_label0(ctx, label);
	} else if (ctx->last_global_label) {
		// Do some strcat first.
		char *buf = xalloc(ctx->allocator, strlen(label) + strlen(ctx->last_global_label) + 1);
		*buf = 0;
		strcat(buf, ctx->last_global_label);
		strcat(buf, label);
		asm_write_label0(ctx, buf);
		xfree(ctx->allocator, buf);
	} else {
		// Warning.
		printf("Warning: No global label written before sublabel '%s'!\n", label);
		asm_write_label0(ctx, label);
	}
}

// Writes label references to the current chunk.
static void asm_write_label_ref0(asm_ctx_t *ctx, char *label, address_t offset, asm_label_ref_t mode) {
	get_or_create_label(ctx, label);
	// New label reference chunk.
	asm_append_chunk(ctx, ASM_CHUNK_LABEL_REF);
	// Label access mode.
	asm_append(ctx, (char *) &mode, 1);
	// Label offset.
	asm_write_address(ctx, offset);
	// Label string.
	asm_append(ctx, label, strlen(label) + 1);
	// New data chunk.
	asm_append_chunk(ctx, ASM_CHUNK_DATA);
#ifdef DEBUG_ASSEMBLER
#ifdef ENABLE_DEBUG_LOGS
	// Gets rid of a line added by asm_write_address earlier.
	printf("\x1b[A" "\x1b[2K");
#endif
	if (offset)
		DEBUG_ASM("r    %s+%d\n", label, offset);
	else
		DEBUG_ASM("r    %s\n", label);
#endif
}

// Writes label references to the current chunk.
void asm_write_label_ref(asm_ctx_t *ctx, char *label, address_t offset, asm_label_ref_t mode) {
	if (*label != '.') {
		// Global label.
		asm_write_label_ref0(ctx, label, offset, mode);
	} else if (ctx->last_global_label) {
		// Do some strcat first.
		char *buf = xalloc(ctx->allocator, strlen(label) + strlen(ctx->last_global_label) + 1);
		*buf = 0;
		strcat(buf, ctx->last_global_label);
		strcat(buf, label);
		asm_write_label_ref0(ctx, buf, offset, mode);
		xfree(ctx->allocator, buf);
	} else {
		// Warning.
		printf("Warning: No global label written before sublabel '%s'!\n", label);
		asm_write_label_ref0(ctx, label, offset, mode);
	}
}

// Writes zeroes.
void asm_write_zero(asm_ctx_t *ctx, address_t count) {
	DEBUG_GEN("  .zero 0x%x\n", count);
	asm_append_chunk(ctx, ASM_CHUNK_ZERO);
	asm_write_address(ctx, count);
	asm_append_chunk(ctx, ASM_CHUNK_DATA);
}

// Reads a number of arbitrary size from the given buffer.
size_t asm_read_numb(uint8_t *buf, size_t bytes) {
	if (bytes > 1) {
		size_t data = 0;
		// Decode endianness.
#if IS_LITTLE_ENDIAN
		// For little endian.
		for (size_t i = 0; i < bytes; i++) {
			data |= buf[i] >> (i * 8);
		}
#else
		// For big endian.
		for (size_t i = 0; i < bytes; i++) {
			data |= buf[bytes - i - 1] >> (i * 8);
		}
#endif
		// Return the result.
		return data;
	} else {
		// No endian-conversion for single bytes.
		return *buf;
	}
}

// Writes a number of arbitrary size to the given buffer.
void asm_write_numb(uint8_t *buf, size_t data, size_t bytes) {
	if (bytes > 1) {
		// Encode endianness.
#if IS_LITTLE_ENDIAN
		// For little endian.
		for (size_t i = 0; i < bytes; i++) {
			buf[i] = data >> (i * 8);
		}
#else
		// For big endian.
		for (size_t i = 0; i < bytes; i++) {
			buf[bytes - i - 1] = data >> (i * 8);
		}
#endif
	} else {
		// No endian-conversion for single bytes.
		*buf = (char) data;
	}
}
