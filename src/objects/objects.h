
#ifndef OBJECTS_H
#define OBJECTS_H

#include <asm.h>

// Function pointer for object file writers.
// typedef void(*object_writer)(asm_ctx_t *ctx);
// Function pointer for object file readers.
// typedef asm_ctx_t *(*object_reader)(FILE *fd);

// Writes an ELF (32-bit) file from an ASM context.
void output_elf32(asm_ctx_t *ctx, bool create_executable);

#endif //OBJECTS_H
