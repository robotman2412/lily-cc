
#include <stdbool.h>

#ifndef param_spec_t
typedef struct param_spec param_spec_t;
#endif

// Register type used by generator.
typedef enum regs {
	REGS_A,
	REGS_X,
	REGS_Y
} regs_t;

// Additional generator context.
typedef struct gen_ctx {
	bool used[3];
	param_spec_t *usedFor[3];
	bool saveYReg;
} gen_ctx_t;
