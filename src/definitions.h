
#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include "config.h"
#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"
#include "signal.h"
#include "sys/ptrace.h"
#include "version_number.h"

/* ==== Debug information ==== */

#ifdef DEBUG_COMPILER
// Prompt debugger by raising SIGTRAP, if DEBUG_COMPILER is defined.
#	define DEBUGGER() raise(SIGTRAP)
#else
// Prompt debugger by raising SIGTRAP, if DEBUG_COMPILER is defined.
#	define DEBUGGER() do{}while(0)
#endif

#ifdef ENABLE_DEBUG_LOGS
// Print a debug log, if ENABLE_DEBUG_LOGS is defined.
#   define DEBUG(...) printf("[DEBUG] " __VA_ARGS__)
#else
// Print a debug log, if ENABLE_DEBUG_LOGS is defined.
#   define DEBUG(...) do{}while(0)
#endif

#ifdef DEBUG_TOKENISER
// Print a debug log, if DEBUG_TOKENISER and ENABLE_DEBUG_LOGS defined.
#   define DEBUG_TKN(...) DEBUG("[TKN] " __VA_ARGS__)
#else
// Print a debug log, if DEBUG_TOKENISER and ENABLE_DEBUG_LOGS defined.
#   define DEBUG_TKN(...) do{}while(0)
#endif

#ifdef DEBUG_PARSER
// Print a debug log, if DEBUG_PARSER and ENABLE_DEBUG_LOGS defined.
#   define DEBUG_PAR(...) DEBUG("[PAR] " __VA_ARGS__)
#else
// Print a debug log, if DEBUG_PARSER and ENABLE_DEBUG_LOGS defined.
#   define DEBUG_PAR(...) do{}while(0)
#endif

#ifdef DEBUG_ASSEMBLER
// Print a debug log, if DEBUG_ASSEMBLER and ENABLE_DEBUG_LOGS defined.
#   define DEBUG_ASM(...) DEBUG("[ASM] " __VA_ARGS__)
#else
// Print a debug log, if DEBUG_ASSEMBLER and ENABLE_DEBUG_LOGS defined.
#   define DEBUG_ASM(...) do{}while(0)
#endif

#ifdef DEBUG_GENERATOR
// Print a debug log, if DEBUG_GENERATOR and ENABLE_DEBUG_LOGS defined.
#   define DEBUG_GEN(...) DEBUG("[GEN] " __VA_ARGS__)
#else
// Print a debug log, if DEBUG_GENERATOR and ENABLE_DEBUG_LOGS defined.
#   define DEBUG_GEN(...) do{}while(0)
#endif

#ifdef DEBUG_PREPROC
// Print a debug log, if DEBUG_PREPROC and ENABLE_DEBUG_LOGS defined.
#   define DEBUG_PRE(...) DEBUG("[PRE] " __VA_ARGS__)
#else
// Print a debug log, if DEBUG_PREPROC and ENABLE_DEBUG_LOGS defined.
#   define DEBUG_PRE(...) do{}while(0)
#endif

/* ==== Checks on arch_config.h ==== */

#if defined(CHAR_IS_SIGNED) && defined(CHAR_IS_UNSIGNED)
#error "Cannot be both CHAR_IS_SIGNED and CHAR_IS_UNSIGNED, change in arch_config.h"
#endif
#if !defined(CHAR_IS_SIGNED) && !defined(CHAR_IS_UNSIGNED)
#error "Please define either CHAR_IS_SIGNED or CHAR_IS_UNSIGNED in arch_config.h"
#endif

#if defined(TARGET_LITTLE_ENDIAN) && defined(TARGET_BIG_ENDIAN)
#error "Cannot be both LITTLE_ENDIAN and BIG_ENDIAN, change in arch_config.h"
#endif
#if !defined(TARGET_LITTLE_ENDIAN) && !defined(TARGET_BIG_ENDIAN)
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

#ifdef CHAR_IS_SIGNED
// Whether the 'char' type is treated as signed.
#define IS_CHAR_SIGNED 1
// Whether the 'char' type is treated as unsigned.
#define IS_CHAR_UNSIGNED 0
#else
// Whether the 'char' type is treated as signed.
#define IS_CHAR_SIGNED 0
// Whether the 'char' type is treated as unsigned.
#define IS_CHAR_UNSIGNED 1
#endif

#ifdef TARGET_LITTLE_ENDIAN
// Whether the target stores numbers in little endian order.
#define IS_LITTLE_ENDIAN 1
// Whether the target stores numbers in big endian order.
#define IS_BIG_ENDIAN 0
#else
// Whether the target stores numbers in little endian order.
#define IS_LITTLE_ENDIAN 0
// Whether the target stores numbers in big endian order.
#define IS_BIG_ENDIAN 1
#endif

/* ==== Word sizes ==== */

#if WORD_BITS <= 8
// The native word size of the target.
typedef uint_least8_t word_t;
#elif WORD_BITS <= 16
// The native word size of the target.
typedef uint_least16_t word_t;
#elif WORD_BITS <= 32
// The native word size of the target.
typedef uint_least32_t word_t;
#elif WORD_BITS <= 64
// The native word size of the target.
typedef uint_least64_t word_t;
#endif

#if MEM_BITS <= 8
// The smallest memory word size of the target.
typedef uint_least8_t memword_t;
#elif MEM_BITS <= 16
// The smallest memory word size of the target.
typedef uint_least16_t memword_t;
#elif MEM_BITS <= 32
// The smallest memory word size of the target.
typedef uint_least32_t memword_t;
#elif MEM_BITS <= 64
// The smallest memory word size of the target.
typedef uint_least64_t memword_t;
#endif

#if ADDR_BITS <= 8
// The largest address size of the target.
typedef uint_least8_t  address_t;
// The largest address difference size of the target.
typedef int_least8_t   addrdiff_t;
#elif ADDR_BITS <= 16
// The largest address size of the target.
typedef uint_least16_t address_t;
// The largest address difference size of the target.
typedef int_least16_t  addrdiff_t;
#elif ADDR_BITS <= 32
// The largest address size of the target.
typedef uint_least32_t address_t;
// The largest address difference size of the target.
typedef int_least32_t  addrdiff_t;
#elif ADDR_BITS <= 64
// The largest address size of the target.
typedef uint_least64_t address_t;
// The largest address difference size of the target.
typedef int_least64_t  addrdiff_t;
#endif

/* ==== Word ratios ==== */

#if WORD_BITS > MEM_BITS
// How many word_t completely fit in one memword_t
#define WORDS_TO_MEMWORDS ((WORD_BITS + MEM_BITS - 1) / MEM_BITS)
// How many memword_t completely fit in one word_t.
#define MEMWORDS_TO_WORDS 1
#elif WORD_BITS < MEM_BITS
// How many word_t completely fit in one memword_t
#define WORDS_TO_MEMWORDS 1
// How many memword_t completely fit in one word_t.
#define MEMWORDS_TO_WORDS ((MEM_BITS + WORD_BITS - 1) / WORD_BITS)
#else
// How many word_t completely fit in one memword_t
#define WORDS_TO_MEMWORDS 1
// How many memword_t completely fit in one word_t.
#define MEMWORDS_TO_WORDS 1
#endif

#if WORD_BITS > ADDR_BITS
// How many word_t completely fit in one address_t.
#define WORDS_TO_ADDRESS ((WORD_BITS + MEM_BITS - 1) / MEM_BITS)
// How many address_t completely fit in one word_t.
#define ADDRESS_TO_WORDS 1
#elif WORD_BITS < ADDR_BITS
// How many word_t completely fit in one address_t.
#define WORDS_TO_ADDRESS 1
// How many address_t completely fit in one word_t.
#define ADDRESS_TO_WORDS ((ADDR_BITS + WORD_BITS - 1) / WORD_BITS)
#else
// How many word_t completely fit in one address_t.
#define WORDS_TO_ADDRESS 1
// How many address_t completely fit in one word_t.
#define ADDRESS_TO_WORDS 1
#endif

// How many address_t completely fit in one memword_t.
#if ADDR_BITS > MEM_BITS
#define ADDRESS_TO_MEMWORDS ((ADDR_BITS + MEM_BITS - 1) / MEM_BITS)
// How many memword_t completely fit in one address_t.
#define MEMWORDS_TO_ADDRESS 1
#elif ADDR_BITS < MEM_BITS
// How many address_t completely fit in one memword_t.
#define ADDRESS_TO_MEMWORDS 1
// How many memword_t completely fit in one address_t.
#define MEMWORDS_TO_ADDRESS ((MEM_BITS + ADDR_BITS - 1) / ADDR_BITS)
#else
// How many address_t completely fit in one memword_t.
#define ADDRESS_TO_MEMWORDS 1
// How many memword_t completely fit in one address_t.
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
	/* ==== Unary operators ==== */
	// Address of operator (prefix &).
	OP_ADROF,
	// Dereference operator (prefix *).
	OP_DEREF,
	// Zero minus operator (prefix -).
	OP_0_MINUS,
	
	/* ==== Post-use modification operators ==== */
	// Post-increment operator (postfix ++).
	OP_POST_INC,
	// Post-decrement operator (postfix --).
	OP_POST_DEC,
	
	/* ==== Binary operators ==== */
	// Addition operator (+).
	OP_ADD,
	// Subtraction operator (-).
	OP_SUB,
	// Multiplication operator (*).
	OP_MUL,
	// Division operator (/).
	OP_DIV,
	// Modulo operator (%).
	OP_MOD,
	
	/* ==== Logic operators ==== */
	// Logical NOT operator (!).
	OP_LOGIC_NOT,
	// Logical AND operator (&&).
	OP_LOGIC_AND,
	// Logical OR operator (||).
	OP_LOGIC_OR,
	
	/* ==== Bitwise operators ==== */
	// Bitwise NOT operator (~).
	OP_BIT_NOT,
	// Bitwise AND operator (&).
	OP_BIT_AND,
	// Bitwise OR operator (|).
	OP_BIT_OR,
	// Bitwise XOR operator (^).
	OP_BIT_XOR,
	// Bitwise left shift operator (<<).
	OP_SHIFT_L,
	// Bitwise right shift operator (>>).
	OP_SHIFT_R,
	
	/* ==== Comparison operators ==== */
	// Greater than operator (>).
	OP_GT,
	// Greater than or equal to operator (>=).
	OP_GE,
	// Less than operator (<).
	OP_LT,
	// Less than or equal to operator (<=).
	OP_LE,
	// Equality operator (==).
	OP_EQ,
	// Inequality operator (!=).
	OP_NE,
	
	// Assignment operator.
	OP_ASSIGN,
	
	// Array and/or pointer indexing operator.
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
// Whether an oper_t is an instance of comparison.
#define OP_IS_COMP(x)  (x >= OP_GT        && x <= OP_NE)

// Types of statement.
typedef enum stmt_type {
	// An empty statement ;
	STMT_TYPE_EMPTY,
	
	// Statements in { curly brackets }
	STMT_TYPE_MULTI,
	
	// if ; else ; statements
	STMT_TYPE_IF,
	// while (loops);
	STMT_TYPE_WHILE,
	// for (;;) loops;
	STMT_TYPE_FOR,
	
	// return statements;
	STMT_TYPE_RET,
	// variable declaration = statements;
	STMT_TYPE_VAR,
	// expression + statements
	STMT_TYPE_EXPR,
	
	// asm ("inline assembly" : "=r" (statements));
	STMT_TYPE_IASM
} stmt_type_t;

// Types of expression.
typedef enum expr_type {
	// Numeric constant value (e.g. number constantor predefined pointer).
	EXPR_TYPE_CONST,
	// C-string by label reference.
	EXPR_TYPE_CSTR,
	
	// Identity (e.g. variable references).
	EXPR_TYPE_IDENT,
	
	// Method calls.
	EXPR_TYPE_CALL,
	
	// Unary math (e.g. !a, -b or *c).
	EXPR_TYPE_MATH1,
	// Binary math (e.g. a+b, c*d or e^f).
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
	// Hint to store to return value.
	// There is no need to vacate any variables coincidentally stored there.
	VAR_TYPE_RETVAL,
	
	// Conditions.
	VAR_TYPE_COND,
	// Hint to use for adrof and storing to pointers.
	VAR_TYPE_PTR,
	// Hint to use for loading from and storing to array indexing.
	VAR_TYPE_INDEXED,
	
	// As of yet unassigned variables.
	// Only applies to variables which can under no circumstance be assigned at this point.
	VAR_TYPE_UNASSIGNED,
} gen_var_type_t;

// Things like numeric types and alike.
// This is also a shorthand for the value of pointers end enumerations.
typedef enum simple_type {
	/* ==== Integer types ==== */
	
	// unsigned char
	STYPE_U_CHAR,
	// signed char
	STYPE_S_CHAR,
	// unsigned short int
	STYPE_U_SHORT,
	// signed short int
	STYPE_S_SHORT,
	// unsigned int
	STYPE_U_INT,
	// signed int
	STYPE_S_INT,
	// unsigned long int
	STYPE_U_LONG,
	// signed long int
	STYPE_S_LONG,
	// unsigned long long int
	STYPE_U_LONGER,
	// signed long long int
	STYPE_S_LONGER,
	
	/* ===== Float types ===== */
	
	// float
	STYPE_FLOAT,
	// double
	STYPE_DOUBLE,
	// long double
	STYPE_LONG_DOUBLE,
	
	/* ===== Other types ===== */
	
	// _Bool (bool; boolean)
	STYPE_BOOL,
	// void
	STYPE_VOID
	
} simple_type_t;

// Whether a type is signed.
// Only applies to integer types.
static inline bool STYPE_IS_SIGNED(simple_type_t type) {
	return type <= STYPE_S_LONGER && (type & 1);
}

#ifdef CHAR_IS_SIGNED
// 'char' type
// The character's default signedness depends on the machine.
#define STYPE_CHAR STYPE_S_CHAR
#else
// 'char' type
// The character's default signedness depends on the machine.
#define STYPE_CHAR STYPE_U_CHAR
#endif

// Size of 'char' types, in memory words.
#define SSIZE_CHAR   ((CHAR_BITS   - 1) / MEM_BITS + 1)
// Size of 'short' types, in memory words.
#define SSIZE_SHORT  ((SHORT_BITS  - 1) / MEM_BITS + 1)
// Size of 'int' types, in memory words.
#define SSIZE_INT    ((INT_BITS    - 1) / MEM_BITS + 1)
// Size of 'long' types, in memory words.
#define SSIZE_LONG   ((LONG_BITS   - 1) / MEM_BITS + 1)
// Size of 'long long' types, in memory words.
#define SSIZE_LONGER ((LONGER_BITS - 1) / MEM_BITS + 1)

// Size of 'float' types, in memory words.
#define SSIZE_FLOAT       ((FLOAT_BITS       - 1) / MEM_BITS + 1)
// Size of 'double' types, in memory words.
#define SSIZE_DOUBLE      ((DOUBLE_BITS      - 1) / MEM_BITS + 1)
// Size of 'long double' types, in memory words.
#define SSIZE_LONG_DOUBLE ((LONG_DOUBLE_BITS - 1) / MEM_BITS + 1)

// Size of 'bool' types, in memory words.
#define SSIZE_BOOL 1

// Determine the best type that fits a pointer.
#if SSIZE_INT == ADDRESS_TO_MEMWORDS

// Integer type of size_t on target.
#define STYPE_POINTER STYPE_U_INT
// Size of pointer types, in memory words.
#define SSIZE_POINTER SSIZE_INT

#elif SSIZE_SHORT == ADDRESS_TO_MEMWORDS

// Integer type of size_t on target.
#define STYPE_POINTER STYPE_U_SHORT
// Size of pointer types, in memory words.
#define SSIZE_POINTER SSIZE_SHORT

#elif SSIZE_CHAR >= ADDRESS_TO_MEMWORDS

// Integer type of size_t on target.
#define STYPE_POINTER STYPE_U_CHAR
// Size of pointer types, in memory words.
#define SSIZE_POINTER SSIZE_CHAR

#elif SSIZE_LONG >= ADDRESS_TO_MEMWORDS

// Integer type of size_t on target.
#define STYPE_POINTER STYPE_U_LONG
// Size of pointer types, in memory words.
#define SSIZE_POINTER SSIZE_LONG

#elif SSIZE_LONGER >= ADDRESS_TO_MEMWORDS

// Integer type of size_t on target.
#define STYPE_POINTER STYPE_U_LONGER
// Size of pointer types, in memory words.
#define SSIZE_POINTER SSIZE_LONGER

#else

#error "No integer type can fit target address size!"

#endif

// Sizes of the simple types, in memory words, by index.
extern size_t simple_type_size[];
#define SSIZE_SIMPLE(index) (simple_type_size[index])
#define arrSSIZE_BY_INDEX {\
	SSIZE_CHAR,   SSIZE_CHAR,\
	SSIZE_SHORT,  SSIZE_SHORT,\
	SSIZE_INT,    SSIZE_INT,\
	SSIZE_LONG,   SSIZE_LONG,\
	SSIZE_LONGER, SSIZE_LONGER,\
	SSIZE_FLOAT,\
	SSIZE_DOUBLE,\
	SSIZE_LONG_DOUBLE,\
	SSIZE_BOOL,\
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

// Extras added to 'struct asm_scope'.
#ifndef ASM_SCOPE_EXTRAS
#define ASM_SCOPE_EXTRAS
#endif

// Extras added to 'struct funcdef'.
#ifndef FUNCDEF_EXTRAS
#define FUNCDEF_EXTRAS
#endif

#endif //DEFINITIONS_H
