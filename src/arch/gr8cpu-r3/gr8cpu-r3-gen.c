
// This is the GR8CPU Rev3 generation file.
// Generation files will specify the calling conventions.

#include <gen.h>
#include <asm.h>
#include <stdio.h>

/* =============== Definitions ================ */
// Generator-specific definitions.

#define INSN_JMP		0x0E
#define OFFS_BRANCH		0x0F
#define OFFS_PIE		0x80

#define OFFS_ADD		0x32
#define OFFS_SUB		0x34
#define OFFS_CMP		0x36

#define OFFS_CALC_AX	0x00
#define OFFS_CALC_AY	0x01
#define OFFS_CALC_AV	0x06
#define OFFS_CALC_AM	0x09
#define OFFS_CALC_XV	0x2A
#define OFFS_CALC_XM	0x2B
#define OFFS_CALC_YV	0x32
#define OFFS_CALC_YM	0x33

#define INSN_INC_A		0x3E
#define INSN_DEC_A		0x40
#define INSN_INC_M		0x3F
#define INSN_DEC_M		0x41
#define INSN_INC_X		0x62
#define INSN_DEC_X		0x63
#define INSN_INC_Y		0x6A
#define INSN_DEC_Y		0x6B

#define INSN_MOV_AX		0x17
#define INSN_MOV_AY		0x18
#define INSN_MOV_XA		0x19
#define INSN_MOV_XY		0x1A
#define INSN_MOV_YA		0x1B
#define INSN_MOV_YX		0x1C
#define INSN_MOV_AI		0x1D
#define INSN_MOV_XI		0x1E
#define INSN_MOV_YI		0x1F

#define OFFS_MOV_RI		0x1D

#define OFFS_MOVLD		0x20
#define OFFS_MOVST		0x29
#define OFFS_MOVM_AM	0x00
#define OFFS_MOVM_XM	0x01
#define OFFS_MOVM_YM	0x02
#define OFFS_MOVM_AMX	0x03
#define OFFS_MOVM_AMY	0x04
#define OFFS_MOVM_AP	0x05
#define OFFS_MOVM_APXY	0x06
#define OFFS_MOVM_APX	0x07
#define OFFS_MOVM_APY	0x08

#define OFFS_PUSHR		0x04
#define OFFS_PULLR		0x09
#define INSN_PUSHI		0x07
#define INSN_PUSHM		0x08
#define INSN_POP		0x0C

#define INSN_CALL		0x02
#define INSN_RET		0x03

void			r3_branch			(asm_ctx_t *ctx, branch_cond_t cond, label_t to);
void			r3_math2			(asm_ctx_t *ctx, uint8_t offs, param_spec_t *a, param_spec_t *b, param_spec_t *out, bool doSave);
bool 			r3_collect_args		(asm_ctx_t *ctx, param_spec_t *a, param_spec_t *b, bool doSave, regs_t *useReg, bool allowAltRegs);
bool			r3_mov				(asm_ctx_t *ctx, regs_t reg, param_spec_t *value);
void			r3_save_register	(asm_ctx_t *ctx, regs_t reg);
void			r3_use_register		(asm_ctx_t *ctx, regs_t reg, param_spec_t *usedFor);
void			r3_unuse_register	(asm_ctx_t *ctx, regs_t reg);

void			r3_labelres_pie		(asm_ctx_t *ctx, address_t pos, address_t labelpos);
void			r3_labelres_abs		(asm_ctx_t *ctx, address_t pos, address_t labelpos);

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

// Emits a branch instruction provided context.
void r3_branch(asm_ctx_t *ctx, branch_cond_t cond, label_t to) {
	uint8_t opc = OFFS_BRANCH;
	switch (cond) {
		case (BRANCH_EQUAL):
			opc += 0x00;
			break;
		case (BRANCH_NOT_EQUAL):
			opc += 0x01;
			break;
		case (BRANCH_GREATER):
			opc += 0x02;
			break;
		case (BRANCH_LESSER_EQUAL):
			opc += 0x03;
			break;
		case (BRANCH_LESSER):
			opc += 0x04;
			break;
		case (BRANCH_GREATER_EQUAL):
			opc += 0x05;
			break;
		case (BRANCH_CARRY):
			opc += 0x06;
			break;
		case (BRANCH_NOT_CARRY):
			opc += 0x07;
			break;
	}
	asm_append(ctx, OFFS_PIE + opc);
	asm_label_ref(ctx, to, &r3_labelres_pie);
}

// Emits math code based on provided context.
void r3_math2(asm_ctx_t *ctx, uint8_t offs, param_spec_t *a, param_spec_t *b, param_spec_t *out, bool doSave) {
	if (doSave) printf("MATH 0x%02x ", offs);
	else printf("COMP 0x%02x ", offs);
	desc_param(ctx, a);
	printf(", ");
	desc_param(ctx, b);
	printf("\n");
	regs_t reg;
	bool useRegs = r3_collect_args(ctx, a, b, doSave, &reg, true);
	if (useRegs) {
		if (reg == REGS_A) {
			switch (b->type) {
				case (REGISTER):
					if (b->ptr.regs == REGS_X) asm_append(ctx, offs + OFFS_CALC_AX);
					else asm_append(ctx, offs + OFFS_CALC_AY);
					break;
				case (LABEL):
					asm_append(ctx, OFFS_PIE + offs + OFFS_CALC_AM);
					asm_label_ref(ctx, b->ptr.label, &r3_labelres_pie);
					break;
				case (ADDRESS):
					asm_append(ctx, offs + OFFS_CALC_AM);
					asm_append2(ctx, b->ptr.address);
					break;
				case (CONSTANT):
					asm_append(ctx, offs + OFFS_CALC_AV);
					asm_append(ctx, b->ptr.constant);
					break;
				default:
					printf("TODO: CALC MODE %d\n", b->type);
					break;
			}
		} else if (reg = REGS_X) {
			switch (b->type) {
				case (LABEL):
					asm_append(ctx, OFFS_PIE + offs + OFFS_CALC_XM);
					asm_label_ref(ctx, b->ptr.label, &r3_labelres_pie);
					break;
				case (ADDRESS):
					asm_append(ctx, offs + OFFS_CALC_XM);
					asm_append2(ctx, b->ptr.address);
					break;
				case (CONSTANT):
					asm_append(ctx, offs + OFFS_CALC_XV);
					asm_append(ctx, b->ptr.constant);
					break;
				default:
					printf("TODO: CALC X MODE %d\n", b->type);
					break;
			}
		} else {
			switch (b->type) {
				case (LABEL):
					asm_append(ctx, OFFS_PIE + offs + OFFS_CALC_YM);
					asm_label_ref(ctx, b->ptr.label, &r3_labelres_pie);
					break;
				case (ADDRESS):
					asm_append(ctx, offs + OFFS_CALC_YM);
					asm_append2(ctx, b->ptr.address);
					break;
				case (CONSTANT):
					asm_append(ctx, offs + OFFS_CALC_YV);
					asm_append(ctx, b->ptr.constant);
					break;
				default:
					printf("TODO: CALC Y MODE %d\n", b->type);
					break;
			}
		}
		if (out) {
			*out = (param_spec_t) {
				.type = REGISTER,
				.type_spec = a->type_spec,
				.ptr = { .regs = reg },
				.size = a->size,
				.save_to = NULL
			};
			r3_use_register(ctx, reg, out);
		} else {
			r3_unuse_register(ctx, reg);
		}
	} else {
		printf("TODO: USE LABELS\n");
	}
}

// Collects arguments into registers if applicable.
// Returns true if arguments were moved to registers.
bool r3_collect_args(asm_ctx_t *ctx, param_spec_t *a, param_spec_t *b, bool doSave, regs_t *useReg, bool allowAltRegs) {
	// Check whether this is a matter of bytes.
	if (a->size != 1) {
		// That the types of A and B match are a given.
		return false;
	}
	
	// Check whether alternate registers can be used directly.
	if (a->type == REGISTER && a->ptr.regs != REGS_A && allowAltRegs) {
		if (b->type == CONSTANT || b->type == ADDRESS || b->type == STACK) {
			*useReg = a->ptr.regs;
			if (a->needs_save) {
				r3_save_register(ctx, *useReg);
			}
			return true;
		}
	}
	
	// Move B to an alternate register if it is already in A.
	if (b->type == REGISTER && b->ptr.regs == REGS_A) {
		regs_t alt = ctx->gen_ctx.used[REGS_Y] ? REGS_X : REGS_Y;
		r3_save_register(ctx, alt);
		r3_use_register(ctx, alt, b);
		r3_mov(ctx, alt, b);
	}
	if (a->type != REGISTER) {
		// Move something into A.
		r3_save_register(ctx, REGS_A);
		r3_use_register(ctx, REGS_A, a);
		r3_mov(ctx, REGS_A, a);
		if (a->needs_save && doSave) {
			// Save if required.
			r3_save_register(ctx, REGS_A);
		}
		useReg = REGS_A;
	} else if (a->ptr.regs != REGS_A) {
		// Move something into A.
		regs_t pre = a->ptr.regs;
		r3_save_register(ctx, REGS_A);
		r3_use_register(ctx, REGS_A, a);
		r3_mov(ctx, REGS_A, a);
		if (a->needs_save && doSave) {
			// Save if required.
			a->ptr.regs = pre;
		}
		useReg = REGS_A;
	}
	
	return true;
}

// Moves value into reg, assuming value is one byte in size.
bool r3_mov(asm_ctx_t *ctx, regs_t reg, param_spec_t *value) {
	switch (value->type) {
		case (REGISTER):
			if (value->ptr.regs != reg) {
				uint8_t insn;
				if (reg == REGS_A) {
					if (value->ptr.regs == REGS_X) insn = INSN_MOV_AX;
					else insn = INSN_MOV_AY;
				} else if (reg == REGS_X) {
					if (value->ptr.regs == REGS_A) insn = INSN_MOV_XA;
					else insn = INSN_MOV_XY;
				} else {
					if (value->ptr.regs == REGS_A) insn = INSN_MOV_YA;
					else insn = INSN_MOV_YX;
				}
				asm_append(ctx, insn);
				if (!value->needs_save) r3_unuse_register(ctx, value->ptr.regs);
			}
			break;
		case (LABEL):
			asm_append(ctx, OFFS_PIE + OFFS_MOVLD + reg);
			asm_label_ref(ctx, value->ptr.label, &r3_labelres_pie);
			break;
		case (ADDRESS):
			asm_append(ctx, OFFS_PIE + OFFS_MOVLD + reg);
			asm_append2(ctx, value->ptr.address);
			break;
		case (CONSTANT):
			asm_append(ctx, OFFS_MOV_RI + reg);
			asm_append(ctx, value->ptr.constant);
			break;
		case (STACK):
			if (ctx->stackSize - value->ptr.stackOffset > 1) {
				printf("Error: depth of %d for argument.\n", ctx->stackSize - value->ptr.stackOffset);
				return false;
			}
			asm_append(ctx, OFFS_PULLR + reg);
			ctx->stackSize --;
			break;
		default:
			printf("TODO: GRAB MODE %d\n", value->type);
			return false;
	}
	// value->type = REGISTER;
	// value->ptr.regs = reg;
	return true;
}

// Saves a register to the stack and updates stuff to reflect the situation.
void r3_save_register(asm_ctx_t *ctx, regs_t reg) {
	// Saving is only required if the register is in use and the value is used later on.
	if (ctx->gen_ctx.used[reg] && ctx->gen_ctx.usedFor[reg]->needs_save) {
		param_spec_t *spec = ctx->gen_ctx.usedFor[reg];
		regs_t alt = reg == REGS_Y ? REGS_X : REGS_Y;
		if (spec->save_to) {
			// Append a store instruction.
			asm_append(ctx, OFFS_PIE + OFFS_MOVST + reg);
			asm_label_ref(ctx, spec->save_to, &r3_labelres_pie);
			// Update the location of the stored value.
			spec->type = LABEL;
			spec->ptr.label = spec->save_to;
		} else if (!ctx->gen_ctx.used[alt]) {
			// Append a mov instruction.
			r3_mov(ctx, alt, spec);
			r3_use_register(ctx, alt, spec);
			// Update the location of the stored value.
			spec->type = REGISTER;
			spec->ptr.regs = alt;
		} else{
			// Append a push instruction.
			asm_append(ctx, OFFS_PUSHR + reg);
			// Update the location of the stored value.
			spec->type = STACK;
			spec->ptr.stackOffset = ctx->stackSize;
			ctx->stackSize ++;
		}
		// Free up the register.
		ctx->gen_ctx.used[reg] = false;
		ctx->gen_ctx.usedFor[reg] = NULL;
	}
}

// Mark a register as used and update the location of it's contents.
void r3_use_register(asm_ctx_t *ctx, regs_t reg, param_spec_t *usedFor) {
	ctx->gen_ctx.used[reg] = true;
	ctx->gen_ctx.usedFor[reg] = usedFor;
}

// Mark a register as unused.
void r3_unuse_register(asm_ctx_t *ctx, regs_t reg) {
	ctx->gen_ctx.used[reg] = false;
}

// Appens ASM data for PIE label reference.
void r3_labelres_pie(asm_ctx_t *ctx, address_t pos, address_t labelpos) {
	asm_append2(ctx, labelpos - pos);
}

// Appens ASM data for absolute label reference.
void r3_labelres_abs(asm_ctx_t *ctx, address_t pos, address_t labelpos) {
	asm_append2(ctx, labelpos);
}

/* ================ Generation ================ */
// Generation of simple statements.

// Anything that needs to happen after asm_init.
void gen_init(asm_ctx_t *ctx) {
	ctx->gen_ctx = (gen_ctx_t) {
		.used = { false, false, false },
		.usedFor = { NULL, NULL, NULL },
		.saveYReg = false
	};
}

// Generate method entry with optional params.
void gen_method_entry(asm_ctx_t *ctx, param_spec_t **params, int nParams) {
	if (nParams == 0) {
		asm_append(ctx, OFFS_PUSHR + REGS_Y);
		ctx->gen_ctx.saveYReg = true;
		return;
	}
	if (nParams <= 3) {
		for (int i = 0; i < nParams; i++) {
			if (params[i]->size > 1) goto label_params;
		}
		ctx->gen_ctx.saveYReg = nParams <= 2;
		if (ctx->gen_ctx.saveYReg) asm_append(ctx, OFFS_PUSHR + REGS_Y);
		for (int i = 0; i < nParams; i++) {
			params[i]->type = REGISTER;
			params[i]->ptr.regs = (regs_t) i;
			r3_use_register(ctx, (regs_t) i, params[i]);
		}
		return;
	}
	label_params:
	asm_append(ctx, OFFS_PUSHR + REGS_Y);
	ctx->gen_ctx.saveYReg = true;
	printf("TODO: LABEL PARAMS\n");
}

// Generate method return with specified return type.
void gen_method_ret(asm_ctx_t *ctx, param_spec_t *returnType) {
	printf("RET ");
	desc_param(ctx, returnType);
	printf("\n");
	if (returnType->size == 1) {
		r3_mov(ctx, REGS_A, returnType);
	} else {
		printf("TODO: RET SIZE %d\n", returnType->size);
	}
	if (ctx->gen_ctx.saveYReg) {
		asm_append(ctx, OFFS_PULLR + REGS_Y);
	}
	while (ctx->stackSize) {
		ctx->stackSize --;
		asm_append(ctx, INSN_POP);
	}
	asm_append(ctx, INSN_RET);
}

// Generate simple math.
void gen_math2(asm_ctx_t *ctx, param_spec_t *a, param_spec_t *b, operator_t op, param_spec_t *out) {
	uint8_t offs;
	switch (op) {
		case (OP_ADD):
			offs = OFFS_ADD;
			break;
		case (OP_SUB):
			offs = OFFS_SUB;
			break;
	}
	r3_math2(ctx, offs, a, b, out, true);
}

// Generate simple math.
void gen_math1(asm_ctx_t *ctx, param_spec_t *a, operator_t op) {
	if (op == OP_INC) {
		if (a->type == ADDRESS) {
			asm_append(ctx, INSN_INC_M);
			asm_append2(ctx, a->ptr.address);
		} else if (a->type == LABEL) {
			asm_append(ctx, OFFS_PIE + INSN_INC_M);
			asm_label_ref(ctx, a->ptr.label, &r3_labelres_pie);
		} else if (a->type == REGISTER) {
			switch (a->ptr.regs) {
				case (REGS_A):
					asm_append(ctx, INSN_INC_A);
					break;
				case (REGS_X):
					asm_append(ctx, INSN_INC_X);
					break;
				case (REGS_Y):
					asm_append(ctx, INSN_INC_Y);
					break;
			}
		}
	} else if (op == OP_DEC) {
		if (a->type == ADDRESS) {
			asm_append(ctx, INSN_DEC_M);
			asm_append2(ctx, a->ptr.address);
		} else if (a->type == LABEL) {
			asm_append(ctx, OFFS_PIE + INSN_DEC_M);
			asm_label_ref(ctx, a->ptr.label, &r3_labelres_pie);
		} else if (a->type == REGISTER) {
			switch (a->ptr.regs) {
				case (REGS_A):
					asm_append(ctx, INSN_DEC_A);
					break;
				case (REGS_X):
					asm_append(ctx, INSN_DEC_X);
					break;
				case (REGS_Y):
					asm_append(ctx, INSN_DEC_Y);
					break;
			}
		}
	}
}

// Generate comparison, returning 0 or 1.
void gen_comp(asm_ctx_t *ctx, param_spec_t *a, param_spec_t *b, branch_cond_t op, param_spec_t *out) {
	r3_math2(ctx, OFFS_CMP, a, b, out, false);
}

// Generate branch after comparison.
void gen_branch(asm_ctx_t *ctx, param_spec_t *a, param_spec_t *b, branch_cond_t cond, label_t to) {
	r3_math2(ctx, OFFS_CMP, a, b, NULL, false);
	r3_branch(ctx, cond, to);
}

// Generate jump.
void gen_jump(asm_ctx_t *ctx, label_t to) {
	asm_append(ctx, OFFS_PIE + INSN_JMP);
	asm_label_ref(ctx, to, &r3_labelres_pie);
}

// Generate code to copy the value of 'src' to the location of 'dest'.
// Doing this is allowed to change the location of either value.
bool gen_mov(asm_ctx_t *ctx, param_spec_t *dest, param_spec_t *src) {
	printf("MOV ");
	desc_param(ctx, dest);
	printf(", ");
	desc_param(ctx, src);
	printf("\n");
	if (dest->size > 1) {
		printf("TODO: MOV SIZE %d\n", dest->size);
	}
	if (dest->type == REGISTER) {
		r3_mov(ctx, dest->ptr.regs, src);
	} else {
		printf("TODO: MOV TYPE %d, %d\n", dest->type, src->type);
	}
}

// Reserve a location for a variable of a certain type.
void gen_var(asm_ctx_t *ctx, type_spec_t *type, param_spec_t *out) {
	printf("VAR\n");
	if (type->size > 1) {
		printf("TODO: VAR SIZE %d\n", type->size);
	}
	// Check if we have a free register.
	for (int i = 0; i < 3; i++) {
		if (!ctx->gen_ctx.used[i]) {
			out->type = REGISTER;
			out->ptr.regs = i;
			r3_use_register(ctx, i, out);
			return;
		}
	}
	// Bar that, we'll use the stack.
	asm_append(ctx, INSN_PUSHI);
	asm_append(ctx, 0);
	out->type = STACK;
	out->ptr.stackOffset = ctx->stackSize;
	ctx->stackSize ++;
}

// Reserve a location for a variable with a given value.
void gen_var_assign(asm_ctx_t *ctx, param_spec_t *val, param_spec_t *out) {
	printf("VAR ");
	desc_param(ctx, val);
	printf("\n");
	if (val->size > 1) {
		printf("TODO: VAR SIZE %d\n", val->size);
	}
	if (!out->needs_save) {
		out->type = val->type;
		out->ptr = val->ptr;
		return;
	}
	// Check if we have a free register.
	for (int i = 0; i < 3; i++) {
		if (!ctx->gen_ctx.used[i]) {
			out->type = REGISTER;
			out->ptr.regs = i;
			r3_use_register(ctx, i, out);
			r3_mov(ctx, i, val);
			return;
		}
	}
	// Bar that, we'll use the stack.
	switch (val->type) {
		case (REGISTER):
			asm_append(ctx, OFFS_PUSHR + val->ptr.regs);
			break;
		case (ADDRESS):
			asm_append(ctx, INSN_PUSHM);
			asm_append2(ctx, val->ptr.address);
			break;
		case (LABEL):
			asm_append(ctx, INSN_PUSHM);
			asm_label_ref(ctx, val->ptr.label, &r3_labelres_pie);
			break;
		case (CONSTANT):
			asm_append(ctx, INSN_PUSHI);
			asm_append(ctx, val->ptr.constant);
			break;
		case (STACK):
			printf("OH_NO\n");
			break;
	}
	out->type = STACK;
	out->ptr.stackOffset = ctx->stackSize;
	ctx->stackSize ++;
}

// Generate code to move 'from' to the same location as 'to'.
// Returns true on success.
bool gen_restore(asm_ctx_t *ctx, param_spec_t *from, param_spec_t *to) {
	switch (to->type) {
		case (REGISTER):
			return r3_mov(ctx, to->ptr.regs, from);
		default:
			printf("TODO: RESTORE %d\n", to->type);
			break;
	}
}

/* ==== Architecture-optimised generation. ==== */
// Any of these may return data to be passed to the complementary methods.

// Support for simple for loop given limit.
char agen_supports_fori(asm_ctx_t *ctx, param_spec_t *limit) { return 0; }

// Simple for (int i; i<123; i++) loop.
void *agen_fori_pre(asm_ctx_t *ctx, param_spec_t *limit) { return NULL; }
// Simple for (int i; i<123; i++) loop.
void *agen_fori_post(asm_ctx_t *ctx, param_spec_t *limit) { return NULL; }