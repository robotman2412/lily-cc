
#ifndef PIXIE_16_CONFIG_H
#define PIXIE_16_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

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

// Type used for opcodes.
typedef enum {
	PX_OP_ADD     = 000, PX_OP_SUB,     PX_OP_CMP,     PX_OP_AND,     PX_OP_OR,     PX_OP_XOR,
	PX_OP_ADDC    = 010, PX_OP_SUBC,    PX_OP_CMPC,    PX_OP_ANDC,    PX_OP_ORC,    PX_OP_XORC,
	PX_OP_INC     = 020, PX_OP_DEC,     PX_OP_CMP1,                                               PX_OP_SHL  = 026, PX_OP_SHR,
	PX_OP_INCC    = 020, PX_OP_DECC,    PX_OP_CMP1C,                                              PX_OP_SHLC = 026, PX_OP_SHRC,
	PX_OP_MOV_ULT = 040, PX_OP_MOV_UGT, PX_OP_MOV_SLT, PX_OP_MOV_SGT, PX_OP_MOV_EQ, PX_OP_MOV_CS, PX_OP_MOV,
	PX_OP_MOV_UGE = 050, PX_OP_MOV_ULE, PX_OP_MOV_SGE, PX_OP_MOV_SLE, PX_OP_MOV_NE, PX_OP_MOV_CC, PX_OP_MOV_JSR,    PX_OP_MOV_CX,
	PX_OP_LEA_ULT = 060, PX_OP_LEA_UGT, PX_OP_LEA_SLT, PX_OP_LEA_SGT, PX_OP_LEA_EQ, PX_OP_LEA_CS, PX_OP_LEA,
	PX_OP_LEA_UGE = 070, PX_OP_LEA_ULE, PX_OP_LEA_SGE, PX_OP_LEA_SLE, PX_OP_LEA_NE, PX_OP_LEA_CC, PX_OP_LEA_JSR,
} px_opcode_t;

// Type used for addressing mode.
typedef enum {
	PX_ADDR_R0,
	PX_ADDR_R1,
	PX_ADDR_R2,
	PX_ADDR_R3,
	PX_ADDR_ST,
	PX_ADDR_MEM,
	PX_ADDR_PC,
	PX_ADDR_IMM,
} px_addr_t;

// Type used for registers.
typedef enum {
	PX_REG_R0,
	PX_REG_R1,
	PX_REG_R2,
	PX_REG_R3,
	PX_REG_ST,
	PX_REG_PF,
	PX_REG_PC,
	PX_REG_IMM,
} reg_t;

// Number of general registers.
#define NUM_REGS  4
// Register names.
#define REG_NAMES { "R0", "R1", "R2", "R3", "ST", "PF", "PC", "imm" }
// Registers that may be used as general registers in order of usage.
#define REG_ORDER { 0, 1, 2, 3 }

// Type used for conditions.
typedef enum {
	// Unsigned less than.
	COND_ULT  = 000,
	// Unsigned greater than.
	COND_UGT  = 001,
	// Signed less than.
	COND_SLT  = 002,
	// Signed greater than.
	COND_SGT  = 003,
	// Equal.
	COND_EQ   = 004,
	// Unsigned carry set.
	COND_CS   = 005,
	// Always true.
	COND_TRUE = 006,
	// Unsigned greater than or equal.
	COND_UGE  = 010,
	// Unsigned less than or equal.
	COND_ULE  = 011,
	// Signed greater than or equal.
	COND_SGE  = 012,
	// Signed less than or equal.
	COND_SLE  = 013,
	// Not equal.
	COND_NE   = 014,
	// Unsigned carry not set.
	COND_CC   = 015,
	// Reserved for JSR instruction.
	COND_JSR  = 016,
	// Reserved for carry extend instruction.
	COND_CX   = 017,
} cond_t;

// Type of calling convention to use.
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

// Struct representation of an instruction.
typedef struct {
	bool        y;
	px_addr_t   x;
	reg_t       b;
	reg_t       a;
	px_opcode_t o;
} px_insn_t;

// State that inline assembly is supported
// #define INLINE_ASM_SUPPORTED

#endif //PIXIE_16_CONFIG_H
