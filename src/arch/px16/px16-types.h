
#include <stdbool.h>

#ifndef param_spec_t
typedef struct param_spec param_spec_t;
#endif

// Register type used by generator.
typedef enum regs {
	REGS_R0,
	REGS_R1,
	REGS_R2,
	REGS_R3,
	REGS_R4
} regs_t;

// Additional generator context.
typedef struct gen_ctx {
	bool used[5];
	param_spec_t *usedFor[5];
} gen_ctx_t;

// For convenience.
__attribute__((packed))
typedef struct px16_insn_word {
	uint8_t o	: 5;
	uint8_t a	: 3;
	bool	p	: 1;
	uint8_t b	: 3;
	bool	q	: 1;
	uint8_t x	: 2;
	bool	y	: 1;
} px16_insn_word_t;
