
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
