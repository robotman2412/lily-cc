
#ifndef ASM_H
#define ASM_H

#include <config.h>
#include <stdbool.h>
#include <definitions.h>

#define ASM_CHUNK_DATA      0xCE
#define ASM_CHUNK_LABEL     0x9A
#define ASM_CHUNK_LABEL_REF 0x3B
#define ASM_CHUNK_ZERO      0x82

#define ASM_NOT_ALIGNED     0

struct asm_scope;
struct asm_ctx;
struct asm_sect;
struct asm_label_def;

// A scope in the variable context.
typedef struct asm_scope asm_scope_t;
// The global assembly context.
typedef struct asm_ctx asm_ctx_t;
// One section of the output binary.
typedef struct asm_sect asm_sect_t;
// One label and information about it.
typedef struct asm_label_def asm_label_def_t;

typedef char *asm_label_t;

#include <strmap.h>
#include <gen.h>

struct asm_scope {
    // The parent scope.
    asm_scope_t *parent;
    // A map of the variables in the given scope.
    map_t        vars;
    // The total number of variables in the entire hierarchy.
    size_t       num;
    // The total number of variables excluding global variables.
    size_t       local_num;
    // A list of usage for registers.
    gen_var_t   *reg_usage[NUM_REGS];
    // Relative size of the stack.
    address_t   stack_size;
};

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
    bool        *regs_used;
    // What is stored in the registers.
    gen_var_t  **regs_stored;
    
    /* ====== Miscellaneous ====== */
    // All the labels that are defined or referenced.
    map_t       *labels;
    // The global scope.
    asm_scope_t  global_scope;
    // The current scope.
    asm_scope_t *current_scope;
    // The last global label emitted, if any.
    asm_label_t  last_global_label;
    // Extra bits of context on an architecture basis.
    ASM_CTX_EXTRAS
    
    /* ======== Function ========= */
    // The current function.
    funcdef_t  *current_func;
    // Whether or not the current function is an inlining.
    bool        is_inline;
    // The labels for temporary variables.
    char      **temp_labels;
    
    // The usage status of each temporary variable.
    bool       *temp_usage;
    // The number of temporary variables in existence.
    address_t   temp_num;
    // Number of last label in function.
    address_t   last_label_no;
    // Relative size of the stack.
    address_t   stack_size;
    
    /* ===== Post-processing ===== */
    // The current PC.
    address_t   pc;
    // The output file descriptor to be used.
    FILE       *out_fd;
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
    // Alignment of this section.
    address_t   align;
    
    /* ===== Post-processing ===== */
    // The offset of this section.
    address_t   offset;
};

struct asm_label_def {
    // Label value as is from source code.
    asm_label_t source;
    // Label value as in text
    char       *value;
    // Whether the label is defined in this compilation.
    bool        is_defined;
    // At post-processing time: the label's address.
    address_t   address;
};

// Initialises the context.
void asm_init           (asm_ctx_t *ctx);

// Uses or creates the section.
// Applies the alignment to the section.
void asm_use_sect       (asm_ctx_t *ctx, char      *id,   address_t  align);
// Updates the alignment of the given section, or all if null.
// Any alignment, even outside of powers of two accepted.
// Does not implicitly create sections.
void asm_set_align      (asm_ctx_t *ctx, char      *id,   address_t  align);
// Writes memory words to the current chunk.
void asm_write_memwords (asm_ctx_t *ctx, memword_t *data, size_t     len);
// Writes memory words to the current chunk.
void asm_write_memword  (asm_ctx_t *ctx, memword_t  data);
// Writes words to the current chunk.
void asm_write_word     (asm_ctx_t *ctx, word_t     data);
// Writes addresses to the current chunk.
void asm_write_address  (asm_ctx_t *ctx, address_t  data);
// Writes a number of arbitrary size to the current chunk.
void asm_write_num      (asm_ctx_t *ctx, size_t     data, size_t     bytes);

// Gets a new label based on asm_ctx::last_label_no.
char *asm_get_label     (asm_ctx_t *ctx);
// Writes label definitions to the current chunk.
void asm_write_label    (asm_ctx_t *ctx, char      *label);
// Writes label references to the current chunk.
void asm_write_label_ref(asm_ctx_t *ctx, char      *label, address_t offset, asm_label_ref_t mode);

// Writes zeroes.
void asm_write_zero     (asm_ctx_t *ctx, address_t  count);

// Reads a number of arbitrary size from the given buffer.
size_t asm_read_numb    (uint8_t   *buf, size_t     bytes);
// Writes a number of arbitrary size to the given buffer.
void asm_write_numb     (uint8_t   *buf, size_t     data,  size_t    bytes);

#endif //ASM_H
