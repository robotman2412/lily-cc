
#include "gr8cpu-r3_gen.h"
#include "gen_util.h"
#include "definitions.h"
#include "asm.h"
#include "malloc.h"
#include "string.h"

#ifdef ENABLE_DEBUG_LOGS
char *b_insn_names[] = {"BEQ", "BNE", "BGT", "BLE", "BLT", "BGE", "BCS", "BCC"};
#endif

/* ======== Gen-specific helper functions ======== */

static inline void r3_branch_to_var(asm_ctx_t *ctx, uint8_t b_insn, gen_var_t *output) {
	uint8_t regno   = REG_A;
	uint8_t n_words = 2;
	// Helper var.
	gen_var_t helper = {
		.type = VAR_TYPE_CONST,
		.iconst = 0
	};
	// Write coditional assignment.
	char *l_true = asm_get_label(ctx);
	char *l_skip = asm_get_label(ctx);
	DEBUG_GEN("  %s %s\n", b_insn_names[b_insn - OFFS_BRANCH - PIE(ctx)], l_true);
	asm_write_memword(ctx, b_insn);
	asm_write_label_ref(ctx, l_true, 0, OFFS(ctx));
	// Code for false.
	r3_load_part(ctx, &helper, regno, 0);
	DEBUG_GEN("  JMP %s\n", l_skip);
	asm_write_memword(ctx, INSN_JMP);
	asm_write_label_ref(ctx, l_skip, 0, OFFS(ctx));
	// Code for true.
	helper.iconst = 1;
	asm_write_label(ctx, l_true);
	r3_load_part(ctx, &helper, regno, 0);
	// Skip label.
	asm_write_label(ctx, l_skip);
	// Storage code.
	r3_store_part(ctx, output, regno, 0);
	r3_load_part(ctx, &helper, regno, 1);
	for (uint8_t i = 1; i < n_words; i++)
		r3_store_part(ctx, output, regno, i);
}

static inline uint8_t r3_var_to_branch(asm_ctx_t *ctx, gen_var_t *cond) {
	if (cond->type == VAR_TYPE_LABEL) {
		uint8_t n_words = 2;
		char *label = cond->label;
		// Load first byte.
		DEBUG_GEN("  MOV A, [%s+0]\n", label);
		asm_write_memword(ctx, OFFS_MOVLD + OFFS_MOVM_AM + PIE(ctx));
		asm_write_label_ref(ctx, label, 0, OFFS(ctx));
		// OR the following bytes.
		for (int i = 1; i < n_words; i++) {
			DEBUG_GEN("  OR A, [%s+%d]\n", label, i);
			asm_write_memword(ctx, OFFS_BIT_AM + OFFS_OR + PIE(ctx));
			asm_write_label_ref(ctx, label, i, OFFS(ctx));
		}
		// The branch instruction is now BNE.
		return INSN_BNE;
	} else if (cond->type == VAR_TYPE_COND) {
		// Take the branch instruction from the condition.
		return cond->cond;
	}
}

// Creates a set of branch instructions for the given conditions.
void r3_branch(asm_ctx_t *ctx, gen_var_t *cond, char *l_true, char *l_false) {
	uint8_t b_insn = r3_var_to_branch(ctx, cond);
	
	if (l_true) {
		// Non-inverted branch.
		DEBUG_GEN("  %s %s\n", b_insn_names[(b_insn & 0x7f) - OFFS_BRANCH], l_true);
		asm_write_memword(ctx, b_insn | PIE(ctx));
		asm_write_label_ref(ctx, l_true, 0, OFFS(ctx));
		if (l_false) {
			// With else label.
			DEBUG_GEN("  JMP %s\n", l_false);
			asm_write_memword(ctx, INSN_JMP + PIE(ctx));
			asm_write_label_ref(ctx, l_false, 0, OFFS(ctx));
		}
	} else if (l_false) {
		// Inverted branch.
		b_insn = INV_BR(b_insn);
		DEBUG_GEN("  %s %s\n", b_insn_names[(b_insn & 0x7f) - OFFS_BRANCH], l_false);
		asm_write_memword(ctx, b_insn | PIE(ctx));
		asm_write_label_ref(ctx, l_false, 0, OFFS(ctx));
	}
}

// Moves a byte of the variable into the given register.
void r3_load_part(asm_ctx_t *ctx, gen_var_t *var, uint8_t regno, uint8_t offs) {
	switch (var->type) {
		case VAR_TYPE_CONST:
			DEBUG_GEN("  MOV %s, 0x%02x\n", reg_names[regno], var->iconst >> (offs * 8));
			// Correct "MOV reg, val" instruction.
			asm_write_memword(ctx, OFFS_MOV_RI + regno);
			// Selected byte of value.
			asm_write_memword(ctx, var->iconst >> (offs * 8));
			break;
		case VAR_TYPE_LABEL:
			DEBUG_GEN("  MOV %s, [%s+%d]\n", reg_names[regno], var->label, offs);
			// Correct "MOV reg, [label+offs]" instruction.
			asm_write_memword(ctx, OFFS_MOVLD + OFFS_MOVM_RM + regno + PIE(ctx));
			// Label reference.
			asm_write_label_ref(ctx, var->label, offs, OFFS(ctx));
			break;
	}
}

// Moves the given register into a byte of the variable.
void r3_store_part(asm_ctx_t *ctx, gen_var_t *var, uint8_t regno, uint8_t offs) {
	if (var->type == VAR_TYPE_RETVAL) {
		// TODO: replace it if not possible.
		// Correct MOV reg, A instruction.
		DEBUG_GEN("  MOV %s, A\n", reg_names[offs + REG_X]);
		asm_write_memword(ctx, offs ? INSN_MOV_YA : INSN_MOV_XA);
	}
	if (var->type == VAR_TYPE_LABEL) {
		// Correct "MOV [label+offs], reg" instruction.
		DEBUG_GEN("  MOV [%s+%d], %s\n", var->label, offs, reg_names[regno]);
		asm_write_memword(ctx, OFFS_MOVST + OFFS_MOVM_RM + regno + PIE(ctx));
		// Label reference.
		asm_write_label_ref(ctx, var->label, offs, OFFS(ctx));
	}
}

// Moves a long into the registers X and Y.
// Used before function entry to functions which take exactly one two-byte integer.
gen_var_t *r3_movl_to_mem(asm_ctx_t *ctx, char *label) {
	// Move X to memory.
	DEBUG_GEN("  MOV [%s+0], X\n", label);
	asm_write_memword  (ctx, OFFS_MOVST + OFFS_MOVM_XM + PIE(ctx));
	asm_write_label_ref(ctx, label, 0, OFFS(ctx));
	// Move Y to memory.
	DEBUG_GEN("  MOV [%s+0], Y\n", label);
	asm_write_memword  (ctx, OFFS_MOVST + OFFS_MOVM_YM + PIE(ctx));
	asm_write_label_ref(ctx, label, 1, OFFS(ctx));
}

// Moves a long into memory.
// Used before function return from functions which return exactly one two-byte integer.
void r3_movl_to_reg(asm_ctx_t *ctx, gen_var_t *var) {
	if (var->type == VAR_TYPE_LABEL) {
		// Label-ish.
		char *label = var->label;
		// Move memory to X.
		DEBUG_GEN("  MOV X, [%s+0]\n", label);
		asm_write_memword  (ctx, OFFS_MOVLD + OFFS_MOVM_XM + PIE(ctx));
		asm_write_label_ref(ctx, label, 0, OFFS(ctx));
		// Move memory to Y.
		DEBUG_GEN("  MOV Y, [%s+1]\n", label);
		asm_write_memword  (ctx, OFFS_MOVLD + OFFS_MOVM_YM + PIE(ctx));
		asm_write_label_ref(ctx, label, 1, OFFS(ctx));
	} else if (var->type == VAR_TYPE_CONST) {
		// The constant.
		address_t iconst = var->iconst;
		// Move memory to X.
		DEBUG_GEN("  MOV X, %02x\n", iconst & 255);
		asm_write_memword  (ctx, OFFS_MOV_RI + REG_X);
		asm_write_memword  (ctx, iconst & 255);
		// Move memory to Y.
		DEBUG_GEN("  MOV Y, %02x\n", iconst >> 8);
		asm_write_memword  (ctx, OFFS_MOV_RI + REG_Y);
		asm_write_memword  (ctx, iconst >> 8);
	}
}



// Dereference a pointer.
gen_var_t *r3_deref(asm_ctx_t *ctx, gen_var_t *output, gen_var_t *ptr) {
	uint8_t n_words = 2;
	if (!output || output->type == VAR_TYPE_COND || (ptr->type != VAR_TYPE_CONST && output->type == VAR_TYPE_RETVAL)) {
		output = (gen_var_t *) malloc(sizeof(gen_var_t));
		*output = (gen_var_t) {
			.label = r3_get_tmp(ctx),
			.type  = VAR_TYPE_LABEL
		};
	}
	if (ptr->type == VAR_TYPE_CONST) {
		address_t addr = ptr->iconst;
		for (address_t i = 0; i < n_words; i++) {
			DEBUG_GEN("  MOV A, [0x%04x]\n", addr + i);
			asm_write_memword(ctx, OFFS_MOVLD + OFFS_MOVM_AP);
			asm_write_address(ctx, addr + i);
			r3_store_part(ctx, output, REG_A, i);
		}
		return output;
	}
	char *label = ptr->label;
	if (ptr->type != VAR_TYPE_LABEL) {
		label = r3_get_tmp(ctx);
		gen_var_t dst = {
			.type = VAR_TYPE_LABEL,
			.label = label
		};
		gen_mov(ctx, &dst, ptr);
	}
	// Wirst word.
	DEBUG_GEN("  MOV A, (%s)\n", label);
	asm_write_memword(ctx, OFFS_MOVLD + OFFS_MOVM_AP + PIE(ctx));
	asm_write_label_ref(ctx, ptr->label, 0, OFFS(ctx));
	r3_store_part(ctx, output, REG_A, 0);
	// Set offset.
	DEBUG_GEN("  MOV X, 0x01\n");
	asm_write_memword(ctx, INSN_MOV_XI);
	asm_write_memword(ctx, 1);
	// Second word.
	DEBUG_GEN("  MOV A, X(%s)\n", label);
	asm_write_memword(ctx, OFFS_MOVLD + OFFS_MOVM_APX + PIE(ctx));
	asm_write_label_ref(ctx, ptr->label, 0, OFFS(ctx));
	r3_store_part(ctx, output, REG_A, 1);
	// Additional words.
	for (address_t i = 2; i < n_words; i++) {
		DEBUG_GEN("  INC X\n");
		asm_write_memword(ctx, INSN_INC_X);
		DEBUG_GEN("  MOV A, X(%s)\n", label);
		asm_write_memword(ctx, OFFS_MOVLD + OFFS_MOVM_APX + PIE(ctx));
		asm_write_label_ref(ctx, ptr->label, 0, OFFS(ctx));
		r3_store_part(ctx, output, REG_A, i);
	}
	return output;
}

// Assign a value to the pointer.
gen_var_t *r3_deref_set(asm_ctx_t *ctx, gen_var_t *dst, gen_var_t *src) {
	uint8_t n_words = 2;
	if (dst->type == VAR_TYPE_CONST) {
		// This is a simple store.
		address_t addr = dst->iconst;
		for (address_t i = 0; i < n_words; i++) {
			r3_load_part(ctx, src, REG_A, i);
			DEBUG_GEN("  MOV [0x%04x], A\n", addr + i);
			asm_write_memword(ctx, OFFS_MOVST + OFFS_MOVM_AP);
			asm_write_address(ctx, addr + i);
		}
		return src;
	}
	
	char *label = dst->label;
	if (dst->type != VAR_TYPE_LABEL) {
		// Ensure the pointer is a label here.
		label = r3_get_tmp(ctx);
		gen_var_t tmp = {
			.type = VAR_TYPE_LABEL,
			.label = label
		};
		gen_mov(ctx, &tmp, dst);
	}
	// Wirst word.
	r3_load_part(ctx, src, REG_A, 0);
	DEBUG_GEN("  MOV (%s), A\n", label);
	asm_write_memword(ctx, OFFS_MOVST + OFFS_MOVM_AP + PIE(ctx));
	asm_write_label_ref(ctx, dst->label, 0, OFFS(ctx));
	// Set offset.
	DEBUG_GEN("  MOV X, 0x01\n");
	asm_write_memword(ctx, INSN_MOV_XI);
	asm_write_memword(ctx, 1);
	// Second word.
	r3_load_part(ctx, src, REG_A, 1);
	DEBUG_GEN("  MOV A, X(%s)\n", label);
	asm_write_memword(ctx, OFFS_MOVST + OFFS_MOVM_APX + PIE(ctx));
	asm_write_label_ref(ctx, dst->label, 0, OFFS(ctx));
	// Additional words.
	for (address_t i = 2; i < n_words; i++) {
		DEBUG_GEN("  INC X\n");
		asm_write_memword(ctx, INSN_INC_X);
		r3_load_part(ctx, src, REG_A, i);
		DEBUG_GEN("  MOV X(%s), A\n", label);
		asm_write_memword(ctx, OFFS_MOVST + OFFS_MOVM_APX + PIE(ctx));
		asm_write_label_ref(ctx, dst->label, 0, OFFS(ctx));
	}
	return src;
}



// Performs a bit shifting operation
gen_var_t *r3_shift(asm_ctx_t *ctx, bool left, gen_var_t *output, gen_var_t *a, int16_t amount) {
	// Correct direction.
	if (amount < 0) {
		amount = -amount;
		left  ^= 1;
	}
	uint8_t n_words = 2;
	// If it exceeds n_words, don't bother.
	if (amount > n_words * 8) {
		gen_var_t zero = {
			.type   = VAR_TYPE_CONST,
			.iconst = 0
		};
		gen_mov(ctx, output, &zero);
		return output;
	}
	
	// If there's no output hint, create one ourselves.
	if (!output) {
		output = (gen_var_t *) malloc(sizeof(gen_var_t));
		*output = (gen_var_t) {
			.label = r3_get_tmp(ctx),
			.type  = VAR_TYPE_LABEL
		};
	}
	gen_var_t *store = output;
	
	if (store->type == VAR_TYPE_COND) {
		// TODO: Use a temporary variable.
		store = (gen_var_t *) malloc(sizeof(gen_var_t));
		*store = (gen_var_t) {
			.label = r3_get_tmp(ctx),
			.type  = VAR_TYPE_LABEL
		};
		gen_mov(ctx, store, a);
	}
	
	// if (amount >= 8) {
	//     // A byte or more shifted.
	//     uint8_t bytes = amount >> 3;
	//     uint8_t bits  = amount &  7;
	//     uint8_t insn  = left ? OFFS_SHM + OFFS_SHM_L : OFFS_SHM + OFFS_SHM_R;
	// } else {
		// Less than a byte shifted.
		oper_t oper = left ? OP_SHIFT_L : OP_SHIFT_R;
		for (int16_t i = 0; i < amount; i++)
			r3_math1_l(ctx, oper, store, store);
		if (output->type == VAR_TYPE_COND) {
			output->cond = INSN_BNE;
		}
	// }
	return output;
}

// Performs a binary math operation for numbers of one byte.
gen_var_t *r3_math2(asm_ctx_t *ctx, oper_t oper, gen_var_t *out_hint, gen_var_t *a, gen_var_t *b, bool is_comp);

// Performs a binary math operation for numbers two bytes or larger.
gen_var_t *r3_math2_l(asm_ctx_t *ctx, oper_t oper, gen_var_t *output, gen_var_t *a, gen_var_t *b, bool is_comp) {
	// Number of words to operate on.
	uint8_t n_words  = 2;
	// Instruction for the occasion.
	uint8_t insn     = 0;
	// Branch instruction for comparisons.
	uint8_t b_insn   = 0;
	// Used to determine patterns used.
	bool is_b_const  = b->type == VAR_TYPE_CONST;
	// False for AND, OR, XOR and alike.
	bool does_cc     = 0;
	// If there's no output hint, create one ourselves.
	// Bitwise operations cannot use var_type_cond directly.
	if (!output || output->type == VAR_TYPE_COND && (oper >= OP_BIT_AND && oper <= OP_BIT_XOR)) {
		output = (gen_var_t *) malloc(sizeof(gen_var_t));
		*output = (gen_var_t) {
			.label = r3_get_tmp(ctx),
			.type  = VAR_TYPE_LABEL
		};
		is_comp = 0;
	}
#ifdef DEBUG_GENERATOR
	char *insn_name = "???";
#endif
	switch (oper) {
		case OP_ADD:
			insn = OFFS_ADD + (is_b_const ? OFFS_CALC_AV : OFFS_CALC_AM + PIE(ctx));
			b_insn = INSN_BNE + PIE(ctx);
			does_cc     = 1;
#ifdef DEBUG_GENERATOR
			insn_name = "ADD";
#endif
			break;
		case OP_SUB:
			insn = OFFS_SUB + (is_b_const ? OFFS_CALC_AV : OFFS_CALC_AM + PIE(ctx));
			b_insn = INSN_BNE + PIE(ctx);
			does_cc     = 1;
#ifdef DEBUG_GENERATOR
			insn_name = "SUB";
#endif
			break;
		case OP_EQ: b_insn = INSN_BEQ; goto cmp;
		case OP_NE: b_insn = INSN_BNE; goto cmp;
		case OP_LT: b_insn = INSN_BLT; goto cmp;
		case OP_GE: b_insn = INSN_BGE; goto cmp;
		case OP_GT: b_insn = INSN_BGT; goto cmp;
		case OP_LE: b_insn = INSN_BLE; goto cmp;
			cmp:
			is_comp = true;
			b_insn += PIE(ctx);
			insn = OFFS_CMP + (is_b_const ? OFFS_CALC_AV : OFFS_CALC_AM + PIE(ctx));
			does_cc     = 1;
#ifdef DEBUG_GENERATOR
			insn_name = "CMP";
#endif
			break;
		case OP_BIT_AND:
			insn = OFFS_AND + (is_b_const ? OFFS_BIT_AV  : OFFS_BIT_AM  + PIE(ctx));
#ifdef DEBUG_GENERATOR
			insn_name = "AND";
#endif
			break;
		case OP_BIT_OR:
			insn = OFFS_OR + (is_b_const ? OFFS_BIT_AV  : OFFS_BIT_AM  + PIE(ctx));
#ifdef DEBUG_GENERATOR
			insn_name = "OR";
#endif
			break;
		case OP_BIT_XOR:
			insn = OFFS_XOR + (is_b_const ? OFFS_BIT_AV  : OFFS_BIT_AM  + PIE(ctx));
#ifdef DEBUG_GENERATOR
			insn_name = "XOR";
#endif
			break;
	}
	uint8_t regno = REG_A;
	for (uint8_t i = 0; i < n_words; i++) {
		// Load part of the thing.
		r3_load_part(ctx, a, regno, i);
		// Add the instruction.
		if (is_b_const) {
			// Constant.
			DEBUG_GEN("  %s%s %s, 0x%02x\n", insn_name, (i && does_cc ? "C" : ""), reg_names[regno], (b->iconst >> (i * 8)) & 255);
			asm_write_memword(ctx, insn + (i && does_cc ? OFFS_CALC_CC : 0));
			asm_write_memword(ctx, b->iconst >> (i * 8));
		} else {
			// Label reference.
			DEBUG_GEN("  %s%s %s, [%s+%d]\n", insn_name, (i && does_cc ? "C" : ""), reg_names[regno], b->label, i);
			asm_write_memword(ctx, insn + (i && does_cc ? OFFS_CALC_CC : 0));
			asm_write_label_ref(ctx, b->label, i, OFFS(ctx));
		}
		if (!is_comp) {
			// Store the result.
			r3_store_part(ctx, output, regno, i);
		}
	}
	if (is_comp) {
		if (output->type == VAR_TYPE_COND) {
			// Condition.
			output->cond = b_insn;
		} else {
			// Have it converted.
			r3_branch_to_var(ctx, b_insn, output);
		}
	}
	return output;
}

// Performs a unary math operation for numbers of one byte.
gen_var_t *r3_math1(asm_ctx_t *ctx, oper_t oper, gen_var_t *out_hint, gen_var_t *a);

// Performs a unary math operation for numbers two bytes or larger.
gen_var_t *r3_math1_l(asm_ctx_t *ctx, oper_t oper, gen_var_t *output, gen_var_t *a) {
	bool use_mem     = gen_cmp(ctx, a, output) || oper == OP_SHIFT_L || oper == OP_SHIFT_R;
	// Number of words to operate on.
	uint8_t n_words  = 2;
	// Instruction for the occasion.
	uint8_t insn     = 0;
	uint8_t insn_cc  = 0;
	// If there's no output hint, create one ourselves.
	if (!output) {
		output = (gen_var_t *) malloc(sizeof(gen_var_t));
		*output = (gen_var_t) {
			.label = r3_get_tmp(ctx),
			.type  = VAR_TYPE_LABEL
		};
	}
	if (!gen_cmp(ctx, a, output)) {
		// If output != input, copy to output first.
		gen_mov(ctx, output, a);
	}
#ifdef DEBUG_GENERATOR
	char *insn_name = "???";
	char *insn_cc_name = "???";
#endif
	// Find the correct instruction.
	switch (oper) {
		case OP_ADD:
			insn      = use_mem ? INSN_INC_M  + PIE(ctx) : INSN_INC_A;
			insn_cc   = use_mem ? INSN_INCC_M + PIE(ctx) : INSN_INCC_A;
#ifdef DEBUG_GENERATOR
			insn_name    = "INC";
			insn_cc_name = "INCC";
#endif
			break;
		case OP_SUB:
			insn      = use_mem ? INSN_DEC_M  + PIE(ctx) : INSN_INC_A;
			insn_cc   = use_mem ? INSN_DECC_M + PIE(ctx) : INSN_INCC_A;
#ifdef DEBUG_GENERATOR
			insn_name    = "DEC";
			insn_cc_name = "DECC";
#endif
			break;
		case OP_SHIFT_L:
			insn      = OFFS_SHM + OFFS_SHM_L + PIE(ctx);
			insn_cc   = OFFS_SHM + OFFS_SHM_L + OFFS_SHM_CC + PIE(ctx);
#ifdef DEBUG_GENERATOR
			insn_name    = "SHL";
			insn_cc_name = "SHLC";
#endif
			break;
		case OP_SHIFT_R:
			insn      = OFFS_SHM + OFFS_SHM_R + PIE(ctx);
			insn_cc   = OFFS_SHM + OFFS_SHM_R + OFFS_SHM_CC + PIE(ctx);
#ifdef DEBUG_GENERATOR
			insn_name    = "SHR";
			insn_cc_name = "SHRC";
#endif
			break;
	}
	if (oper == OP_SHIFT_R) {
		// Iterate in reverse order because of the nature of shifting right.
		for (uint8_t i = n_words - 1; i != (uint8_t)-1; i--) {
			// Add the instruction.
			DEBUG_GEN("  %s [%s+%d]\n", i!=n_words-1 ? insn_cc_name : insn_name, output->label, i);
			asm_write_memword(ctx, i!=n_words-1 ? insn_cc : insn);
			asm_write_label_ref(ctx, output->label, i, OFFS(ctx));
		}
	} else if (use_mem) {
		// Perform on memory.
		for (uint8_t i = 0; i < n_words; i++) {
			// Add the instruction.
			DEBUG_GEN("  %s [%s+%d]\n", i ? insn_cc_name : insn_name, output->label, i);
			asm_write_memword(ctx, i ? insn_cc : insn);
			asm_write_label_ref(ctx, output->label, i, OFFS(ctx));
		}
	} else {
		uint8_t regno = REG_A;
		// Perform on registers.
		for (uint8_t i = 0; i < n_words; i++) {
			// Load part of the thing.
			r3_load_part(ctx, a, regno, i);
			// Label reference.
			DEBUG_GEN("  %s %s\n", i ? insn_cc_name : insn_name, reg_names[regno]);
			asm_write_memword(ctx, i ? insn_cc : insn);
			// Store part of the thing.
			r3_store_part(ctx, output, regno, i);
		}
	}
	return output;
}

/* ================== Functions ================== */

// Function entry for non-inlined functions. 
void gen_function_entry(asm_ctx_t *ctx, funcdef_t *funcdef) {
	if (funcdef->args.num == 1 /*&& funcdef->args.arr[0].size == 2*/) {
		// Exactly one long for an argument.
		// Define label.
		char *sect_id = ctx->current_section_id;
		asm_use_sect(ctx, ".bss", ASM_NOT_ALIGNED);
		char *label = malloc(strlen(funcdef->ident.strval) + 8);
		sprintf(label, "%s.LA0000", funcdef->ident.strval);
		asm_write_label(ctx, label);
		asm_write_zero (ctx, 2);
		gen_define_var (ctx, strdup(label), funcdef->args.arr[0].strval);
		r3_gen_var     (ctx, funcdef);
		
		// Go back to the original section the function was in.
		asm_use_sect(ctx, sect_id, ASM_NOT_ALIGNED);
		// Add the entry label.
		asm_write_label(ctx, funcdef->ident.strval);
		
		// Move over the value.
		DEBUG_GEN("  MOV [%s+0], X\n", label);
		asm_write_memword(ctx, OFFS_MOVST + OFFS_MOVM_XM + PIE(ctx));
		asm_write_label_ref(ctx, label, 0, OFFS(ctx));
		DEBUG_GEN("  MOV [%s+1], Y\n", label);
		asm_write_memword(ctx, OFFS_MOVST + OFFS_MOVM_YM + PIE(ctx));
		asm_write_label_ref(ctx, label, 1, OFFS(ctx));
		
		free(label);
	} else {
		// Arguments in memory.
		// Define labels.
		char *sect_id = ctx->current_section_id;
		asm_use_sect(ctx, ".bss", ASM_NOT_ALIGNED);
		char *label = malloc(strlen(funcdef->ident.strval) + 8);
		for (address_t i = 0; i < funcdef->args.num; i++) {
			sprintf(label, "%s.LA%04x", funcdef->ident.strval, i);
			asm_write_label(ctx, label);
			asm_write_zero (ctx, 2);
			gen_define_var (ctx, strdup(label), funcdef->args.arr[i].strval);
		}
		r3_gen_var(ctx, funcdef);
		
		// Go back to the original section the function was in.
		asm_use_sect(ctx, sect_id, ASM_NOT_ALIGNED);
		// Add the entry label.
		asm_write_label(ctx, funcdef->ident.strval);
		
		free(label);
	}
}

// Return statement for non-inlined functions.
// retval is null for void returns.
void gen_return(asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *retval) {
	if (retval) {
		// Gimme value.
		r3_movl_to_reg(ctx, retval);
	}
	// Append the return.
	if (/*func is interrupt handler?*/0) {
		DEBUG_GEN("  RTI\n");
		asm_write_memword(ctx, INSN_RTI);
	} else {
		DEBUG_GEN("  RET\n");
		asm_write_memword(ctx, INSN_RET);
	}
}

/* ================== Statements ================= */

// If statement implementation.
bool gen_if(asm_ctx_t *ctx, gen_var_t *cond, stmt_t *s_if, stmt_t *s_else) {
	if (s_else) {
		// Write the branch.
		char *l_true = asm_get_label(ctx);
		char *l_skip;
		r3_branch(ctx, cond, l_true, NULL);
		// True:
		bool if_explicit = gen_stmt(ctx, s_if, false);
		if (!if_explicit) {
			// Don't insert a dead jump.
			l_skip = asm_get_label(ctx);
			DEBUG_GEN("  JMP %s\n", l_skip);
			asm_write_memword(ctx, INSN_JMP);
			asm_write_label_ref(ctx, l_skip, 0, OFFS(ctx));
		}
		// False:
		asm_write_label(ctx, l_true);
		bool else_explicit = gen_stmt(ctx, s_else, false);
		// Skip label.
		if (!if_explicit) {
			// Don't add a useless label.
			asm_write_label(ctx, l_skip);
		}
		return if_explicit && else_explicit;
	} else {
		// Write the branch.
		char *l_skip = asm_get_label(ctx);
		r3_branch(ctx, cond, NULL, l_skip);
		// True:
		gen_stmt(ctx, s_if, false);
		// Skip label.
		asm_write_label(ctx, l_skip);
		return false;
	}
}

// While statement implementation.
void gen_while(asm_ctx_t *ctx, expr_t *cond, stmt_t *code, bool do_while) {
	do_while = false;
	char *expr_label;
	char *loop_label = asm_get_label(ctx);
	if (!do_while) {
		// Skip first expression checking in do...while loops.
		expr_label = asm_get_label(ctx);
		DEBUG_GEN("  JMP %s\n", expr_label);
		asm_write_memword(ctx, INSN_JMP + PIE(ctx));
		asm_write_label_ref(ctx, expr_label, 0, OFFS(ctx));
	}
	// Write code.
	asm_write_label(ctx, loop_label);
	bool explicit = gen_stmt(ctx, code, false);
	// Loop and condition check.
	if (!do_while) {
		asm_write_label(ctx, expr_label);
	}
	gen_var_t cond_hint = {
		.type = VAR_TYPE_COND
	};
	gen_var_t *cond_res = gen_expression(ctx, cond, &cond_hint);
	// Perform branch.
	r3_branch(ctx, cond_res, loop_label, NULL);
}

/* ================= Expressions ================= */

// Expression: Function call.
// args may be null for zero arguments.
gen_var_t *gen_expr_call(asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *callee, size_t n_args, gen_var_t **args) {
	
}

// Expression: Binary math operation.
gen_var_t *gen_expr_math2(asm_ctx_t *ctx, oper_t oper, gen_var_t *out_hint, gen_var_t *a, gen_var_t *b) {
	if (oper == OP_SHIFT_L || oper == OP_SHIFT_R) {
		if (b->type == VAR_TYPE_CONST) {
			return r3_shift(ctx, oper == OP_SHIFT_L, out_hint, a, b->iconst);
		}
		// TODO: Have this done by a function.
	} else if (oper == OP_MUL || oper == OP_DIV || oper == OP_MOD) {
		// TODO: Have this done by a function.
	} else if (b->type == VAR_TYPE_CONST && b->iconst == 1 && OP_IS_ADD(oper)) {
		// Adding or subtracting one can be simplified.
		return r3_math1_l(ctx, oper, out_hint, a);
	} else if (b->type == VAR_TYPE_CONST && b->iconst == (address_t) -1 && OP_IS_ADD(oper)) {
		// Adding or subtracting minus one can also be simplified.
		return r3_math1_l(ctx, oper == OP_ADD ? OP_SUB : OP_ADD, out_hint, a);
	} else {
		return r3_math2_l(ctx, oper, out_hint, a, b, out_hint && out_hint->type == VAR_TYPE_COND);
	}
}

// Expression: Unary math operation.
gen_var_t *gen_expr_math1(asm_ctx_t *ctx, oper_t oper, gen_var_t *output, gen_var_t *a) {
	if (oper == OP_LOGIC_NOT) {
		if (a->type == VAR_TYPE_COND) {
			// We can usually invert the branch condition.
			if (!output) output = a;
			else output->type = VAR_TYPE_COND;
			output->cond = INV_BR(a->cond);
			return output;
		} else {
			// Make that into a condition.
			if (!output) {
				output = (gen_var_t *) malloc(sizeof(gen_var_t));
				output->type = VAR_TYPE_COND;
			}
			output->cond = INV_BR(r3_var_to_branch(ctx, a));
			return output;
		}
	} else if (oper == OP_0_MINUS) {
		// This needs to be done this way.
		gen_var_t zero = {
			.type   = VAR_TYPE_CONST,
			.iconst = 0
		};
		return r3_math2_l(ctx, OP_SUB, output, &zero, a, output && output->type == VAR_TYPE_COND);
	} else if (oper == OP_DEREF) {
		if (output && output->type == VAR_TYPE_PTR) {
			// This is either &var or *ptr = val.
			output->ptr = a;
			return output;
		} else {
			// Do the thing.
			return r3_deref(ctx, output, a);
		}
	} else if (oper == OP_ADROF) {
		if (!output || output->type != VAR_TYPE_LABEL) {
			output = (gen_var_t *) malloc(sizeof(gen_var_t));
			*output = (gen_var_t) {
				.type = VAR_TYPE_LABEL,
				.label = r3_get_tmp(ctx)
			};
		}
		// Get le addr of a.
		if (a->type == VAR_TYPE_LABEL) {
			char *label = a->label;
			DEBUG_GEN("  GPTR [%s]\n", label);
			asm_write_memword(ctx, INSN_GPTR + PIE(ctx));
			asm_write_label_ref(ctx, label, 0, OFFS(ctx));
			if (output->type != VAR_TYPE_RETVAL) {
				r3_store_part(ctx, output, REG_X, 0);
				r3_store_part(ctx, output, REG_Y, 1);
			}
			return output;
		}
	} else {
		// Simpler math operations.
		return r3_math1_l(ctx, oper, output, a);
	}
}

// Variables: Move variable to another location.
void gen_mov(asm_ctx_t *ctx, gen_var_t *dst, gen_var_t *src) {
	if (gen_cmp(ctx, dst, src)) return;
	uint8_t regno = REG_A;
	uint8_t n_words = 2;
	if (dst->type == VAR_TYPE_PTR) {
		if (src->type == VAR_TYPE_COND) {
			// Squeeze this out first.
			gen_var_t tmp = {
				.type  = VAR_TYPE_LABEL,
				.label = r3_get_tmp(ctx)
			};
			r3_branch_to_var(ctx, src->cond, &tmp);
			src = COPY(&tmp, gen_var_t);
		}
		// Pointer time.
		r3_deref_set(ctx, (gen_var_t *) dst->ptr, src);
	} else if (src->type == VAR_TYPE_COND) {
		// Have it converted.
		r3_branch_to_var(ctx, src->cond, dst);
	} else {
		// Simple copy will do.
		for (uint8_t i = 0; i < n_words; i++) {
			r3_load_part (ctx, src, regno, i);
			r3_store_part(ctx, dst, regno, i);
		}
	}
}

// Generates .bss labels for variables and temporary variables in a function.
void r3_gen_var(asm_ctx_t *ctx, funcdef_t *func) {
	// TODO: A per-scope implementation.
	preproc_data_t *data = func->preproc;
	for (size_t i = 0; i < map_size(data->vars); i++) {
		char *label = (char *) data->vars->values[i];
		gen_define_var(ctx, label, data->vars->strings[i]);
		asm_write_label(ctx, label);
		asm_write_zero(ctx, 2);
	}
}

// Gets or adds a temp var.
char *r3_get_tmp(asm_ctx_t *ctx) {
	// Check existing.
	for (size_t i = 0; i < ctx->temp_num; i++) {
		if (!ctx->temp_usage[i]) {
			ctx->temp_usage[i] = 1;
			return ctx->temp_labels[i];
		}
	}
	
	// Make one more.
	char *func_label;
	char *label;
	make_one:
	func_label = ctx->current_func->ident.strval;
	label = malloc(strlen(func_label) + 8);
	sprintf(label, "%s.LT%04x", func_label, ctx->temp_num);
	DEBUG_GEN("// Add temp label %s\n", label);
	gen_define_temp(ctx, label);
	
	// Write the label in.
	char *sect = ctx->current_section_id;
	asm_use_sect(ctx, ".bss", ASM_NOT_ALIGNED);
	asm_write_label(ctx, label);
	asm_write_zero(ctx, 2);
	asm_use_sect(ctx, sect, ASM_NOT_ALIGNED);
	return label;
}

// Variables: Create a variable based on other value.
// Other value is null if not initialised.
void gen_var_dup(asm_ctx_t *ctx, funcdef_t *funcdef, ident_t *ident, gen_var_t *other) {
	// TODO: Account for other value.
}

// Variables: Create a label for the varialbe at preprocessing time.
char *gen_preproc_var(asm_ctx_t *ctx, preproc_data_t *parent, ident_t *ident) {
	char *fn_label = ctx->current_func->ident.strval;
	char *label = malloc(strlen(fn_label) + 8);
	sprintf(label, "%s.LV%04lx", fn_label, ctx->current_scope->local_num);
	return label;
}
