
#ifndef GR8CPU_R3_CONFIG_H
#define GR8CPU_R3_CONFIG_H

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

// Generator fallbacks used.
#define FALLBACK_gen_expression
#define FALLBACK_gen_expr_inline

#endif //GR8CPU_R3_CONFIG_H
