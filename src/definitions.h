
#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include "config.h"
#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"
#include "version_number.h"

/* ==== Debug information ==== */

#ifdef ENABLE_DEBUG_LOGS
#   define DEBUG(...) printf("[DEBUG] " __VA_ARGS__)
#else
#   define DEBUG(...)
#endif

#ifdef DEBUG_TOKENISER
#   define DEBUG_TKN(...) DEBUG("[TKN] " __VA_ARGS__)
#else
#   define DEBUG_TKN(...)
#endif

#ifdef DEBUG_PARSER
#   define DEBUG_PAR(...) DEBUG("[PAR] " __VA_ARGS__)
#else
#   define DEBUG_PAR(...)
#endif

#ifdef DEBUG_ASSEMBLER
#   define DEBUG_ASM(...) DEBUG("[ASM] " __VA_ARGS__)
#else
#   define DEBUG_ASM(...)
#endif

#ifdef DEBUG_GENERATOR
#   define DEBUG_GEN(...) DEBUG("[GEN] " __VA_ARGS__)
#else
#   define DEBUG_GEN(...)
#endif

#ifdef DEBUG_PREPROC
#   define DEBUG_PRE(...) DEBUG("[PRE] " __VA_ARGS__)
#else
#   define DEBUG_PRE(...)
#endif

#define IS_CHAR_SIGNED   defined(CHAR_IS_SIGNED)
#define IS_CHAR_UNSIGNED defined(CHAR_IS_UNSIGNED)

#if defined(CHAR_IS_SIGNED) && defined(CHAR_IS_UNSIGNED)
#error "Cannot be both CHAR_IS_SIGNED and CHAR_IS_UNSIGNED, change in arch_config.h"
#endif
#if !defined(CHAR_IS_SIGNED) && !defined(CHAR_IS_UNSIGNED)
#error "Please define either CHAR_IS_SIGNED or CHAR_IS_UNSIGNED in arch_config.h"
#endif

#define IS_LITTLE_ENDIAN defined(LITTLE_ENDIAN)
#define IS_BIG_ENDIAN    defined(BIG_ENDIAN)

#if defined(LITTLE_ENDIAN) && defined(BIG_ENDIAN)
#error "Cannot be both LITTLE_ENDIAN and BIG_ENDIAN, change in arch_config.h"
#endif
#if !defined(LITTLE_ENDIAN) && !defined(BIG_ENDIAN)
#error "Please define either LITTLE_ENDIAN or BIG_ENDIAN in arch_config.h"
#endif

#ifndef WORD_BITS
#error "Please define WORD_BITS in arch_config.h"
#endif
#ifndef MEM_BITS
#error "Please define MEM_BITS in arch_config.h"
#endif
#ifndef ADDR_BITS
#error "Please define ADDR_BITS in arch_config.h"
#endif

/* ==== Word sizes ==== */

#if WORD_BITS <= 8
typedef uint_least8_t word_t;
#elif WORD_BITS <= 16
typedef uint_least16_t word_t;
#elif WORD_BITS <= 32
typedef uint_least32_t word_t;
#elif WORD_BITS <= 64
typedef uint_least64_t word_t;
#endif

#if MEM_BITS <= 8
typedef uint_least8_t memword_t;
#elif MEM_BITS <= 16
typedef uint_least16_t memword_t;
#elif MEM_BITS <= 32
typedef uint_least32_t memword_t;
#elif MEM_BITS <= 64
typedef uint_least64_t memword_t;
#endif

#if ADDR_BITS <= 8
typedef uint_least8_t address_t;
#elif ADDR_BITS <= 16
typedef uint_least16_t address_t;
#elif ADDR_BITS <= 32
typedef uint_least32_t address_t;
#elif ADDR_BITS <= 64
typedef uint_least64_t address_t;
#endif

/* ==== Word ratios ==== */

#if WORD_BITS > MEM_BITS
#define WORDS_TO_MEMWORDS ((WORD_BITS + MEM_BITS - 1) / MEM_BITS)
#define MEMWORDS_TO_WORDS 1
#elif WORD_BITS < MEM_BITS
#define WORDS_TO_MEMWORDS 1
#define MEMWORDS_TO_WORDS ((MEM_BITS + WORD_BITS - 1) / WORD_BITS)
#else
#define WORDS_TO_MEMWORDS 1
#define MEMWORDS_TO_WORDS 1
#endif

#if WORD_BITS > ADDR_BITS
#define WORDS_TO_ADDRESS ((WORD_BITS + MEM_BITS - 1) / MEM_BITS)
#define ADDRESS_TO_WORDS 1
#elif WORD_BITS < ADDR_BITS
#define WORDS_TO_ADDRESS 1
#define ADDRESS_TO_WORDS ((ADDR_BITS + WORD_BITS - 1) / WORD_BITS)
#else
#define WORDS_TO_ADDRESS 1
#define ADDRESS_TO_WORDS 1
#endif

#if ADDR_BITS > MEM_BITS
#define ADDRESS_TO_MEMWORDS ((ADDR_BITS + MEM_BITS - 1) / MEM_BITS)
#define MEMWORDS_TO_ADDRESS 1
#elif ADDR_BITS < MEM_BITS
#define ADDRESS_TO_MEMWORDS 1
#define MEMWORDS_TO_ADDRESS ((MEM_BITS + ADDR_BITS - 1) / ADDR_BITS)
#else
#define ADDRESS_TO_MEMWORDS 1
#define MEMWORDS_TO_ADDRESS 1
#endif

// Methods by which a label can be referenced in assembly.
typedef enum asm_label_ref {
	// Offset relative to address immediately after resolved address.
	// Pointer size.
	ASM_LABEL_REF_OFFS_PTR,
	// Offset relative to address immediately after resolved address.
	// Word size.
	ASM_LABEL_REF_OFFS_WORD,
	// Absolute address of label.
	// Pointer size.
	ASM_LABEL_REF_ABS_PTR,
	// Absolute address of label.
	// Word size (low word if applicable).
	ASM_LABEL_REF_ABS_WORD,
	// Word size (high word if applicable).
	ASM_LABEL_REF_ABS_WORD_HIGH
} asm_label_ref_t;

// All operators defined by the C language.
typedef enum oper {
	// Unary operators.
	OP_ADROF,
	OP_DEREF,
	OP_0_MINUS,
	//Binary operators.
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_SHIFT_L,
	OP_SHIFT_R,
	// Logic operators.
	OP_LOGIC_NOT,
	OP_LOGIC_AND,
	OP_LOGIC_OR,
	//Bitwise operators.
	OP_BIT_NOT,
	OP_BIT_AND,
	OP_BIT_OR,
	OP_BIT_XOR,
	// Comparison operators.
	OP_GT,
	OP_GE,
	OP_LT,
	OP_LE,
	OP_EQ,
	OP_NE,
	// Assignment operators.
	OP_ASSIGN,
	// Miscellaneous operators.
	OP_INDEX
} oper_t;

// Whether an oper_t is an instance of pointer handling.
#define OP_IS_PTR(x)   (x == OP_ADROF     || x == OP_DEREF    || OP_INDEX)
// Whether an oper_t is an instance of bitwise math.
#define OP_IS_BIT(x)   (x >= OP_BIT_NOT   && x <= OP_BIT_XOR)
// Whether an oper_t is an instance of additive math.
#define OP_IS_ADD(x)   (x == OP_ADD       || x == OP_SUB)
// Whether an oper_t is an instance of bitwise shifting.
#define OP_IS_SHIFT(x) (x == OP_SHIFT_L   || x == OP_SHIFT_R)
// Whether an oper_t is an instance of boolean math.
#define OP_IS_LOGIC(x) (x >= OP_LOGIC_NOT && x <= OP_LOGIC_OR)

// Types of statement.
typedef enum stmt_type {
	// Statements in { curly brackets }
	STMT_TYPE_MULTI,
	// if ; else { statements }
	STMT_TYPE_IF,
	// while (loops);
	STMT_TYPE_WHILE,
	// return statements;
	STMT_TYPE_RET,
	// variable declaration = statements;
	STMT_TYPE_VAR,
	// expression * statements
	STMT_TYPE_EXPR,
	// asm ("inline assembly" : "=r" (statements));
	STMT_TYPE_IASM
} stmt_type_t;

// Types of expression.
typedef enum expr_type {
	// Constant value (e.g. number constant, string constant or predefined pointer).
	EXPR_TYPE_CONST,
	// Identity (e.g. variable references).
	EXPR_TYPE_IDENT,
	// Method calls.
	EXPR_TYPE_CALL,
	// Unary math (e.g. !a, -b or *c).
	EXPR_TYPE_MATH1,
	// Binary meth (e.g. a+b, c*d or e^f).
	EXPR_TYPE_MATH2
} expr_type_t;

// Locations in which a variable can be stored at runtime.
typedef enum gen_var_type {
	// For void functions.
	VAR_TYPE_VOID,
	// Constant value.
	VAR_TYPE_CONST,
	
	// Located at label.
	VAR_TYPE_LABEL,
	// Located in stack frame.
	VAR_TYPE_STACKFRAME,
	// Located in stack by offset.
	VAR_TYPE_STACKOFFS,
	
	// Stored in register.
	VAR_TYPE_REG,
	// Stored as return value.
	VAR_TYPE_RETVAL,
	
	// Conditions.
	VAR_TYPE_COND,
	// Hint to use for adrof and storing to pointers.
	VAR_TYPE_PTR
} gen_var_type_t;

// Things like numeric types and alike.
// This is also a shorthand for the value of pointers end enumerations.
typedef enum simple_type {
	/* ==== Integer types ==== */
	
	// unsigned char
	CTYPE_U_CHAR,
	// signed char
	CTYPE_S_CHAR,
	// unsigned short int
	CTYPE_U_SHORT,
	// signed short int
	CTYPE_S_SHORT,
	// unsigned int
	CTYPE_U_INT,
	// signed int
	CTYPE_S_INT,
	// unsigned long int
	CTYPE_U_LONG,
	// signed long int
	CTYPE_S_LONG,
	// unsigned long long int
	CTYPE_U_LONGER,
	// signed long long int
	CTYPE_S_LONGER,
	
	/* ===== Float types ===== */
	
	// float
	CTYPE_FLOAT,
	// double
	CTYPE_DOUBLE,
	// long double
	CTYPE_LONG_DOUBLE,
	
	/* ===== Other types ===== */
	
	// _Bool (bool)
	CTYPE_BOOL,
	// void
	CTYPE_VOID
	
} simple_type_t;

#ifdef CHAR_IS_SIGNED
// 'char' type
// The character's default signedness depends on the machine.
#define CTYPE_CHAR CTYPE_S_CHAR
#else
// 'char' type
// The character's default signedness depends on the machine.
#define CTYPE_CHAR CTYPE_U_CHAR
#endif

// Size of 'char' types, in memory words.
#define CSIZE_CHAR   ((CHAR_BITS   - 1) / MEM_BITS + 1)
// Size of 'short' types, in memory words.
#define CSIZE_SHORT  ((SHORT_BITS  - 1) / MEM_BITS + 1)
// Size of 'int' types, in memory words.
#define CSIZE_INT    ((INT_BITS    - 1) / MEM_BITS + 1)
// Size of 'long' types, in memory words.
#define CSIZE_LONG   ((LONG_BITS   - 1) / MEM_BITS + 1)
// Size of 'long long' types, in memory words.
#define CSIZE_LONGER ((LONGER_BITS - 1) / MEM_BITS + 1)

// Size of 'float' types, in memory words.
#define CSIZE_FLOAT       ((FLOAT_BITS   - 1) / MEM_BITS + 1)
// Size of 'double' types, in memory words.
#define CSIZE_DOUBLE      ((DOUBLE_BITS   - 1) / MEM_BITS + 1)
// Size of 'long double' types, in memory words.
#define CSIZE_LONG_DOUBLE ((LONG_DOUBLE_BITS   - 1) / MEM_BITS + 1)

// Size of 'bool' types, in memory words.
#define CSIZE_BOOL 1

// Sizes of the simple types, in memory words, by index.
extern size_t simple_type_size[];
#define CSIZE_SIMPLE(index) (simple_type_size[index])
#define arrCSIZE_BY_INDEX {\
	CSIZE_CHAR,   CSIZE_CHAR,\
	CSIZE_SHORT,  CSIZE_SHORT,\
	CSIZE_INT,    CSIZE_INT,\
	CSIZE_LONG,    CSIZE_LONG,\
	CSIZE_LONGER, CSIZE_LONGER,\
	CSIZE_FLOAT,\
	CSIZE_DOUBLE,\
	CSIZE_LONG_DOUBLE,\
	CSIZE_BOOL,\
	0,\
}

// Categories of types.
typedef enum type_cat {
	// Simple types.
	TYPE_CAT_SIMPLE,
	// Pointer types.
	TYPE_CAT_POINTER,
	// Array types.
	TYPE_CAT_ARRAY,
	// Struct types
	TYPE_CAT_STRUCT,
	// Union types
	TYPE_CAT_UNION,
} type_cat_t;

// If no extra data groups have been defined, define them as empty.

// Extras added to 'struct preprocessor_data'.
#ifndef PREPROC_EXTRAS
#define PREPROC_EXTRAS
#endif

// Extras added to 'struct asm_ctx'.
#ifndef ASM_CTX_EXTRAS
#define ASM_CTX_EXTRAS
#endif

// Extras added to 'struct funcdef'.
#ifndef FUNCDEF_EXTRAS
#define FUNCDEF_EXTRAS
#endif

#endif //DEFINITIONS_H
