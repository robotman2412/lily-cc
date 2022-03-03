
#ifndef GR8CPU_R3_CONFIG_H
#define GR8CPU_R3_CONFIG_H

#include <stdint.h>

#define ARCH_ID "GR8CPU"

// Endianness.
// Define BIG_ENDIAN instead if your machine is big endian.
#define LITTLE_ENDIAN

// Word sizes.
#define WORD_BITS    8
#define MEM_BITS     8
#define ADDR_BITS   16

// Integer type sizes, in bits used in calculations.
#define CHAR_BITS    8
#define SHORT_BITS  16
#define INT_BITS    16
#define LONG_BITS   32
#define LONGER_BITS 64

// Floating-point type sizes, in bits used in calculations.
// If floats are not natively supported and there is no standard format, these values may be anything.
// For GR8CPU Rev3.2, there is currently no standard format.
#define FLOAT_BITS       32
#define DOUBLE_BITS      32
#define LONG_DOUBLE_BITS 64

// Character default signedness.
// This is a property of the machine and not the language.
// Only applies to 'char' without 'signed' nor 'unsigned'.
#define CHAR_IS_UNSIGNED

// Register names.
#define NUM_REGS  3
#define REG_NAMES { "A", "X", "Y" }
#define REG_A     0
#define REG_X     1
#define REG_Y     2

// Type used for registers.
typedef uint8_t reg_t;
// Type used for conditions.
typedef uint8_t cond_t;
// Type used for pointers.
typedef uint16_t ptr_t;

// Extra data added to funcdef_t.
typedef enum {
	// Functions with one two-byte parameter.
	// Stored in the X and Y registers.
	R3_CC_INT,
	// Functions with 0 to 3 one-byte parameters.
	// Stored in the A, X and Y registers.
	R3_CC_CHAR,
	// Functions with any other pattern.
	// Stored in labeled memory addresses.
	R3_CC_MEM
} r3_call_conv_t;
#define FUNCDEF_EXTRAS r3_call_conv_t call_conv;

// State that inline assembly is supported
#define INLINE_ASM_SUPPORTED

// Generator fallbacks used.
#define FALLBACK_gen_expression
#define FALLBACK_gen_expr_inline
#define FALLBACK_gen_function
#define FALLBACK_gen_stmt

#endif //GR8CPU_R3_CONFIG_H
