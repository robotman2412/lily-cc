
#ifndef GR8CPU_R3_CONFIG_H
#define GR8CPU_R3_CONFIG_H

#include <stdint.h>

#define ARCH_ID "GR8CPU"

// Endianness.
#define LITTLE_ENDIAN

// Word sizes.
#define WORD_BITS  8
#define MEM_BITS   8
#define ADDR_BITS 16

// Register names.
#define NUM_REGS 3
#define REG_NAMES { "A", "X", "Y" }
#define REG_A 0
#define REG_X 1
#define REG_Y 2

typedef uint8_t reg_t;
typedef uint8_t cond_t;

// Generator fallbacks used.
#define FALLBACK_gen_expression
#define FALLBACK_gen_expr_inline
#define FALLBACK_gen_function
#define FALLBACK_gen_stmt

#endif //GR8CPU_R3_CONFIG_H
