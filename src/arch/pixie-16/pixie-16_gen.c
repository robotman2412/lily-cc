
#include "pixie-16_gen.h"
#include "gen_util.h"
#include "definitions.h"
#include "asm.h"
#include "malloc.h"
#include "string.h"

#ifdef ENABLE_DEBUG_LOGS
char *c2_insn_names[] = {
	"ADD", "SUB", "CMP",  "AND", "OR",  "XOR", "???", "???",
};
char *c1_insn_names[] = {
	"INC", "DEC", "CMP1", "???", "???", "???", "SHL", "SHR",
};
char *addr_names[] = {
	"R0+", "R1+", "R2+", "R3+",
	"ST+", "",    "PC~", "",
};
char *b_insn_names[] = {
	".ULT", ".UGT", ".SLT", ".SGT", ".EQ", ".CS", "",     ".BRK",
	".UGE", ".ULE", ".SGE", ".SLE", ".NE", ".CC", ".JSR", ".RTI",
};
void PX_DESC_INSN(px_insn_t insn, char *imm0, char *imm1) {
	if (!imm0) imm0 = "???";
	if (!imm1) imm1 = "???";
	if (insn.a < 7) imm0 = reg_names[insn.a];
	if (insn.b < 7) imm1 = reg_names[insn.b];
	
	// Determine instruction name.
	bool  is_math1  = false;
	char *name;
	char *suffix;
	if (insn.o < 020) {
		// MATH2 instructions.
		name     = c2_insn_names[insn.o & 007];
		suffix   = (insn.o & 010) ? "C" : "";
	} else if (insn.o < 040) {
		// MATH1 instructions.
		name     = c1_insn_names[insn.o & 007];
		suffix   = (insn.o & 010) ? "C" : "";
		is_math1 = true;
	} else {
		// MOV and LEA.
		name     = (insn.o & 020) ? "LEA" : "MOV";
		suffix   = b_insn_names[insn.o & 017];
	}
	
	// Find the operand with addressing mode.
	char *tmp;
	char *git;
	if (insn.y) {
		tmp  = malloc(strlen(imm1) + 6);
		git  = imm1;
		imm1 = tmp;
	} else {
		tmp  = malloc(strlen(imm0) + 6);
		git  = imm0;
		imm0 = tmp;
	}
	
	// Insert addressing mode.
	if (insn.o == 5) {
		sprintf(tmp, "[%s]", git);
	} else if (insn.o == 7) {
		strcpy(tmp, git);
	} else {
		sprintf(tmp, "[%s%s]", addr_names[insn.x], git);
	}
	
	// Print the final thing.
	if (is_math1) {
		DEBUG_GEN("  %s%s %s\n", name, suffix, imm0);
	} else {
		DEBUG_GEN("  %s%s %s, %s\n", name, suffix, imm0, imm1);
	}
	free(tmp);
}
#else
#define PX_DESC_INSN(insn, a, b) do{}while(0)
#endif

/* ======== Gen-specific helper functions ======== */

// Function for packing an instruction.
 __attribute__((pure))
memword_t px_pack_insn(px_insn_t insn) {
	return (insn.y & 1) << 15
		 | (insn.x & 7) << 12
		 | (insn.b & 7) <<  9
		 | (insn.a & 7) <<  6
		 | (insn.o & 63);
}

// Function for unpacking an instruction.
 __attribute__((pure))
px_insn_t px_unpack_insn(memword_t packed) {
	return (px_insn_t) {
		.y = (packed & 0x8000) >> 15,
		.x = (packed & 0x7000) >> 12,
		.b = (packed & 0x0e00) >>  9,
		.a = (packed & 0x01c0) >>  6,
		.o = (packed & 0x003f),
	};
}

// Determine the calling conventions to use.
void px_update_cc(asm_ctx_t *ctx, funcdef_t *funcdef) {
	if (funcdef->args.num > 4) {
		funcdef->call_conv = PX_CC_STACK;
	} else if (funcdef->args.num) {
		funcdef->call_conv = PX_CC_REGS;
	} else {
		funcdef->call_conv = PX_CC_NONE;
	}
}

// Creates a branch condition from a variable.
cond_t px_var_to_cond(asm_ctx_t *ctx, gen_var_t *var) {
	if (var->type == VAR_TYPE_COND) {
		// Already done.
		return var->cond;
	} else {
		// Literally everything else can be done with CMP1 (UGE).
		// This allows absolutely every addressing mode.
		gen_expr_math1(ctx, OP_LT, var, var);
		return COND_UGE;
	}
}

// Generate a branch to one of two labels.
void px_branch(asm_ctx_t *ctx, gen_var_t *cond_var, char *l_true, char *l_false) {
	cond_t cond = px_var_to_cond(ctx, cond_var);
	
	if (DET_PIE(ctx)) {
		// PIE alternative.
		if (l_true) {
			DEBUG_GEN("  LEA%s PC, [PC~%s]\n", b_insn_names[cond&15], l_true);
			asm_write_memword(ctx, INSN_JMP_PIE);
			asm_write_label_ref(ctx, l_true, 0, ASM_LABEL_REF_OFFS_PTR);
		}
		if (l_false) {
			cond = INV_BR(cond);
			DEBUG_GEN("  LEA%s PC, [PC~%s]\n", b_insn_names[cond&15], l_false);
			asm_write_memword(ctx, INSN_JMP_PIE);
			asm_write_label_ref(ctx, l_false, 0, ASM_LABEL_REF_OFFS_PTR);
		}
	} else {
		// Non-PIE alternative
		if (l_true) {
			DEBUG_GEN("  MOV%s PC, %s\n", b_insn_names[cond&15], l_true);
			asm_write_memword(ctx, INSN_JMP);
			asm_write_label_ref(ctx, l_true, 0, ASM_LABEL_REF_ABS_PTR);
		}
		if (l_false) {
			cond = INV_BR(cond);
			DEBUG_GEN("  MOV%s PC, %s\n", b_insn_names[cond&15], l_false);
			asm_write_memword(ctx, INSN_JMP);
			asm_write_label_ref(ctx, l_false, 0, ASM_LABEL_REF_ABS_PTR);
		}
	}
}

// Generate a jump to a label.
void px_jump(asm_ctx_t *ctx, char *label) {
	if (DET_PIE(ctx)) {
		// PIE alternative.
		DEBUG_GEN("  LEA PC, [PC~%s]\n", label);
		asm_write_memword(ctx, INSN_JMP_PIE);
		asm_write_label_ref(ctx, label, 0, ASM_LABEL_REF_OFFS_PTR);
	} else {
		// Non-PIE alternative
		DEBUG_GEN("  MOV PC, %s\n", label);
		asm_write_memword(ctx, INSN_JMP);
		asm_write_label_ref(ctx, label, 0, ASM_LABEL_REF_ABS_PTR);
	}
}

// Pick an addressing mode for a label.
px_insn_t px_insn_label(asm_ctx_t *ctx, asm_label_t label, bool y, asm_label_ref_t *ref) {
	if (DET_PIE(ctx)) {
		// [PC~label]
		if (ref) *ref = ASM_LABEL_REF_OFFS_PTR;
		return (px_insn_t) {
			.y = y,
			.x = 6,
			.a = 7,
			.b = 7,
		};
	} else {
		// [label]
		if (ref) *ref = ASM_LABEL_REF_ABS_PTR;
		return (px_insn_t) {
			.y = y,
			.x = 5,
			.a = 7,
			.b = 7,
		};
	}
}

// Pick a register to use.
reg_t px_pick_reg(asm_ctx_t *ctx, bool do_vacate) {
	// Check for a free register.
	for (size_t i = 0; i < NUM_REGS; i++) {
		if (!ctx->current_scope->reg_usage[i]) {
			return i;
		}
	}
	// Otherwise, free up a pseudorandom one.
	// Eenie meenie minie moe...
	// TODO: Detect least used?
	return (((*ctx->current_func->ident.strval + ctx->stack_size) * 27483676) >> 21) % 4;
}

// Move a value to a register.
void px_mov_to_reg(asm_ctx_t *ctx, gen_var_t *val, reg_t dest) {
	if (val->type == VAR_TYPE_REG) {
		if (val->reg == dest) return;
		// Register to register move.
		DEBUG_GEN("  MOV %s, %s\n", reg_names[dest], reg_names[val->reg]);
		px_insn_t insn = {
			.y = 0,
			.x = ADDR_IMM,
			.a = dest,
			.b = val->reg,
			.o = PX_OP_MOV,
		};
		asm_write_memword(ctx, px_pack_insn(insn));
	} else if (val->type == VAR_TYPE_LABEL) {
		// Memory to register move.
		asm_label_ref_t ref;
		px_insn_t insn = px_insn_label(ctx, val->label, true, &ref);
		insn.a = dest;
		insn.o = PX_OP_MOV;
		PX_DESC_INSN(insn, NULL, val->label);
		asm_write_memword(ctx, px_pack_insn(insn));
		asm_write_label_ref(ctx, val->label, 0, ref);
	} else if (val->type == VAR_TYPE_CONST) {
		// Copy constant to register.
		DEBUG_GEN("  MOV %s, 0x%04x\n", reg_names[dest], val->iconst);
		px_insn_t insn = {
			.y = 0,
			.x = ADDR_IMM,
			.a = dest,
			.b = REG_IMM,
			.o = PX_OP_MOV,
		};
		asm_write_memword(ctx, px_pack_insn(insn));
		asm_write_memword(ctx, val->iconst);
	} else if (val->type == VAR_TYPE_COND) {
		// Condition to boolean.
		
		// Set to 0 by default.
		DEBUG_GEN("  MOV %s, 0\n", reg_names[dest]);
		px_insn_t insn = {
			.y = 0,
			.x = ADDR_IMM,
			.a = dest,
			.b = REG_IMM,
			.o = PX_OP_MOV,
		};
		asm_write_memword(ctx, px_pack_insn(insn));
		asm_write_memword(ctx, 0);
		// Then set to 1 if condition matches.
		DEBUG_GEN("  MOV%s %s, 1\n", b_insn_names[val->cond & 017], reg_names[dest]);
		insn.o = 040 | val->cond;
		asm_write_memword(ctx, px_pack_insn(insn));
		asm_write_memword(ctx, 1);
	} else if (val->type == VAR_TYPE_STACKOFFS) {
		// Stack offset to register.
		DEBUG_GEN("  MOS %s, [ST+%d]\n", reg_names[dest], ctx->stack_size - val->offset);
		px_insn_t insn = {
			.y = 1,
			.x = ADDR_ST,
			.a = dest,
			.b = REG_ST,
			.o = PX_OP_MOV,
		};
		asm_write_memword(ctx, px_pack_insn(insn));
		asm_write_memword(ctx, val->offset);
	}
}

/* ================== Functions ================== */

// Function entry for non-inlined functions. 
void gen_function_entry(asm_ctx_t *ctx, funcdef_t *funcdef) {
	// Update the calling conventions.
	px_update_cc(ctx, funcdef);
	
	if (funcdef->call_conv == PX_CC_REGS) {
		DEBUG_GEN("// calling convention: registers\n");
		// Define variables in registers.
		for (size_t i = 0; i < funcdef->args.num; i++) {
			gen_var_t *var = malloc(sizeof(gen_var_t));
			*var = (gen_var_t) {
				.type  = VAR_TYPE_REG,
				.reg   = i,
				.owner = funcdef->args.arr[i].strval
			};
			gen_define_var(ctx, var, funcdef->args.arr[i].strval);
		}
	} else if (funcdef->call_conv == PX_CC_STACK) {
		DEBUG_GEN("// calling convention: stack\n");
		// Define variables in stack, first parameter pushed last.
		// First parameter has least offset.
		for (size_t i = 0; i < funcdef->args.num; i++) {
			gen_var_t *var = malloc(sizeof(gen_var_t));
			*var = (gen_var_t) {
				.type   = VAR_TYPE_STACKOFFS,
				.offset = i,
				.owner  = funcdef->args.arr[i].strval
			};
			gen_define_var(ctx, var, funcdef->args.arr[i].strval);
		}
		// Update stack size.
		ctx->current_scope->stack_size = funcdef->args.num;
	} else {
		DEBUG_GEN("// calling convention: no parameters\n");
	}
}

// Return statement for non-inlined functions.
// retval is null for void returns.
void gen_return(asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *retval) {
	if (retval) {
		// Enforce retval is in R0.
		px_mov_to_reg(ctx, retval, REG_R0);
	}
	// Append the return.
	DEBUG_GEN("  MOV PC, [ST]\n");
	asm_write_memword(ctx, INSN_RET);
}

/* ================== Statements ================= */

// If statement implementation.
bool gen_if(asm_ctx_t *ctx, gen_var_t *cond, stmt_t *s_if, stmt_t *s_else) {
	if (0 /* Conditional mov optimisation? */) {
		// TODO: Perform check for this one.
	} else {
		// Traditional branch.
		if (s_else) {
			// Write the branch.
			char *l_true = asm_get_label(ctx);
			char *l_skip;
			px_branch(ctx, cond, l_true, NULL);
			// True:
			bool if_explicit = gen_stmt(ctx, s_if, false);
			if (!if_explicit) {
				// Don't insert a dead jump.
				l_skip = asm_get_label(ctx);
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
			px_branch(ctx, cond, NULL, l_skip);
			// True:
			gen_stmt(ctx, s_if, false);
			// Skip label.
			asm_write_label(ctx, l_skip);
			return false;
		}
	}
}

// While statement implementation.
void gen_while(asm_ctx_t *ctx, expr_t *cond, stmt_t *code, bool is_do_while) {
	char *loop_label  = asm_get_label(ctx);
	char *check_label = asm_get_label(ctx);
	
	// Initial check?
	if (!is_do_while) {
		// For "while (condition) {}" loops, check condition before entering loop.
		px_jump(ctx, check_label);
	}
	
	// Loop code.
	asm_write_label(ctx, loop_label);
	gen_stmt(ctx, code, false);
	
	// Condition check.
	gen_var_t cond_hint = {
		.type = VAR_TYPE_COND,
	};
	gen_var_t *cond_res = gen_expression(ctx, cond, &cond_hint);
	px_branch(ctx, cond_res, loop_label, NULL);
	if (cond_res != &cond_hint) {
		gen_unuse(ctx, cond_res);
	}
}

// Create a string for the variable to insert into the assembly. (only if inline assembly is supported)
// The string will be freed later and it is allowed to generate code in this method.
char *gen_iasm_var(asm_ctx_t *ctx, gen_var_t *var, iasm_reg_t *reg) {
	bool needs_change = false;
	if (var->type == VAR_TYPE_CONST && !reg->mode_known_const) {
		needs_change = true;
	} else if (var->type == VAR_TYPE_REG && !reg->mode_register) {
		needs_change = true;
	} else if (var->type == VAR_TYPE_LABEL && !reg->mode_memory) {
		needs_change = true;
	}
	
	if (!needs_change) goto conversion;
	if (reg->mode_register) {
		px_mov_to_reg(ctx, var, px_pick_reg(ctx, true));
	}
	
	conversion:
	// Convert the variable to assembly craps.
	if (var->type == VAR_TYPE_CONST) {
		// Constant.
		char *buf = malloc(7);
		sprintf(buf, "0x%04x", var->iconst);
		return buf;
	} else if (var->type == VAR_TYPE_REG) {
		// Register.
		return strdup(reg_names[var->reg]);
	} else if (var->type == VAR_TYPE_LABEL) {
		// Label.
		char *buf = malloc(strlen(var->label) + 3);
		sprintf(buf, "[%s]", var->label);
		return buf;
	} else if (var->type == VAR_TYPE_STACKOFFS) {
		// In stack.
		char *buf = malloc(12);
		sprintf(buf, "[ST+%u]", ctx->stack_size - var->offset);
		return buf;
	}
}

/* ================= Expressions ================= */

// Expression: Function call.
// args may be null for zero arguments.
gen_var_t *gen_expr_call(asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *callee, size_t n_args, gen_var_t **args) {
	// TODO.
	gen_var_t dummy = {
		.type   = VAR_TYPE_CONST,
		.iconst = 0
	};
	return COPY(&dummy, gen_var_t);
}

// Expression: Binary math operation.
gen_var_t *gen_expr_math2(asm_ctx_t *ctx, oper_t oper, gen_var_t *out_hint, gen_var_t *a, gen_var_t *b) {
	
}

// Expression: Unary math operation.
gen_var_t *gen_expr_math1(asm_ctx_t *ctx, oper_t oper, gen_var_t *output, gen_var_t *a) {
	
}

// Variables: Move variable to another location.
void gen_mov(asm_ctx_t *ctx, gen_var_t *dst, gen_var_t *src) {
	if (dst->type == VAR_TYPE_REG) {
		// To register move.
		px_mov_to_reg(ctx, src, dst->reg);
	} else if (dst->type == VAR_TYPE_COND) {
		// Convert to condition.
		dst->cond = px_var_to_cond(ctx, src);
	} else {
		// Move through register.
		reg_t regno;
		if (src->type == VAR_TYPE_CONST) {
			// We can encode this differently.
			regno = REG_IMM;
		} if (src->type == VAR_TYPE_REG) {
			// Use the existing register.
			regno = src->reg;
		} else {
			// Pick a new register.
			// TODO: Pick a register and/or vacate.
			regno = REG_R3;
			px_mov_to_reg(ctx, src, regno);
		}
		
		if (dst->type == VAR_TYPE_STACKOFFS) {
			// Place in stack by offset.
			DEBUG_GEN("  MOV [ST+%d], %s\n", dst->offset, reg_names[regno]);
			px_insn_t insn = {
				.y = 1,
				.x = ADDR_ST,
				.b = regno,
				.a = REG_ST,
				.o = PX_OP_MOV,
			};
			asm_write_memword(ctx, px_pack_insn(insn));
			asm_write_memword(ctx, ctx->stack_size - dst->offset);
		} else if (dst->type == VAR_TYPE_LABEL) {
			// Place in memory.
			asm_label_ref_t ref;
			px_insn_t insn = px_insn_label(ctx, dst->label, false, &ref);
			insn.b = regno;
			insn.o = PX_OP_MOV;
			#ifdef ENABLE_DEBUG_LOGS
			if (src->type == VAR_TYPE_CONST) {
				char tmp[7];
				sprintf(tmp, "0x%04x", src->iconst);
				PX_DESC_INSN(insn, dst->label, tmp);
			} else {
				PX_DESC_INSN(insn, dst->label, NULL);
			}
			#endif
			asm_write_memword(ctx, px_pack_insn(insn));
			asm_write_label_ref(ctx, dst->label, -(src->type == VAR_TYPE_CONST), ref);
		} else if (dst->type == VAR_TYPE_PTR) {
			// TODO: Store to pointer.
		}
		
		if (src->type == VAR_TYPE_CONST) {
			// Add the missing constant.
			asm_write_memword(ctx, src->iconst);
		}
	}
}

// Generates .bss labels for variables and temporary variables in a function.
void px_gen_var(asm_ctx_t *ctx, funcdef_t *func) {
	// TODO: A per-scope implementation.
	// preproc_data_t *data = func->preproc;
	// for (size_t i = 0; i < map_size(data->vars); i++) {
	// 	char *label = (char *) data->vars->values[i];
	// 	gen_define_var(ctx, label, data->vars->strings[i]);
	// 	asm_write_label(ctx, label);
	// 	asm_write_zero(ctx, 2);
	// }
}

// Gets or adds a temp var.
// Each temp label represents one byte, so some variables will use multiple.
char *px_get_tmp(asm_ctx_t *ctx, size_t size) {
	// Check existing.
	size_t remaining = size;
	for (size_t i = 0; i < ctx->temp_num; i++) {
		if (!ctx->temp_usage[i]) {
			remaining --;
		} else {
			remaining = size;
		}
		if (!remaining) {
			// Find the index.
			size_t index = i - size + 1;
			// Mark all labels overwritten as used.
			for (size_t x = index; x <= i; x++) {
				ctx->temp_usage[x] = true;
			}
			// Return the found labels.
			return ctx->temp_labels[index];
		}
	}
	
	// Make some more.
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

// Variables: Create a label for the variable at preprocessing time.
// Must allocate a new gen_var_t object.
gen_var_t *gen_preproc_var(asm_ctx_t *ctx, preproc_data_t *parent, ident_t *ident) {
	// Create a label.
	char *fn_label = ctx->current_func->ident.strval;
	char *label = malloc(strlen(fn_label) + 8);
	sprintf(label, "%s.LV%04lx", fn_label, ctx->current_scope->local_num);
	
	// Package it into a gen_var_t.
	gen_var_t loc = {
		.type  = VAR_TYPE_LABEL,
		.label = label,
		.owner = ident->strval
	};
	
	// And return a copy.
	return COPY(&loc, gen_var_t);
}
