
#ifndef CONFIG_TYPES_H
#define CONFIG_TYPES_H

#include <stdint.h>
#include <stddef.h>

// General configs and typedefs.

// Ensure USE_LITTLE_ENDIAN is not defined.
#ifdef USE_LITTLE_ENDIAN
#error "USE_LITTLE_ENDIAN should not be defined by arch-config.h!"
#endif

// If IS_BIG_ENDIAN is defined, use big endian.
// If neither is defined, use little endian.
#ifdef IS_BIG_ENDIAN
#define USE_LITTLE_ENDIAN 0
#elif defined(IS_LITTLE_ENDIAN)
#define USE_LITTLE_ENDIAN 1
#else
#error "Neither IS_LITTLE_ENDIAN nor IS_BIG_ENDIAN are defined!"
#endif

// Ensure WORD_BYTES is defined.
#ifndef WORD_BYTES
#error "WORD_BYTES is not defined!"
#endif

// Default ADDR_BYTES to equal WORD_BYTES.
#ifndef ADDR_BYTES
#define ADDR_BYTES WORD_BYTES
#endif

// Default MEMW_BYTES to equal WORD_BYTES.
#ifndef MEMW_BYTES
#define MEMW_BYTES WORD_BYTES
#endif

// Smallest word size for memory.
// This may not be larger than word size for CPU or word size for memory addresses.
// This is the basis for the sizes of various types.
#if MEMW_BYTES == 1
typedef uint8_t memword_t;
typedef uint16_t memword2_t;
typedef uint32_t memword4_t;
#elif MEMW_BYTES == 2
typedef uint16_t memword_t;
typedef uint32_t memword2_t;
typedef uint64_t memword4_t;
#elif MEMW_BYTES <= 4
typedef uint32_t memword_t;
typedef uint64_t memword2_t;
typedef uint64_t memword4_t;
#elif MEMW_BYTES <= 8
typedef uint64_t memword_t;
typedef uint64_t memword2_t;
typedef uint64_t memword4_t;
#endif

// Word size for CPU.
// This may not smaller than word size for memory.
// This is usually the integer size.
#if WORD_BYTES == 1
typedef uint8_t word_t;
typedef uint16_t word2_t;
typedef uint32_t word4_t;
#elif WORD_BYTES == 2
typedef uint16_t word_t;
typedef uint32_t word2_t;
typedef uint64_t word4_t;
#elif WORD_BYTES <= 4
typedef uint32_t word_t;
typedef uint64_t word2_t;
typedef uint64_t word4_t;
#elif WORD_BYTES <= 8
typedef uint64_t word_t;
typedef uint64_t word2_t;
typedef uint64_t word4_t;
#endif

// Word size for memory addresses.
// This may not be smaller than word size for memory.
#if ADDR_BYTES == 1
typedef uint8_t address_t;
#elif ADDR_BYTES == 2
typedef uint16_t address_t;
#elif ADDR_BYTES <= 4
typedef uint32_t address_t;
#elif ADDR_BYTES <= 8
typedef uint64_t address_t;
#endif

// Word conversion ratios.
#define ADDR_NUM_WORDS ((ADDR_BYTES + WORD_BYTES - 1) / WORD_BYTES)
#define ADDR_NUM_MEMWS ((ADDR_BYTES + MEMW_BYTES - 1) / MEMW_BYTES)
#define WORD_NUM_MEMWS ((WORD_BYTES + MEMW_BYTES - 1) / MEMW_BYTES)
#define MEMW_NUM_WORDS ((MEMW_BYTES + WORD_BYTES - 1) / WORD_BYTES)

// Enable debug logs if the config states to do so.
#ifdef ENABLE_DEBUG_LOGS
#define DEBUG(...) printf("Debug: " __VA_ARGS__)
#else
#define DEBUG(...)
#endif

#endif // CONFIG_TYPES_H
