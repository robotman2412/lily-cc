
#include "elf.h"
#include <asm_postproc.h>

static bool host_little_endian;
static bool swap_endian;

static inline uint_least16_t ELF_U16(uint_least16_t in) {
	return (swap_endian ? ((in) >> 8) | ((in) << 8) : (in));
}
static inline uint_least32_t ELF_U32(uint_least32_t in) {
	return (swap_endian ?
		  (((in) >> 24) & 0x000000ff)
		| (((in) >>  8) & 0x0000ff00)
		| (((in) <<  8) & 0x00ff0000)
		| (((in) << 24) & 0xff000000)
		: (in));
}
static inline uint_least64_t ELF_U64(uint_least64_t in) {
	return (swap_endian ?
		  (((in) >> 56) & 0x00000000000000ffL)
		| (((in) >> 48) & 0x000000000000ff00L)
		| (((in) >> 24) & 0x0000000000ff0000L)
		| (((in) >>  8) & 0x00000000ff000000L)
		| (((in) <<  8) & 0x000000ff00000000L)
		| (((in) << 24) & 0x0000ff0000000000L)
		| (((in) << 48) & 0x00ff000000000000L)
		| (((in) << 56) & 0xff00000000000000L)
		: (in));
}

static void output_elf32_reduce(asm_ctx_t *ctx, uint8_t chunk_type, size_t chunk_len, uint8_t *chunk_data, void *args) {
	
}

void output_elf32(asm_ctx_t *ctx, bool create_executable) {
	// Check whether the host is little endian.
	union {
		uint16_t num;
		uint8_t  val[2];
	} endtest;
	endtest.num = 0x1234;
	host_little_endian = endtest.val[0] == 0x34;
	swap_endian = host_little_endian != (bool) IS_LITTLE_ENDIAN;
	
	ctx->pc = 0;
	
	// Find the desired section order.
	size_t n_sect      = ctx->sections->numEntries;
	char **sect_ids    = ctx->sections->strings;
	asm_sect_t **sects = (asm_sect_t **) ctx->sections->values;
	
	// Iterate to output.
	asm_ppc_iterate(ctx, n_sect, sect_ids, sects, &output_elf32_reduce, NULL, true);
}
