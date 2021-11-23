
#include "asm.h"
#include <malloc.h>
#include <string.h>

static inline asm_sect_t *asm_create_sect (asm_ctx_t *ctx, char *id);
static        void        asm_append_chunk(asm_ctx_t *ctx, char  type);
static inline void        asm_append_raw  (asm_ctx_t *ctx, char *data, size_t len);
static inline void        asm_append      (asm_ctx_t *ctx, char *data, size_t len);

// Initialises the context.
void asm_init(asm_ctx_t *ctx) {
    // Sections.
    ctx->sections    = (map_t *)      malloc(sizeof(map_t));
    map_create(ctx->sections);
    // Registers.
    ctx->regs_used   = (bool *)       malloc(sizeof(bool)      * NUM_REGS);
    ctx->regs_stored = (gen_var_t **) malloc(sizeof(gen_var_t *) * NUM_REGS);
    for (int i = 0; i < NUM_REGS; i++) {
        ctx->regs_used[i]   = false;
        ctx->regs_stored[i] = NULL;
    }
    // Create and select the .text section. (compiled machine code)
    asm_use_sect   (ctx, ".text");
    // Create the .rodata section. (read-only initialised data)
    asm_create_sect(ctx, ".rodata");
    // Create the .data section. (initialised data)
    asm_create_sect(ctx, ".data");
    // Create the .bss section. (uninitialised data)
    asm_create_sect(ctx, ".bss");
}

// Append more data is the RAW way.
static inline void asm_append_raw(asm_ctx_t *ctx, char *data, size_t len) {
    asm_sect_t *sect = ctx->current_section;
    if (sect->chunks_capacity < sect->chunks_len + len) {
        // Expand capacity.
        sect->chunks_capacity = sect->chunks_len + len + 128;
        sect->chunks = realloc(sect->chunks, sect->chunks_capacity);
    }
    // Append data.
    if (data) {
        memcpy(sect->chunks + sect->chunks_len, data, len);
    } else {
        memset(sect->chunks + sect->chunks_len, 0, len);
    }
}

// Append more data to the current chunk.
static inline void asm_append(asm_ctx_t *ctx, char *data, size_t len) {
    asm_append_raw(ctx, data, len);
    ctx->current_section->chunk_len += len;
}

// Append a chunk of a certain type.
static inline void asm_append_chunk(asm_ctx_t *ctx, char type) {
    if (*ctx->current_section->chunk_len) {
        // Add some stuff.
        asm_append_raw(ctx, &type, 1);
        asm_append_raw(ctx, NULL,  sizeof(size_t));
        // New length pointer.
        ctx->current_section->chunk_len = (size_t *) (ctx->current_section->chunks_len - sizeof(size_t));
    } else {
        // Change the chunk instead of adding another.
        char *ptr = (char *) (ctx->current_section->chunk_len - 1);
        *ptr = type;
    }
    // Set the new length.
    *ctx->current_section->chunk_len = 0;
}

// Creates the section.
static inline asm_sect_t *asm_create_sect(asm_ctx_t *ctx, char *id) {
    asm_sect_t *sect = (asm_sect_t *) malloc(sizeof(asm_sect_t));
    sect->chunks          = (uint8_t *) malloc(256);
    sect->chunks_capacity = 256;
    sect->chunks_len      = sizeof(size_t) + 1;
    sect->chunk_len       = (void *) sect->chunks + 1;
    *sect->chunks         = ASM_CHUNK_DATA;
    *sect->chunk_len      = 0;
    map_set(ctx->sections, id, sect);
    return sect;
}

// Uses or creates the section.
void asm_use_sect(asm_ctx_t *ctx, char *id) {
    asm_sect_t *sect = map_get(ctx->sections, id);
    if (!sect) {
        sect = asm_create_sect(ctx, id);
    }
    ctx->current_section = sect;
    ctx->current_section_id = id;
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
        char buf[bytes];
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
    } else {
        // No endian-conversion for single bytes.
        asm_append(ctx, (char *) &data, 1);
    }
}

// Writes label definitions to the current chunk.
void asm_write_label(asm_ctx_t *ctx, char *label) {
    // New label chunk.
    asm_append_chunk(ctx, ASM_CHUNK_LABEL);
    // Label string.
    asm_append(ctx, label, strlen(label) + 1);
    // New data chunk.
    asm_append_chunk(ctx, ASM_CHUNK_DATA);
}

// Writes label references to the current chunk.
void asm_write_label_ref(asm_ctx_t *ctx, char *label, address_t offset, asm_label_ref_t mode) {
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
}
