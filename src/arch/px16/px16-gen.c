
// This is the GR8CPU Rev3 generation file.
// Generation files will specify the calling conventions.

#include <gen.h>
#include <asm.h>
#include <stdio.h>

/* =============== Definitions ================ */
// Generator-specific definitions.

#define OP_ADD		0x00
#define OP_SUB		0x01
#define OP_ADDC		0x02
#define OP_SUBC		0x03
#define OP_INC		0x04
#define OP_SHIFT	0x05
#define OP_AND		0x06
#define OP_OR		0x07
#define OP_CMP		0x08
#define OP_XOR		0x09
#define OP_CMPC		0x0A

#define OP_SEI		0x19
#define OP_OPER		0x1A
#define OP_JMP		0x1B
#define OP_MOV		0x1C
#define	OP_LEA		0x1E
#define	OP_HLT		0x1F

/* ================ Utilities ================= */
// Generator-specific utilities.

// Prints out some debug info about param.
static void desc_param(asm_ctx_t *ctx, param_spec_t *param) {
	char *r = "AXY";
	switch (param->type) {
		case (REGISTER):
			printf("REG(%c)", r[param->ptr.regs]);
			break;
		case (LABEL):
			printf("LAB(%s)", param->ptr.label);
			break;
		case (ADDRESS):
			printf("ADR(0x%04x)", param->ptr.address);
			break;
		case (CONSTANT):
			printf("VAL(0x%02x)", param->ptr.constant);
			break;
		case (STACK):
			printf("STK(%d)", param->ptr.stackOffset);
			break;
		default:
			printf("?%d?", param->type);
			break;
	}
	if (param->needs_save) {
		printf(" save");
	}
}

/* ================ Generation ================ */
// Generation of simple statements.

/* -------- Utilities --------- */

// Anything that needs to happen after asm_init.
void gen_init(asm_ctx_t *ctx) {
	ctx->gen_ctx = (gen_ctx_t) {
		.used = { false, false, false, false, false },
		.usedFor = { NULL, NULL, NULL, NULL, NULL }
	};
}

// Notify the generator of a pointer changing.
void gen_update_ptr(asm_ctx_t *ctx, param_spec_t* from, param_spec_t *to) {
	for (int i = 0; i < 3; i ++) {
		if (ctx->gen_ctx.used[i] && ctx->gen_ctx.usedFor[i] == from) {
			ctx->gen_ctx.usedFor[i] = to;
		}
	}
}

// Generate casting for numeric types.
// Casting may not generate any ASM.
void gen_cast_num(asm_ctx_t *ctx, param_spec_t *param, type_spec_t to) {
	type_spec_t from = param->type_spec;
	// Casting constants is very simple.
	if (param->type == CONSTANT) {
		word4_t val = param->ptr.constant;
		if (to.type == BOOL) {
			// Ensure the value is either 0 or 1.
			// Anything nonzero becomes one.
			val = val != 0;
		} else {
			// Check whether we must carry-extend.
			bool wasSigned = from.type >= NUM_HHI && from.type <= NUM_LLI;
			bool isSigned = to.type >= NUM_HHI && to.type <= NUM_LLI;
			if (wasSigned && isSigned && to.size > from.size) {
				// Fill the higher bytes with ones or zeroes respectively.
				bool sign = val >> (8 * from.size - 1);
				if (sign) {
					for (int i = from.size; i < to.size; i++) {
						val |= 0xff << (8 * i);
					}
				} else {
					for (int i = from.size; i < to.size; i++) {
						val &= ~(0xff << (8 * i));
					}
				}
			}
			// TODO: Maybe AND-onate.
		}
		param->ptr.constant = val;
		param->type_spec = to;
		param->size = to.size;
		return;
	}
}

/* --------- Methods ---------- */

// Generate method entry with optional params.
void gen_method_entry(asm_ctx_t *ctx, funcdef_t *funcdef, param_spec_t **params, int nParams) {
	
}

// Generate method return with specified return type.
void gen_method_ret(asm_ctx_t *ctx, param_spec_t *returnType) {
	
}

// Generate simple math.
void gen_math2(asm_ctx_t *ctx, param_spec_t *a, param_spec_t *b, operator_t op, param_spec_t *out) {
	
}

// Generate simple math.
void gen_math1(asm_ctx_t *ctx, param_spec_t *a, operator_t op) {
	
}

// Generate comparison, returning 0 or 1.
void gen_comp(asm_ctx_t *ctx, param_spec_t *a, param_spec_t *b, branch_cond_t op, param_spec_t *out) {
	
}

/* ------- Flow control ------- */

// Generate branch after comparison.
void gen_branch(asm_ctx_t *ctx, param_spec_t *a, param_spec_t *b, branch_cond_t cond, label_t to) {
	
}

// Generate jump.
void gen_jump(asm_ctx_t *ctx, label_t to) {
	
}

/* -------- Variables --------- */

// Reserve a location for a variable of a certain type.
void gen_var(asm_ctx_t *ctx, type_spec_t *type, param_spec_t *out) {
	
}

// Reserve a location for a variable with a given value.
void gen_var_assign(asm_ctx_t *ctx, param_spec_t *val, param_spec_t *out) {
	
}

// Generate code to copy the value of 'src' to the location of 'dest'.
// Doing this is allowed to change the location of either value.
bool gen_mov(asm_ctx_t *ctx, param_spec_t *dest, param_spec_t *src) {
	
}

// Generate code to move 'from' to the same location as 'to'.
// Returns true on success.
bool gen_restore(asm_ctx_t *ctx, param_spec_t *from, param_spec_t *to) {
	
}

/* ==== Architecture-optimised generation. ==== */
