
#ifndef ASM_H
#define ASM_H

#include <config.h>
#include <stdbool.h>
#include <definitions.h>

#define ASM_CHUNK_DATA      0xCE
#define ASM_CHUNK_LABEL     0x9A
#define ASM_CHUNK_LABEL_REF 0x3B

struct asm_ctx;
struct asm_sect;
struct asm_label_def;

// The global assembly context.
typedef struct asm_ctx asm_ctx_t;
// One section of the output binary.
typedef struct asm_sect asm_sect_t;
// One label and information about it.
typedef struct asm_label_def asm_label_def_t;

typedef char *asm_label_t;

#include <strmap.h>
#include <gen.h>

struct asm_ctx {
    /* ======== Sections ========= */
    // A map of all the sections.
    map_t       *sections;
    // ID of the active section.
    char        *current_section_id;
    // The active sections.
    asm_sect_t  *current_section;
    
    /* ======== Registers ======== */
    // Whether the registers are used.
    bool       *regs_used;
    // What is stored in the registers.
    gen_var_t **regs_stored;
    
    /* ====== Miscellaneous ====== */
    // All the labels that are defined or referenced.
    map_t      *labels;
};

struct asm_sect {
    /* ========== Data =========== */
    // Data stored in this section.
    uint8_t    *chunks;
    // Length of the current chunk of data.
    size_t     *chunk_len;
    // Capacity for data.
    size_t      chunks_capacity;
    // Used data.
    size_t      chunks_len;
    
    /* ===== Post-processing ===== */
    // The offset of this section.
    address_t   offset;
};

struct asm_label_def {
    // Label value as is from source code.
    asm_label_t  source;
    // Label value as in text
    char        *value;
    // Whether the label is defined in this compilation.
    bool         is_defined;
    // At post-processing time: the label's address.
    address_t    address;
};

// Initialises the context.
void asm_init           (asm_ctx_t *ctx);

// Uses or creates the section.
void asm_use_sect       (asm_ctx_t *ctx, char *id);
// Writes memory words to the current chunk.
void asm_write_memwords (asm_ctx_t *ctx, memword_t *data, size_t len);
// Writes memory words to the current chunk.
void asm_write_memword  (asm_ctx_t *ctx, memword_t  data);
// Writes words to the current chunk.
void asm_write_word     (asm_ctx_t *ctx, word_t     data);
// Writes addresses to the current chunk.
void asm_write_address  (asm_ctx_t *ctx, address_t  data);
// Writes a number of arbitrary size to the current chunk.
void asm_write_num      (asm_ctx_t *ctx, size_t     data, size_t bytes);

// Writes label definitions to the current chunk.
void asm_write_label    (asm_ctx_t *ctx, char      *label);
// Writes label references to the current chunk.
void asm_write_label_ref(asm_ctx_t *ctx, char      *label, address_t offset, asm_label_ref_t mode);

#endif //ASM_H
