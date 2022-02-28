
#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include "config.h"
#include "stdint.h"
#include "stddef.h"
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

#define IS_LITTLE_ENDIAN defined(LITTLE_ENDIAN)
#define IS_BIG_ENDIAN    defined(BIG_ENDIAN)

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

#define OP_IS_PTR(x)   (x == OP_ADROF     || x == OP_DEREF    || OP_INDEX)
#define OP_IS_BIT(x)   (x >= OP_BIT_NOT   && x <= OP_BIT_XOR)
#define OP_IS_ADD(x)   (x == OP_ADD       || x == OP_SUB)
#define OP_IS_SHIFT(x) (x == OP_SHIFT_L   || x == OP_SHIFT_R)
#define OP_IS_LOGIC(x) (x >= OP_LOGIC_NOT && x <= OP_LOGIC_OR)

typedef enum stmt_type {
    STMT_TYPE_MULTI,
    STMT_TYPE_IF,
    STMT_TYPE_WHILE,
    STMT_TYPE_RET,
    STMT_TYPE_VAR,
    STMT_TYPE_EXPR,
    STMT_TYPE_IASM
} stmt_type_t;

typedef enum expr_type {
    EXPR_TYPE_CONST,
    EXPR_TYPE_IDENT,
    EXPR_TYPE_CALL,
    EXPR_TYPE_MATH1,
    EXPR_TYPE_MATH2
} expr_type_t;

typedef enum gen_var_type {
    // For void functions.
    VAR_TYPE_VOID,
    // Constant value.
    VAR_TYPE_CONST,
    
    // Located at label.
    VAR_TYPE_LABEL,
    // Located in stack.
    VAR_TYPE_STACKFRAME,
    // Located in stack frame.
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

#endif //DEFINITIONS_H
