
#ifndef TEMPLATE_CONFIG_H
#define TEMPLATE_CONFIG_H

#include <stdint.h>

#define ARCH_ID "template"

// Endianness.
#define TARGET_LITTLE_ENDIAN

// Word sizes.
#define WORD_BITS 16
#define MEM_BITS   8
#define ADDR_BITS 16

// Register names.
#define NUM_REGS 4
#define REG_NAMES { "R0", "R1", "R2", "R3" }
#define REG_R0 0
#define REG_R1 1
#define REG_R2 2
#define REG_R3 3
typedef uint8_t reg_t;

// Generator fallbacks used.
#define FALLBACK_gen_expression
#define FALLBACK_gen_expr_inline
#define FALLBACK_gen_function
#define FALLBACK_gen_stmt

#endif //TEMPLATE_CONFIG_H
