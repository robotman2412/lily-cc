
#include "pixie-16_gen.h"
#include "gen_util.h"
#include "definitions.h"
#include "asm.h"
#include "malloc.h"
#include "string.h"
#include "signal.h"

#include "pixie-16_internal.h"

/* ======== Gen-specific helper functions ======== */

// Bump a register to the top of the usage list.
void px_touch_reg(asm_ctx_t *ctx, reg_t regno) {
	for (int i = 3; i > 1; i--) {
		if (ctx->reg_usage_order[i] == regno) {
			for (; i > 1; i--) {
				ctx->reg_usage_order[i] = ctx->reg_usage_order[i - 1];
			}
			ctx->reg_usage_order[0] = regno;
			return;
		}
	}
}

// Gets the least used register in the list.
__attribute__((__always_inline__))
reg_t px_least_used_reg(asm_ctx_t *ctx) {
	return ctx->reg_usage_order[3];
}

// Gets the constant required for a stack indexing memory access.
 __attribute__((pure))
static inline address_t px_get_depth(asm_ctx_t *ctx, gen_var_t *var, address_t var_offs) {
	return ctx->current_scope->stack_size - var->offset + var_offs - 1;
}

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

// Write an instruction with some context.
void px_write_insn0(asm_ctx_t *ctx, px_insn_t insn, asm_label_t label0, address_t offs0, asm_label_t label1, address_t offs1, bool is_iasm) {
	#ifdef ENABLE_DEBUG_LOGS
	// Debug log variables.
	char *imm0 = NULL;
	char *imm1 = NULL;
	#endif
	
	// Determine memory clobbers.
	if (!is_iasm && (insn.a == REG_ST || insn.b == REG_ST || insn.x == REG_ST)) {
		px_memclobber(ctx, true);
	}
	
	// Write the basic instruction.
	asm_write_memword(ctx, px_pack_insn(insn));
	
	// Determine the count of IMM parameters.
	uint_fast8_t num_param = (insn.a == REG_IMM) + (insn.b == REG_IMM);
	
	if (insn.a == REG_IMM) {
		if (label0) {
			if (!insn.y && insn.x == ADDR_PC) {
				// Determine offset relative to instruction end instead of label reference.
				address_t offs = (insn.b == REG_IMM) ? offs0 - 2 : offs0 - 1;
				// Write label reference (PIE).
				asm_write_label_ref(
					ctx, label0, offs,
					ASM_LABEL_REF_OFFS_PTR
				);
			} else {
				// Write label reference (Non-PIE).
				asm_write_label_ref(
					ctx, label0, offs0,
					ASM_LABEL_REF_ABS_PTR
				);
			}
			
			#ifdef ENABLE_DEBUG_LOGS
			imm0 = xalloc(ctx->allocator, strlen(label0) + 10);
			if (offs0) {
				// Label and constant to hexadecimal.
				sprintf(imm0, "%s+0x%04x", label0, offs0);
			} else {
				// Copy label to description.
				strcpy(imm0, label0);
			}
			#endif
			
		} else {
			// Write constant.
			asm_write_memword(ctx, offs0);
			
			#ifdef ENABLE_DEBUG_LOGS
			// Constant to hexadecimal.
			imm0 = xalloc(ctx->allocator, 10);
			sprintf(imm0, "0x%04x", offs0);
			#endif
		}
	}
	if (insn.b == REG_IMM) {
		if (label1) {
			if (insn.y && insn.x == ADDR_PC) {
				// Write label reference (PIE).
				asm_write_label_ref(
					ctx, label1, offs1 - 1,
					ASM_LABEL_REF_OFFS_PTR
				);
			} else {
				// Write label reference (Non-PIE).
				asm_write_label_ref(
					ctx, label1, offs1,
					ASM_LABEL_REF_ABS_PTR
				);
			}
			
			#ifdef ENABLE_DEBUG_LOGS
			imm1 = xalloc(ctx->allocator, strlen(label1) + 10);
			if (offs1) {
				// Label and constant to hexadecimal.
				sprintf(imm1, "%s+0x%04x", label1, offs1);
			} else {
				// Copy label to description.
				strcpy(imm1, label1);
			}
			#endif
			
		} else {
			// Write constant.
			asm_write_memword(ctx, offs1);
			
			#ifdef ENABLE_DEBUG_LOGS
			// Constant to hexadecimal.
			imm1 = xalloc(ctx->allocator, 10);
			sprintf(imm1, "0x%04x", offs1);
			#endif
		}
	}
	
	if (insn.a < 4) {
		px_touch_reg(ctx, insn.a);
	}
	if (insn.b < 4) {
		px_touch_reg(ctx, insn.b);
	}
	if (insn.x < 4) {
		px_touch_reg(ctx, insn.x);
	}
	
	#ifdef ENABLE_DEBUG_LOGS
	// Describe instruction.
	PX_DESC_INSN(insn, imm0, imm1);
	// Clean up.
	if (imm0) xfree(ctx->allocator, imm0);
	if (imm1) xfree(ctx->allocator, imm1);
	#endif
}

// Write an instruction with some context.
void px_write_insn_iasm(asm_ctx_t *ctx, px_insn_t insn, asm_label_t label0, address_t offs0, asm_label_t label1, address_t offs1) {
	px_write_insn0(ctx, insn, label0, offs0, label1, offs1, true);
}

// Write an instruction with some context.
void px_write_insn(asm_ctx_t *ctx, px_insn_t insn, asm_label_t label0, address_t offs0, asm_label_t label1, address_t offs1) {
	px_write_insn0(ctx, insn, label0, offs0, label1, offs1, false);
}

// Grab an addressing mode for a parameter.
reg_t px_addr_var(asm_ctx_t *ctx, gen_var_t *var, address_t part, reg_t *addrmode, asm_label_t *label, address_t *offs, reg_t dest) {
	static reg_t addrmode_dummy;
	if (!addrmode) {
		if (var->type != VAR_TYPE_CONST && var->type != VAR_TYPE_REG) {
			// We must move this to register.
			px_part_to_reg(ctx, var, dest, part);
			return dest;
		}
		
		addrmode = &addrmode_dummy;
	}
	
	switch (var->type) {
			gen_var_t *a, *b;
			
		case VAR_TYPE_CONST:
			// Constant thingy.
			*addrmode = ADDR_IMM;
			*offs = var->iconst >> (MEM_BITS * part);
			return REG_IMM;
			
		case VAR_TYPE_LABEL:
			// At label.
			*addrmode = ADDR_MEM;
			*label = var->label;
			*offs = part;
			return REG_IMM;
			
		case VAR_TYPE_STACKOFFS:
			// In stack.
			*addrmode = ADDR_ST;
			*offs = px_get_depth(ctx, var, part);
			return REG_IMM;
			
		case VAR_TYPE_REG:
			// In register.
			*addrmode = ADDR_IMM;
			return var->reg + part;
			
		case VAR_TYPE_RETVAL:
			// TODO
			break;
			
		case VAR_TYPE_PTR:
			if (var->ptr->type == VAR_TYPE_REG) {
				// This is a bit simpler.
				*addrmode = ADDR_MEM;
				return var->ptr->reg;
			} else if (var->ptr->type == VAR_TYPE_CONST) {
				// A constant point might as well be a normal memory access.
				*addrmode = ADDR_MEM;
				*offs = var->ptr->iconst;
				return REG_IMM;
			} else {
				char *imm1 = NULL;
				// Im poimtre.
				px_insn_t insn = {
					.y = 1,
					.a = dest,
					.o = PX_OP_MOV,
				};
				asm_label_t label1 = NULL;
				address_t   offs1  = 0;
				// Do some recursive funnies.
				insn.b = px_addr_var(ctx, var->ptr, 0, &insn.x, &label1, &offs1, dest);
				px_write_insn(ctx, insn, NULL, 0, label1, offs1);
				*addrmode = ADDR_MEM;
				// Return the registrex.
				return dest;
			}
			break;
		
		case VAR_TYPE_INDEXED:
			// Array indexing.
			a = var->indexed.location;
			b = var->indexed.index;
			
			// Get components into registers if not already.
			px_var_to_reg(ctx, a, true);
			px_var_to_reg(ctx, b, true);
			
			// Check for constants.
			if (a->type == VAR_TYPE_CONST) {
				if (b->type == VAR_TYPE_CONST) {
					// TODO: Those are both constants.
				} else {
					// Swapperoni.
					gen_var_t *tmp = a;
					a = b;
					b = tmp;
				}
			}
			
			if (b->type == VAR_TYPE_CONST) {
				// Constant offset and variable offset.
				*addrmode = a->reg;
				*offs     = b->iconst + part;
				return REG_IMM;
			} else if (part == 0) {
				// Two variable offsets, index part 0.
				*addrmode = a->reg;
				return b->reg;
			} else {
				// Two variable offsets, index exceeding 0.
				if (var->indexed.combined == NULL) {
					// Combine the two.
					reg_t dest = px_pick_reg(ctx, true);
					px_insn_t insn = {
						.y = 1,
						.x = a->reg,
						.b = b->reg,
						.a = dest,
						.o = PX_OP_LEA,
					};
					px_write_insn(ctx, insn, NULL, 0, NULL, 0);
					
					// Make a fancy variable for next time.
					var->indexed.combined = xalloc(ctx->current_scope->allocator, sizeof(gen_var_t));
					*var->indexed.combined = (gen_var_t) {
						.type        = VAR_TYPE_REG,
						.reg         = dest,
						.ctype       = ctype_ptr_simple(ctx, STYPE_CHAR), // TODO: Determine the correqt one.
						.owner       = NULL,
						.default_loc = NULL,
					};
					
					ctx->current_scope->reg_usage[dest] = var->indexed.combined;
				}
				
				// Construct with the offset.
				*addrmode = var->indexed.combined->reg;
				*offs     = part;
				return REG_IMM;
			}
			break;
	}
	return 0;
}

// Determine the calling conventions to use.
void px_update_cc(asm_ctx_t *ctx, funcdef_t *funcdef) {
	address_t arg_size = 0;
	for (size_t i = 0; i < funcdef->args.num; i++) {
		arg_size += funcdef->args.arr[i].type->size;
	}
	
	if (arg_size > NUM_REGS) {
		funcdef->call_conv = PX_CC_STACK;
	} else if (arg_size) {
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
		gen_var_t cond_hint = {
			.type = VAR_TYPE_COND,
			.cond = COND_UGE
		};
		gen_expr_math1(ctx, OP_GE, &cond_hint, var);
		return COND_UGE;
	}
}

// Generate a branch to one of two labels.
void px_branch(asm_ctx_t *ctx, gen_var_t *cond_var, char *l_true, char *l_false) {
	cond_t cond = px_var_to_cond(ctx, cond_var);
	
	if (DET_PIE(ctx)) {
		// PIE alternative.
		if (l_true) {
			px_insn_t insn = {
				.y = true,
				.x = ADDR_PC,
				.b = REG_IMM,
				.a = REG_PC,
				.o = PX_OFFS_LEA | cond,
			};
			px_write_insn(ctx, insn, NULL, 0, l_true, 0);
		}
		if (l_false) {
			px_insn_t insn = {
				.y = true,
				.x = ADDR_PC,
				.b = REG_IMM,
				.a = REG_PC,
				.o = PX_OFFS_LEA | INV_BR(cond),
			};
			px_write_insn(ctx, insn, NULL, 0, l_false, 0);
		}
	} else {
		// Non-PIE alternative
		if (l_true) {
			px_insn_t insn = {
				.y = true,
				.x = ADDR_MEM,
				.b = REG_IMM,
				.a = REG_PC,
				.o = PX_OFFS_MOV | cond,
			};
			px_write_insn(ctx, insn, NULL, 0, l_true, 0);
		}
		if (l_false) {
			px_insn_t insn = {
				.y = true,
				.x = ADDR_MEM,
				.b = REG_IMM,
				.a = REG_PC,
				.o = PX_OFFS_MOV | INV_BR(cond),
			};
			px_write_insn(ctx, insn, NULL, 0, l_false, 0);
		}
	}
}

// Generate a jump to a label.
void px_jump(asm_ctx_t *ctx, char *label) {
	if (DET_PIE(ctx)) {
		// PIE alternative.
		px_insn_t insn = {
			.y = true,
			.x = ADDR_PC,
			.b = REG_IMM,
			.a = REG_PC,
			.o = PX_OP_LEA,
		};
		px_write_insn(ctx, insn, NULL, 0, label, 0);
	} else {
		// Non-PIE alternative
		px_insn_t insn = {
			.y = true,
			.x = ADDR_MEM,
			.b = REG_IMM,
			.a = REG_PC,
			.o = PX_OP_MOV,
		};
		px_write_insn(ctx, insn, NULL, 0, label, 0);
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
	
	// Otherwise, free up the least used one.
	reg_t pick = px_least_used_reg(ctx);
	
	if (do_vacate) {
		gen_var_t *var = ctx->current_scope->reg_usage[pick];
		if (var->default_loc) {
			// Move it to it's default location.
			gen_mov(ctx, var->default_loc, var);
		} else {
			// Give it a temp location.
			gen_var_t *tmp = px_get_tmp(ctx, 1, true);
			tmp->ctype = var->ctype;
			gen_mov(ctx, tmp, var);
			*var = *tmp;
		}
	}
	px_touch_reg(ctx, pick);
	
	return pick;
}

// Move part of a value to a register.
void px_part_to_reg(asm_ctx_t *ctx, gen_var_t *var, reg_t dest, address_t index) {
	if (var->type == VAR_TYPE_CONST) {
		word_t iconst = var->iconst >> (MEM_BITS * index);
		if (!iconst) {
			// XOR dest, dest optimisation.
			px_insn_t insn = {
				.y = 0,
				.x = ADDR_IMM,
				.b = dest,
				.a = dest,
				.o = PX_OP_XOR,
			};
			px_write_insn(ctx, insn, NULL, 0, NULL, 0);
			return;
		}
	}
	
	#ifdef ENABLE_DEBUG_LOGS
	char *imm1 = NULL;
	#endif
	// Yep.
	px_insn_t insn = {
		.y = 1,
		.a = dest,
		.o = PX_OP_MOV,
	};
	asm_label_t label1 = NULL;
	address_t   offs1  = 0;
	// Request the value.
	insn.b = px_addr_var(ctx, var, index, &insn.x, &label1, &offs1, dest);
	px_write_insn(ctx, insn, NULL, 0, label1, offs1);
}

// Move a value to a register.
void px_mov_to_reg(asm_ctx_t *ctx, gen_var_t *val, reg_t dest) {
	if (val->type == VAR_TYPE_REG && val->reg == dest) return;
	address_t n_words = val->ctype->size;
	for (address_t i = 0; i < n_words; i++) {
		px_part_to_reg(ctx, val, dest + i, i);
	}
}

// Creates MATH1 instructions.
gen_var_t *px_math1(asm_ctx_t *ctx, memword_t opcode, gen_var_t *out_hint, gen_var_t *a) {
	gen_var_t *output = out_hint;
	address_t n_words = a->ctype->size;
	bool      do_copy = !gen_cmp(ctx, output, a) && opcode != PX_OP_CMP1;
	if (!output || do_copy) {
		output = px_get_tmp(ctx, n_words, true);
		output->ctype = a->ctype;
	}
	if (do_copy) {
		// Perform the copy.
		gen_mov(ctx, output, a);
		a = output;
	}
	
	// Special case for SHR.
	address_t i = 0, limit = n_words, delta = 1;
	if (opcode == PX_OP_SHR) {
		limit = -1;
		i     = n_words - 1;
		delta = -1;
	}
	
	// Pointer conversion check.
	if (a->type == VAR_TYPE_PTR && a->ptr->type != VAR_TYPE_CONST && a->ptr->type != VAR_TYPE_REG) {
		// Make the pointer into a register.
		reg_t regno  = px_pick_reg(ctx, true);
		px_mov_to_reg(ctx, a->ptr, regno);
		ctx->current_scope->reg_usage[regno] = a;
		a->ptr->type = VAR_TYPE_REG;
		a->ptr->reg  = regno;
	}
	
	// A fancy for loop.
	for (; i != limit; i += delta) {
		
		px_insn_t insn = {
			.y = 0,
			.b = 0,
			.o = opcode,
		};
		asm_label_t label0 = NULL;
		address_t   offs0  = 0;
		asm_label_t label1 = NULL;
		address_t   offs1  = 0;
		// Collect addressing modes.
		insn.a = px_addr_var(ctx, a, i, &insn.x, &label0, &offs0, 0);
		// Write the resulting instruction.
		px_write_insn(ctx, insn, label0, offs0, label1, offs1);
		
		// Make the rest the carry continue alternative.
		opcode |= PX_OFFS_CC;
	}
	
	if (do_copy) {
		output->ctype = a->ctype;
	}
	
	return output;
}

// Creates MATH2 instructions.
gen_var_t *px_math2(asm_ctx_t *ctx, memword_t opcode, gen_var_t *out_hint, gen_var_t *a, gen_var_t *b) {
	// Determine whether size matching is required.
	if (a->ctype->size != b->ctype->size && (STYPE_IS_SIGNED(a->ctype->simple_type) || STYPE_IS_SIGNED(b->ctype->simple_type))) {
		// Size matching is required.
		if (a->ctype->size > b->ctype->size) {
			b = gen_cast(ctx, b, a->ctype);
		} else {
			a = gen_cast(ctx, a, b->ctype);
		}
	}
	
	// Determine the amount of bytes to operate on.
	bool      a_big   = a->ctype->size > b->ctype->size;
	address_t n_words = a_big ? b->ctype->size : a->ctype->size;
	address_t n_ext   = (a_big ? a->ctype->size : b->ctype->size) - n_words;
	
	gen_var_t *output = out_hint;
	bool      do_copy = !gen_cmp(ctx, output, a) && opcode != PX_OP_CMP;
	if (!output || do_copy) {
		output = px_get_tmp(ctx, n_words, true);
		output->ctype = a->ctype;
	}
	if (do_copy) {
		// Perform the copy.
		gen_mov(ctx, output, a);
		a = output;
	}
	
	// Whether a temp register is required.
	bool conv_b = a->type != VAR_TYPE_REG && b->type != VAR_TYPE_REG && b->type != VAR_TYPE_CONST;
	bool y      = b->type != VAR_TYPE_REG;
	reg_t reg_b = y ? 0 : b->reg;
	if (conv_b) {
		reg_b = px_pick_reg(ctx, true);
	} else if (b->type == VAR_TYPE_CONST) {
		reg_b = REG_IMM;
	}
	
	// A fancy for loop.
	for (address_t i = 0; i < n_words; i++) {
		// Copy B to a register if required.
		bool do_inc = false;
		if (b->type == VAR_TYPE_CONST) {
			// Check for INC, DEC and CMP1 optimisations.
			word_t val_part = b->iconst >> (MEM_BITS * i);
			do_inc = (i == 0 && val_part == 1) || (i && val_part == 0);
			do_inc &= (opcode & !PX_OFFS_CC) <= PX_OP_CMP;
		}
		
		px_insn_t insn = {
			.y = 0,
			.o = do_inc ? opcode | PX_OP_INC : opcode,
		};
		asm_label_t label0 = NULL;
		address_t   offs0  = 0;
		asm_label_t label1 = NULL;
		address_t   offs1  = 0;
		// Collect addressing modes.
		insn.a = px_addr_var(ctx, a, i, &insn.x, &label0, &offs0, 0);
		if (do_inc) {
			// INC optimisation.
			insn.b = 0;
		} if (conv_b) {
			// Move B to a temp register.
			px_part_to_reg(ctx, b, reg_b, i);
			insn.b = reg_b;
		} else if (insn.x == ADDR_IMM) {
			// Address using B.
			insn.y = 1;
			insn.b = px_addr_var(ctx, b, i, &insn.x, &label1, &offs1, reg_b);
		} else {
			// Use B as a reg or const.
			insn.b = px_addr_var(ctx, b, i, NULL, &label1, &offs1, reg_b);
		}
		// Write the resulting instruction.
		px_write_insn(ctx, insn, label0, offs0, label1, offs1);
		
		// Make the rest carry continue.
		opcode |= PX_OFFS_CC;
	}
	
	if (out_hint && out_hint->type == VAR_TYPE_COND) {
		// Yeah we can do condition hints.
		out_hint->cond = COND_NE;
		out_hint->ctype = ctype_simple(ctx, STYPE_BOOL);
		return out_hint;
	} else {
		if (do_copy) {
			output->ctype = a->ctype;
		}
		// Normal ass computation.
		return output;
	}
}

/* ================== Functions ================== */

// Function entry for non-inlined functions. 
void gen_function_entry(asm_ctx_t *ctx, funcdef_t *funcdef) {
	// Update the calling conventions.
	px_update_cc(ctx, funcdef);
	// Write the label that everyone will refer to.
	asm_write_label(ctx, funcdef->ident.strval);
	// Initialise the real stack size to 0.
	ctx->current_scope->real_stack_size = 0;
	// Preset the register usages.
	for (int i = 0; i < 4; i++) {
		ctx->reg_usage_order[i] = i;
	}
	
	if (funcdef->call_conv == PX_CC_REGS) {
		// Registers.
		DEBUG_GEN("// calling convention: registers\n");
		
		// Define variables in registers.
		address_t arg_size = 0;
		for (size_t i = 0; i < funcdef->args.num; i++) {
			gen_var_t *var = xalloc(ctx->allocator, sizeof(gen_var_t));
			gen_var_t *loc = xalloc(ctx->allocator, sizeof(gen_var_t));
			
			// Variable in register.
			*var = (gen_var_t) {
				.type  = VAR_TYPE_REG,
				.reg   = i,
				.owner = funcdef->args.arr[i].strval,
				.ctype = funcdef->args.arr[i].type,
			};
			// It's default location is in the stack.
			*loc = (gen_var_t) {
				.type        = VAR_TYPE_STACKOFFS,
				.offset      = i,
				.owner       = funcdef->args.arr[i].strval,
				.ctype       = funcdef->args.arr[i].type,
				.default_loc = NULL
			};
			var->default_loc = loc;
			
			// Mark the register as used.
			ctx->current_scope->reg_usage[i] = var;
			
			gen_define_var(ctx, var, funcdef->args.arr[i].strval);
			arg_size += var->ctype->size;
		}
		// Update stack size.
		ctx->current_scope->stack_size += arg_size;
	} else if (funcdef->call_conv == PX_CC_STACK) {
		// Stack.
		DEBUG_GEN("// calling convention: stack\n");
		
		// Define variables in stack, first parameter pushed last.
		// First parameter has least offset.
		address_t arg_size = 0;
		for (size_t i = 0; i < funcdef->args.num; i++) {
			gen_var_t *var = xalloc(ctx->allocator, sizeof(gen_var_t));
			
			// Variable in stack.
			*var = (gen_var_t) {
				.type        = VAR_TYPE_STACKOFFS,
				.offset      = i,
				.owner       = funcdef->args.arr[i].strval,
				.ctype       = funcdef->args.arr[i].type,
				.default_loc = NULL
			};
			// Which is also it's default location.
			var->default_loc = XCOPY(ctx->allocator, var, gen_var_t);
			
			gen_define_var(ctx, var, funcdef->args.arr[i].strval);
			arg_size += var->ctype->size;
		}
		// Update stack size.
		ctx->current_scope->stack_size += arg_size;
		ctx->current_scope->real_stack_size = arg_size;
	} else {
		// None.
		DEBUG_GEN("// calling convention: no parameters\n");
	}
}

// Return statement for non-inlined functions.
// retval is null for void returns.
void gen_return(asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *retval) {
	px_memclobber(ctx, false);
	if (retval) {
		// Enforce retval is in R0.
		if (retval->ctype->size != funcdef->returns->size) {
			retval = gen_cast(ctx, retval, funcdef->returns);
		}
		px_mov_to_reg(ctx, retval, REG_R0);
	}
	
	// Pop some stuff off the stack.
	address_t sub = ctx->current_scope->stack_size;
	ctx->current_scope->stack_size = 0;
	gen_stack_clear(ctx, sub);
	px_memclobber(ctx, true);
	
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
			// Skip label (after if code runs, to jump over else code).
			if (!if_explicit) {
				// Only add this label when the if code does not explicitly return.
				asm_write_label(ctx, l_skip);
			}
			return if_explicit && else_explicit;
		} else {
			// Write the branch.
			char *l_skip = asm_get_label(ctx);
			px_branch(ctx, cond, NULL, l_skip);
			// True:
			gen_stmt(ctx, s_if, false);
			// Skip label (to skip over if code when codition is not met).
			asm_write_label(ctx, l_skip);
			return false;
		}
	}
}

// While statement implementation.
void gen_while(asm_ctx_t *ctx, expr_t *cond, stmt_t *code, bool is_do_while) {
	char *loop_label  = asm_get_label(ctx);
	char *check_label = asm_get_label(ctx);
	
	bool is_forever = cond->type == EXPR_TYPE_CONST && cond->iconst;
	
	// Initial check?
	if (!is_do_while && !is_forever) {
		// For "while (condition) {}" loops, check condition before entering loop.
		px_jump(ctx, check_label);
	}
	
	// Loop code.
	asm_write_label(ctx, loop_label);
	gen_stmt(ctx, code, false);
	
	if (is_forever) {
		// No check because it loops forever.
		px_jump(ctx, loop_label);
	} else {
		// Condition check.
		gen_var_t cond_hint = {
			.type = VAR_TYPE_COND,
		};
		asm_write_label(ctx, check_label);
		gen_var_t *cond_res = gen_expression(ctx, cond, &cond_hint);
		px_branch(ctx, cond_res, loop_label, NULL);
		if (cond_res != &cond_hint) {
			gen_unuse(ctx, cond_res);
		}
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
	} else if (var->type == VAR_TYPE_STACKOFFS && !reg->mode_memory) {
		needs_change = true;
	} else if (var->type == VAR_TYPE_PTR) {
		needs_change = true;
	}
	
	if (!needs_change) goto conversion;
	if (reg->mode_register) {
		reg_t regno = px_pick_reg(ctx, true);
		px_mov_to_reg(ctx, var, regno);
		gen_var_t *default_loc = XCOPY(ctx->allocator, var, gen_var_t);
		var->type        = VAR_TYPE_REG;
		var->reg         = regno;
		var->default_loc = default_loc;
		ctx->current_scope->reg_usage[regno] = var;
	}
	
	conversion:
	// Convert the variable to assembly craps.
	if (var->type == VAR_TYPE_CONST) {
		// Constant.
		char *buf = xalloc(ctx->current_scope->allocator, 7);
		sprintf(buf, "0x%04x", (uint16_t) var->iconst);
		return buf;
	} else if (var->type == VAR_TYPE_REG) {
		// Register.
		return xstrdup(ctx->current_scope->allocator, reg_names[var->reg]);
	} else if (var->type == VAR_TYPE_LABEL) {
		// Label.
		char *buf = xalloc(ctx->current_scope->allocator, strlen(var->label) + 3);
		sprintf(buf, "[%s]", var->label);
		return buf;
	} else if (var->type == VAR_TYPE_STACKOFFS) {
		// In stack.
		char *buf = xalloc(ctx->current_scope->allocator, 12);
		sprintf(buf, "[ST+%u]", ctx->current_scope->stack_size - var->offset);
		return buf;
	} else {
		raise(SIGSEGV);
	}
}

/* ================= Expressions ================= */

// Expression: Function call.
// args may be null for zero arguments.
gen_var_t *gen_expr_call(asm_ctx_t *ctx, funcdef_t *funcdef, expr_t *callee, size_t n_args, expr_t *args) {
	// Find the calling convention.
	px_update_cc(ctx, funcdef);
	
	DEBUG_GEN("// Starting function call to %s\n", funcdef->ident.strval);
	
	// Generate callee address first so it won't interfere with parameters.
	gen_var_t *var = gen_expression(ctx, callee, NULL);
	
	if (funcdef->call_conv == PX_CC_REGS) {
		// Output hints for locations, if any.
		gen_var_t *hints[NUM_REGS];
		// Actual locations of parameters.
		gen_var_t *locations[NUM_REGS];
		
		// Generate parameter values.
		reg_t regindex = 0;
		for (size_t i = 0; i < n_args; i++) {
			// Make an output hint for the appropriate register.
			gen_var_t *out_hint = xalloc(ctx->allocator, sizeof(gen_var_t));
			*out_hint = (gen_var_t) {
				.type  = VAR_TYPE_REG,
				.reg   = regindex,
				.ctype = funcdef->args.arr[i].type,
			};
			hints[i] = out_hint;
			regindex += funcdef->args.arr[i].type->size;
			
			// Generate the expression.
			gen_var_t *res = gen_expression(ctx, &args[i], out_hint);
			if (!res) {
				// Abort.
				return NULL;
			}
			// Save the result for later so that it can be moved to the right register.
			locations[i] = res;
		}
		
		// Move parameters to registers.
		for (size_t i = 0; i < n_args; i++) {
			gen_mov(ctx, hints[i], locations[i]);
		}
		
	} else if (funcdef->call_conv == PX_CC_STACK) {
		// TODO.
	}
	
	DEBUG_GEN("// Jumping to subroutine %s\n", funcdef->ident.strval);
	
	// Jump to the subroutine.
	switch (var->type) {
		
		case (VAR_TYPE_CONST): {
			// Jump to constant.
			px_insn_t insn = {
				.y = true,
				.x = ADDR_IMM,
				.b = REG_IMM,
				.a = REG_PC,
				.o = PX_OFFS_MOV | COND_JSR,
			};
			px_write_insn(ctx, insn, NULL, 0, NULL, var->iconst);
		} break;
		case (VAR_TYPE_LABEL): {
			if (DET_PIE(ctx)) {
				// Jump to label (PIE).
				px_insn_t insn = {
					.y = true,
					.x = ADDR_PC,
					.b = REG_IMM,
					.a = REG_PC,
					.o = PX_OFFS_LEA | COND_JSR,
				};
				px_write_insn(ctx, insn, NULL, 0, var->label, 0);
			} else {
				// Jump to label (Non-PIE).
				px_insn_t insn = {
					.y = true,
					.x = ADDR_IMM,
					.b = REG_IMM,
					.a = REG_PC,
					.o = PX_OFFS_MOV | COND_JSR,
				};
				px_write_insn(ctx, insn, NULL, 0, var->label, 0);
			}
		} break;
		case (VAR_TYPE_PTR): {
			// Jump to pointer (TODO).
			px_insn_t insn = {
			};
		} break;
		case (VAR_TYPE_REG): {
			// Jump to register.
			px_insn_t insn = {
				.y = true,
				.x = ADDR_IMM,
				.b = var->reg,
				.a = REG_PC,
				.o = PX_OFFS_MOV | COND_JSR,
			};
			px_write_insn(ctx, insn, NULL, 0, NULL, 0);
		} break;
		case (VAR_TYPE_STACKOFFS): {
			// Jump to stack offset (TODO).
			px_insn_t insn = {
			};
		} break;
	}
	
	if (funcdef->returns && funcdef->returns->simple_type != STYPE_VOID) {
		// Make a return with values.
		gen_var_t retval;
		if (funcdef->returns->size <= NUM_REGS) {
			// Registrex.
			retval = (gen_var_t) {
				.type  = VAR_TYPE_REG,
				.reg   = REG_R0,
				.ctype = funcdef->returns,
				.owner = NULL
			};
		} else {
			// TODO!
			retval = (gen_var_t) {
				.type   = VAR_TYPE_VOID,
				.ctype  = ctype_simple(ctx, STYPE_VOID)
			};
		}
		
		return XCOPY(ctx->allocator, &retval, gen_var_t);
		
	} else {
		// Make a return void.
		gen_var_t dummy = {
			.type   = VAR_TYPE_VOID,
			.ctype  = ctype_simple(ctx, STYPE_VOID)
		};
		return XCOPY(ctx->allocator, &dummy, gen_var_t);
	}
}

// Expression: Binary math operation.
gen_var_t *gen_expr_math2(asm_ctx_t *ctx, oper_t oper, gen_var_t *out_hint, gen_var_t *a, gen_var_t *b) {
	address_t n_words = 1;
	bool isSigned     = true;
	
	// Can we simplify?
	if ((OP_IS_SHIFT(oper) || OP_IS_ADD(oper) || OP_IS_COMP(oper)) && b->type == VAR_TYPE_CONST && b->iconst == 1) {
		return gen_expr_math1(ctx, oper, out_hint, a);
	}
	
	if (OP_IS_LOGIC(oper)) {
		// TODO.
	} else if (oper == OP_INDEX) {
		// Construct the indexing hint.
		gen_var_t *hint = xalloc(ctx->allocator, sizeof(gen_var_t));
		*hint = (gen_var_t) {
			.type        = VAR_TYPE_INDEXED,
			.indexed     = {
				.location = a,
				.index    = b,
				.combined = NULL,
			},
			.ctype 	     = (a->ctype->category == TYPE_CAT_ARRAY || a->ctype->category == TYPE_CAT_POINTER)
						 ? a->ctype->underlying : ctype_simple(ctx, STYPE_S_INT), // TODO: : : : ?
			.owner       = NULL,
			.default_loc = NULL
		};
		
		if (out_hint) {
			// Move directly to the destination thing.
			gen_mov(ctx, out_hint, hint);
			return out_hint;
		} else {
			// Just give back the hint.
			return hint;
		}
		
	} else if (OP_IS_SHIFT(oper)) {
		// TODO.
	} else if (OP_IS_COMP(oper)) {
		// TODO.
	} else {
		// General math stuff.
		memword_t opcode = 0;
		switch (oper) {
			case OP_ADD:     opcode = PX_OP_ADD; break;
			case OP_SUB:     opcode = PX_OP_SUB; break;
			case OP_BIT_AND: opcode = PX_OP_AND; break;
			case OP_BIT_OR:  opcode = PX_OP_OR;  break;
			case OP_BIT_XOR: opcode = PX_OP_XOR; break;
		}
		return px_math2(ctx, opcode, out_hint, a, b);
	}
	
	printf("Unimplemented point was reached!\n");
	raise(SIGABRT);
	return NULL;
}

// Expression: Unary math operation.
gen_var_t *gen_expr_math1(asm_ctx_t *ctx, oper_t oper, gen_var_t *output, gen_var_t *a) {
	bool isSigned     = true;
	if (oper == OP_LOGIC_NOT) {
		if (a->type == VAR_TYPE_COND) {
			// Invert a branch condition.
			if (!output) output = a;
			a->cond = INV_BR(a->cond);
		} else {
			// Use the CMP1 optimisation.
			// Go to CMP1 (ULT).
			oper = OP_LT;
			goto cmp1;
		}
		
	} else if (OP_IS_COMP(oper)) {
		cmp1:
		if (!output || output->type != VAR_TYPE_COND) {
			// Make a new output.
			output  = xalloc(ctx->allocator, sizeof(gen_var_t));
			*output = (gen_var_t) {
				.type        = VAR_TYPE_COND,
				.owner       = NULL,
				.default_loc = NULL,
			};
		}
		px_math1(ctx, PX_OP_CMP1, output, a);
		// Translate comparison operators.
		switch (oper) {
			case OP_LT:
				output->cond = isSigned ? COND_SLT : COND_ULT;
				break;
			case OP_LE:
				output->cond = isSigned ? COND_SLE : COND_ULE;
				break;
			case OP_GT:
				output->cond = isSigned ? COND_SGT : COND_UGT;
				break;
			case OP_GE:
				output->cond = isSigned ? COND_SGE : COND_UGE;
				break;
			case OP_EQ:
				output->cond = COND_EQ;
				break;
			case OP_NE:
				output->cond = COND_NE;
				break;
		}
		return output;
		
	} else if (oper == OP_SHIFT_L) {
		// Shift left.
		return px_math1(ctx, PX_OP_SHL, output, a);
	} else if (oper == OP_SHIFT_R) {
		// Shift right.
		return px_math1(ctx, PX_OP_SHR, output, a);
	} else if (oper == OP_DEREF) {
		// Look at where the pointer goes to.
		gen_var_t *var = xalloc(ctx->allocator, sizeof(gen_var_t));
		var->type        = VAR_TYPE_PTR;
		var->ctype       = a->ctype->underlying;
		var->ptr         = a;
		var->owner       = NULL;
		var->default_loc = NULL;
		if (!var->ctype) var->ctype = ctype_simple(ctx, STYPE_S_INT);
		return var;
	} else if (oper == OP_ADROF) {
		if (a->default_loc) {
			// LEA of default location.
			// This starts clobbering things up.
			return gen_expr_math1(ctx, oper, output, a->default_loc);
		} else if (a->type == VAR_TYPE_LABEL) {
			bool use_hint = output && output->type == VAR_TYPE_REG;
			reg_t regno = use_hint ? output->reg : px_pick_reg(ctx, true);
			if (DET_PIE(ctx)) {
				// LEA (pie).
				px_insn_t insn = {
					.y = true,
					.x = ADDR_PC,
					.b = REG_IMM,
					.a = regno,
					.o = PX_OP_LEA,
				};
				px_write_insn(ctx, insn, NULL, 0, a->label, 0);
			} else {
				// LEA (non-pie).
				px_insn_t insn = {
					.y = true,
					.x = ADDR_MEM,
					.b = REG_IMM,
					.a = regno,
					.o = PX_OP_LEA,
				};
				px_write_insn(ctx, insn, NULL, 0, a->label, 0);
			}
			// Create a nice var.
			gen_var_t *var;
			if (use_hint) {
				var = output;
			} else {
				var = xalloc(ctx->allocator, sizeof(gen_var_t));
				var->owner       = NULL;
				var->default_loc = NULL;
				var->reg         = regno;
				var->type        = VAR_TYPE_REG;
				var->ctype       = ctype_ptr(ctx, a->ctype);
				ctx->current_scope->reg_usage[regno] = var;
			}
			return var;
		} else if (a->type == VAR_TYPE_STACKOFFS) {
			bool use_hint = output && output->type == VAR_TYPE_REG;
			// LEA (stack).
			reg_t regno = use_hint ? output->reg : px_pick_reg(ctx, true);
			px_insn_t insn = {
				.y = true,
				.x = ADDR_ST,
				.b = REG_IMM,
				.a = regno,
				.o = PX_OP_LEA,
			};
			px_write_insn(ctx, insn, NULL, 0, NULL, px_get_depth(ctx, a, 0));
			// Create a nice var.
			gen_var_t *var;
			if (use_hint) {
				var = output;
			} else {
				var = xalloc(ctx->allocator, sizeof(gen_var_t));
				var->owner       = NULL;
				var->default_loc = NULL;
				var->reg         = regno;
				var->type        = VAR_TYPE_REG;
				var->ctype       = ctype_ptr(ctx, a->ctype);
				ctx->current_scope->reg_usage[regno] = var;
			}
			return var;
		} else {
			// Move it to a stack temp.
			// This starts clobbering things up.
			gen_var_t *tmp = px_get_tmp(ctx, 1, false);
			tmp->ctype = a->ctype;
			gen_mov(ctx, tmp, a);
			return gen_expr_math1(ctx, oper, output, tmp);
		}
	} else if (oper == OP_ADD) {
		// Increment.
		return px_math1(ctx, PX_OP_INC, output, a);
	} else if (oper == OP_SUB) {
		// Decrement.
		return px_math1(ctx, PX_OP_DEC, output, a);
	}
}

// Helper for reinterpretation casts.
static gen_var_t *px_reinterpret(asm_ctx_t *ctx, gen_var_t *a, var_type_t *ctype) {
	a = xmake_copy(ctx->current_scope->allocator, a, sizeof(gen_var_t));
	a->ctype = ctype;
	return a;
}

// Expression: Type cast.
gen_var_t *gen_cast(asm_ctx_t *ctx, gen_var_t *a, var_type_t *ctype) {
	// Determine whether the source is a float.
	bool is_src_float = a->ctype->category == TYPE_CAT_SIMPLE && a->ctype->simple_type >= STYPE_FLOAT && a->ctype->simple_type <= STYPE_LONG_DOUBLE;
	// Determine whether the target is a float.
	bool is_dst_float = ctype->category == TYPE_CAT_SIMPLE && ctype->simple_type >= STYPE_FLOAT && ctype->simple_type <= STYPE_LONG_DOUBLE;
	
	if (a->type == VAR_TYPE_CONST) {
		if (is_src_float || is_dst_float) {
			// Float type constants need to be converted.
			// But we don't have float yet ¯\_(ツ)_/¯
			return a;
		} else {
			// Any other constants can be reinterpreted.
			return px_reinterpret(ctx, a, ctype);
		}
	} else if (a->ctype->category == TYPE_CAT_SIMPLE) {
		if (is_src_float || is_src_float) {
			// Again, float types need to be converted.
			// But we don't have float yet ¯\_(ツ)_/¯
			return a;
		} else if (a->ctype->size == ctype->size) {
			// Some simple types can be reinterpreted.
			return px_reinterpret(ctx, a, ctype);
		} else {
			// Other simple types need to be explicitly casted.
			// Get a little new variable.
			gen_var_t *b = px_get_tmp(ctx, ctype->size, true);
			b->ctype = ctype;
			px_insn_t insn;
			
			// We shall now ACCESS the MEMORY.
			px_memclobber(ctx, a->type == VAR_TYPE_STACKOFFS || b->type == VAR_TYPE_STACKOFFS);
			
			if (ctype->size < a->ctype->size) {
				// Just do a copy.
				px_mov_n(ctx, b, a, ctype->size);
				
			} else if (b->type != VAR_TYPE_REG || (b->ctype->size - a->ctype->size) > 1) {
				// Carry extension code with temp reg.
				px_mov_n(ctx, b, a, a->ctype->size);
				// We need to make carry extension code.
				reg_t regno;
				if (a->type == VAR_TYPE_REG) {
					// Use the part reg.
					regno = a->reg + a->ctype->size - 1;
				} else {
					// Pick a temp reg.
					regno = px_pick_reg(ctx, true);
					// Move the highest word into our temp reg.
					px_part_to_reg(ctx, a, regno, a->ctype->size - 1);
				}
				
				// Now, extend using this value's sign bit.
				for (address_t i = a->ctype->size; i < ctype->size; i++) {
					insn = (px_insn_t) {
						.y = 0,
						.b = regno,
						.o = PX_OP_MOV | COND_CX,
					};
					asm_label_t label0 = NULL;
					address_t   offs0  = 0;
					insn.a = px_addr_var(ctx, b, i, &insn.x, &label0, &offs0, regno);
					px_write_insn(ctx, insn, label0, offs0, NULL, 0);
				}
			} else {
				// Carry extension code without temp reg.
				px_mov_n(ctx, b, a, a->ctype->size);
				
				// Now, extend using this value's sign bit.
				for (address_t i = a->ctype->size; i < ctype->size; i++) {
					insn = (px_insn_t) {
						.y = 1,
						.a = b->reg + i,
						.o = PX_OP_MOV | COND_CX,
					};
					asm_label_t label1 = NULL;
					address_t   offs1  = 0;
					px_addr_var(ctx, a, a->ctype->size - 1, &insn.x, &label1, &offs1, insn.a);
					px_write_insn(ctx, insn, NULL, 0, label1, offs1);
				}
			}
			
			return b;
		}
	} else if (ctype->category == TYPE_CAT_POINTER) {
		if (a->ctype->category == TYPE_CAT_ARRAY) {
			// Creating pointer from array.
			// TODO
			return a;
		} else {
			// Pointer reinterpretation.
			return px_reinterpret(ctx, a, ctype);
		}
	}
}

/* ================== Variables ================== */

// Make a certain amount of space in the stack.
void gen_stack_space(asm_ctx_t *ctx, address_t num) {
	// if (!num) return;
	// ctx->current_scope->real_stack_size += num;
}

// Scale the stack back down.
void gen_stack_clear(asm_ctx_t *ctx, address_t num) {
	// if (!num) return;
	// ctx->current_scope->real_stack_size -= num;
}

// Called before a memory clobbering instruction is to be written.
void px_memclobber(asm_ctx_t *ctx, bool clobbers_stack) {
	if (clobbers_stack) {
		// Check whether the stack size differs at all.
		addrdiff_t diff = ctx->current_scope->real_stack_size - ctx->current_scope->stack_size;
		// Prevent infinite recursion.
		ctx->current_scope->real_stack_size = ctx->current_scope->stack_size;
		
		// Fix stack size.
		if (diff == 1 || diff == -1) {
			// Increment or decrement ST.
			px_insn_t insn = {
				.y = 0,
				.x = ADDR_IMM,
				.b = 0,
				.a = REG_ST,
				.o = diff == 1 ? PX_OP_INC : PX_OP_DEC,
			};
			px_write_insn(ctx, insn, NULL, 0, NULL, 0);
		} else if (diff > 1) {
			// Add to ST.
			px_insn_t insn = {
				.y = 0,
				.x = ADDR_IMM,
				.b = REG_IMM,
				.a = REG_ST,
				.o = PX_OP_ADD,
			};
			px_write_insn(ctx, insn, NULL, 0, NULL, diff);
		} else if (diff < -1) {
			// Sub from ST for clarity.
			px_insn_t insn = {
				.y = 0,
				.x = ADDR_IMM,
				.b = REG_IMM,
				.a = REG_ST,
				.o = PX_OP_SUB,
			};
			px_write_insn(ctx, insn, NULL, 0, NULL, -diff);
		}
	}
}

// Variables: Move variable to another location.
void px_mov_n(asm_ctx_t *ctx, gen_var_t *dst, gen_var_t *src, address_t n_words) {
	if (gen_cmp(ctx, dst, src)) return;
	if (dst->type == VAR_TYPE_REG) {
		// To register move.
		for (address_t i = 0; i < n_words; i++) {
			px_part_to_reg(ctx, src, dst->reg + i, i);
		}
	} else if (dst->type == VAR_TYPE_COND) {
		// Convert to condition.
		dst->cond = px_var_to_cond(ctx, src);
	} else {
		// Move through register.
		reg_t regno;
		bool  mov_to_reg = false;
		if (src->type == VAR_TYPE_CONST) {
			// We can encode this differently.
			regno = REG_IMM;
		} else if (src->type == VAR_TYPE_REG) {
			// Use the existing register.
			regno = src->reg;
		} else {
			// Pick a new register.
			regno = px_pick_reg(ctx, true);
			mov_to_reg = true;
		}
		
		for (address_t i = 0; i < n_words; i++) {
			px_insn_t insn = {
				.y = 0,
				.o = PX_OP_MOV,
			};
			asm_label_t label0 = NULL;
			address_t   offs0  = 0;
			asm_label_t label1 = NULL;
			address_t   offs1  = 0;
			// Collect addressing modes.
			insn.a = px_addr_var(ctx, dst, i, &insn.x, &label0, &offs0, 0);
			if (mov_to_reg) {
				// Move B to a temp register.
				px_part_to_reg(ctx, src, regno, i);
				insn.b = regno;
			} else if (insn.x == ADDR_IMM) {
				// Address using B.
				insn.y = 1;
				insn.b = px_addr_var(ctx, src, i, &insn.x, &label1, &offs1, regno);
			} else {
				// Use B as a reg or const.
				insn.b = px_addr_var(ctx, src, i, NULL, &label1, &offs1, regno);
			}
			// Write the resulting instruction.
			px_write_insn(ctx, insn, label0, offs0, label1, offs1);
		}
	}
}

// Variables: Move given variable into a register.
void px_var_to_reg(asm_ctx_t *ctx, gen_var_t *var, bool allow_const) {
	// Check whether it's already in a register.
	if ((var->type != VAR_TYPE_CONST || !allow_const) && var->type != VAR_TYPE_REG) {
		// Make a copy of the original.
		gen_var_t *orig = xalloc(ctx->current_scope->allocator, sizeof(gen_var_t));
		memcpy(orig, var, sizeof(gen_var_t));
		
		// Reconfigure the variable.
		reg_t regno = px_pick_reg(ctx, true);
		*var = (gen_var_t) {
			.type        = VAR_TYPE_REG,
			.reg         = regno,
			.owner       = orig->owner,
			.ctype       = orig->ctype,
			.default_loc = orig->default_loc ? orig->default_loc : orig,
		};
		ctx->current_scope->reg_usage[regno] = var;
		
		// Clean up.
		if (orig->default_loc) {
			xfree(ctx->current_scope->allocator, orig);
		}
	}
}

// Variables: Move variable to another location.
void gen_mov(asm_ctx_t *ctx, gen_var_t *dst, gen_var_t *src) {
	address_t n_words;
	if (dst->type == VAR_TYPE_UNASSIGNED && src->type == VAR_TYPE_REG && 0 /* no copy is ok? */) {
		// Unassigned variable optimisation.
		// Make default location.
		src->default_loc = dst;
		// Swap src and dst.
		gen_var_t tmp = *dst;
		*dst = *src;
		*src = tmp;
		return;
	}
	// Normal copy.
	if (src->type == VAR_TYPE_CONST) {
		n_words = dst->ctype->size;
	} else {
		n_words = src->ctype->size < dst->ctype->size
				? src->ctype->size : dst->ctype->size;
	}
	px_mov_n(ctx, dst, src, n_words);
}

// Gets or adds a temp var.
// Each temp label represents one word, so some variables will use multiple.
gen_var_t *px_get_tmp(asm_ctx_t *ctx, size_t size, bool allow_reg) {
	// Try to pick an empty register.
	if (size == 1 && allow_reg) {
		for (reg_t i = 0; i < NUM_REGS; i++) {
			if (!ctx->current_scope->reg_usage[i]) {
				// We can use this register.
				gen_var_t *var = xalloc(ctx->current_scope->allocator, sizeof(gen_var_t));
				*var = (gen_var_t) {
					.type        = VAR_TYPE_REG,
					.reg         = i,
					.owner       = NULL,
					.default_loc = NULL
				};
				
				ctx->current_scope->reg_usage[i] = var;
				return var;
			}
		}
	}
	
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
			// Return the found stack offset.
			address_t end_offset = ctx->current_scope->stack_size - ctx->temp_num;
			address_t offset     = end_offset - i;
			// Package it up.
			gen_var_t *var = xalloc(ctx->current_scope->allocator, sizeof(gen_var_t));
			*var = (gen_var_t) {
				.type        = VAR_TYPE_STACKOFFS,
				.offset      = offset,
				.owner       = NULL,
				.default_loc = NULL
			};
			return var;
		}
	}
	
	// Make some more.
	char *func_label;
	char *label;
	make_one:
	func_label = ctx->current_func->ident.strval;
	label = xalloc(ctx->allocator, strlen(func_label) + 8);
	sprintf(label, "%s.LT%04x", func_label, ctx->temp_num);
	DEBUG_GEN("// Add temp label %s\n", label);
	gen_define_temp(ctx, label);
	
	// Return the new stack bit.
	ctx->current_scope->stack_size += size;
	gen_stack_space(ctx, size);
	gen_var_t *var = xalloc(ctx->current_scope->allocator, sizeof(gen_var_t));
	*var = (gen_var_t) {
		.type        = VAR_TYPE_STACKOFFS,
		.offset      = ctx->current_scope->stack_size - size,
		.owner       = NULL,
		.default_loc = NULL
	};
	return var;
}

// Variables: Create a label for the variable at preprocessing time.
// Must allocate a new gen_var_t object.
gen_var_t *gen_preproc_var(asm_ctx_t *ctx, preproc_data_t *parent, ident_t *ident) {
	// Create a label.
	char *fn_label = ctx->current_func->ident.strval;
	char *label = xalloc(ctx->allocator, strlen(fn_label) + 8);
	sprintf(label, "%s.LV%04lx", fn_label, ctx->current_scope->local_num);
	
	// Package it into a gen_var_t.
	gen_var_t loc = {
		.type   = VAR_TYPE_STACKOFFS,
		.offset = -1,
		.owner  = ident->strval,
		.ctype  = ident->type,
	};
	
	// And return a copy.
	return XCOPY(ctx->allocator, &loc, gen_var_t);
}
