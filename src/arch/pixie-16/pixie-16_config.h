
#ifndef PIXIE_16_CONFIG_H
#define PIXIE_16_CONFIG_H

#include <stdint.h>

#define ARCH_ID "Pixie 16"

// Specifies to main.c that there is an additional argument parser function.
// This is always called machine_argparse, and is used for -m... options.
#define HAS_MACHINE_ARGPARSE

// Specifies that Position-Independant Executables are supported.
// For PX16, PIE executables are the default unless '-mentrypoint=...' is specified.
// This option implies the program is run under an OS.
#define HAS_PIE_EXEC
// Specifies that Position-Independant Code objects are supported.
// For PX16, all '-shared' binaries are PIC by default.
// This option implies the program is run under an OS.
#define HAS_PIE_OBJ
// PX16 also supports Global Offset Table usage, which is used to dynamically link against libraries at runtime.
// This option implies the program is run under an OS.
#define HAS_GOT

// Specifies to the ELF writer that the machine type is 0.
// Optional definition.
#define ELF_MACHINE 0x00

// Endianness.
// Define TARGET_BIG_ENDIAN instead if your machine is big endian.
#define TARGET_LITTLE_ENDIAN

// Word sizes.
#define WORD_BITS   16
#define MEM_BITS    16
#define ADDR_BITS   16

// Integer type sizes, in bits used in calculations.
#define CHAR_BITS   16
#define SHORT_BITS  16
#define INT_BITS    16
#define LONG_BITS   32
#define LONGER_BITS 64

// Floating-point type sizes, in bits used in calculations.
// If floats are not natively supported and there is no standard format, these values may be anything.
// For Pixie 16, there is currently no standard format.
#define FLOAT_BITS       32
#define DOUBLE_BITS      32
#define LONG_DOUBLE_BITS 64

// Character default signedness.
// This is a property of the machine and not the language.
// Only applies to 'char' without 'signed' nor 'unsigned'.
#define CHAR_IS_UNSIGNED

// Number of general registers.
#define NUM_REGS  4
// Register names.
#define REG_NAMES { "R0", "R1", "R2", "R3", "ST", "PF", "PC", "imm" }
#define REG_R0    0
#define REG_R1    1
#define REG_R2    2
#define REG_R3    3
#define REG_ST    4
#define REG_PF    5
#define REG_PC    6
#define REG_IMM   7

// Type used for registers.
typedef uint_least8_t reg_t;
// Type used for conditions.
typedef uint_least8_t cond_t;
// Type used for pointers.
typedef uint_least16_t ptr_t;

// Extra data added to funcdef_t.
typedef enum {
	// Functions with no parameters.
	PX_CC_NONE,
	// Functions with up to four words of parameters.
	// Stored in R0 through R3.
	PX_CC_REGS,
	// Functions with more than four words of parameters.
	// Stored in the stack, first parameter pushed last.
	PX_CC_STACK,
} px_call_conv_t;
#define FUNCDEF_EXTRAS px_call_conv_t call_conv;

// Extra data added to asm_scope_t.
#define ASM_SCOPE_EXTRAS address_t real_stack_size;

// State that inline assembly is supported
#define INLINE_ASM_SUPPORTED

// Generator fallbacks used.
#define FALLBACK_gen_expression
#define FALLBACK_gen_expr_inline
#define FALLBACK_gen_function
#define FALLBACK_gen_stmt

#endif //PIXIE_16_CONFIG_H
