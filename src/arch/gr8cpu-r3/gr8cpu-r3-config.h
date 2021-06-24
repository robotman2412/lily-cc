
// Size in bytes of addresses in memory.
#define MEMW_BYTES 1
// Size in bytes of the CPU's standard word size.
#define WORD_BYTES 1
// Size in bytes of the CPU's addressing (pointers, size_t).
#define ADDR_BYTES 2

// Total number of registers.
#define NUM_REGS 3

// Whether to append little endian or big endian data.
#define IS_LITTLE_ENDIAN

// For convenience.
#define REG_NAMES { \
	"a", "x", "y" \
}

// Enable debug logs.
//#define ENABLE_DEBUG_LOGS
