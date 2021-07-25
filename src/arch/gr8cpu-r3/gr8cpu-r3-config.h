
// Size in bytes of addresses in memory.
#define MEMW_BYTES 1
// Size in bytes of the CPU's standard word size.
#define WORD_BYTES 1
// Size in bytes of the CPU's addressing (pointers, size_t).
#define ADDR_BYTES 2

// Size in bytes of char.
#define CHAR_BYTES 1
// Size in bytes of short int.
#define SHORT_BYTES 2
// Size in bytes of int.
#define INT_BYTES 2
// Size in bytes of long int.
#define LONG_BYTES 4
// Size in bytes of long long int.
#define LONG_LONG_BYTES 8

// Total number of registers.
#define NUM_REGS 3

// Whether to append little endian or big endian data.
#define IS_LITTLE_ENDIAN

// Char is unsigned by default.
#define IS_CHAR_UNSIGNED

// For convenience.
#define REG_NAMES { \
	"a", "x", "y" \
}

// Enable debug logs.
#define ENABLE_DEBUG_LOGS
