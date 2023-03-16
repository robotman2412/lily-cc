
#include "pixie-16_instruction.h"
#include "pixie-16_options.h"
#include "gen_util.h"
#include "definitions.h"
#include "asm.h"
#include "malloc.h"
#include "string.h"
#include "signal.h"

#include "pixie-16_internal.h"

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
// Does not check for memory clobbers.
void px_write_insn_raw(asm_ctx_t *ctx, px_insn_t insn, asm_label_t label0, address_t offs0, asm_label_t label1, address_t offs1) {
	#ifdef ENABLE_DEBUG_LOGS
	// Debug log variables.
	char *imm0 = NULL;
	char *imm1 = NULL;
	#endif
	
	// Test for suspicious instructions.
	if ((insn.y == 0 && insn.x == insn.a && insn.x != PX_ADDR_IMM) || (insn.y == 1 && insn.x == insn.b && insn.x != PX_ADDR_IMM)) {
		DEBUG_GEN("// WARNING: Suspicious instruction.\n");
		DEBUGGER();
	}
	
	// Test for impossible instructions.
	if ((insn.y == 0 && insn.x == PX_ADDR_IMM && insn.a == PX_REG_IMM) || (insn.y == 1 && insn.a == PX_REG_IMM)) {
		DEBUG_GEN("// WARNING: Invalid instruction.\n");
		DEBUGGER();
	}
	
	// Write the basic instruction.
	asm_write_memword(ctx, px_pack_insn(insn));
	
	// Determine the count of IMM parameters.
	uint_fast8_t num_param = (insn.a == PX_REG_IMM) + (insn.b == PX_REG_IMM);
	
	if (insn.a == PX_REG_IMM) {
		if (label0) {
			if (!insn.y && insn.x == PX_ADDR_PC) {
				// Determine offset relative to instruction end instead of label reference.
				address_t offs = (insn.b == PX_REG_IMM) ? offs0 - 2 : offs0 - 1;
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
	if (insn.b == PX_REG_IMM) {
		if (label1) {
			if (insn.y && insn.x == PX_ADDR_PC) {
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
// Does not check for memory clobbers.
void px_write_insn_iasm(asm_ctx_t *ctx, px_insn_t insn, asm_label_t label0, address_t offs0, asm_label_t label1, address_t offs1) {
	px_write_insn_raw(ctx, insn, label0, offs0, label1, offs1);
}

// Write an instruction with some context.
// Checks for memory clobbers.
void px_write_insn(asm_ctx_t *ctx, px_insn_t insn, asm_label_t label0, address_t offs0, asm_label_t label1, address_t offs1) {
	// Test whether this is a write to stack.
	if (insn.y == 0 && insn.x == PX_ADDR_ST) {
		// Test for possible stack push operation.
		addrdiff_t error = ctx->current_scope->stack_size - ctx->current_scope->real_stack_size;
		if (error - 1 == offs0) {
			DEBUG_GEN("// Stack optimised (push).\n");
			
			// Re-write the insn.
			insn.x = PX_ADDR_MEM;
			insn.a = PX_REG_ST;
			px_write_insn_raw(ctx, insn, NULL, 0, label1, offs1);
			ctx->current_scope->real_stack_size ++;
			return;
		}
	}
	
	// Determine memory clobbers.
	if (insn.a == PX_REG_ST || insn.b == PX_REG_ST || insn.x == PX_ADDR_ST) {
		px_memclobber(ctx, true);
	}
	
	px_write_insn_raw(ctx, insn, label0, offs0, label1, offs1);
}



// Generate a branch to one of two labels.
void px_branch(asm_ctx_t *ctx, expr_t *expr, gen_var_t *cond_var, char *l_true, char *l_false) {
	if (!l_true && !l_false) return;
	cond_t cond = px_var_to_cond(ctx, expr, cond_var);
	
	if (DET_PIE(ctx)) {
		// PIE alternative.
		if (l_true) {
			px_insn_t insn = {
				.y = true,
				.x = PX_ADDR_PC,
				.b = PX_REG_IMM,
				.a = PX_REG_PC,
				.o = PX_OFFS_LEA | cond,
			};
			px_write_insn(ctx, insn, NULL, 0, l_true, 0);
		}
		if (l_false) {
			px_insn_t insn = {
				.y = true,
				.x = PX_ADDR_PC,
				.b = PX_REG_IMM,
				.a = PX_REG_PC,
				.o = PX_OFFS_LEA | INV_BR(cond),
			};
			px_write_insn(ctx, insn, NULL, 0, l_false, 0);
		}
	} else {
		// Non-PIE alternative
		if (l_true) {
			px_insn_t insn = {
				.y = true,
				.x = PX_ADDR_MEM,
				.b = PX_REG_IMM,
				.a = PX_REG_PC,
				.o = PX_OFFS_MOV | cond,
			};
			px_write_insn(ctx, insn, NULL, 0, l_true, 0);
		}
		if (l_false) {
			px_insn_t insn = {
				.y = true,
				.x = PX_ADDR_MEM,
				.b = PX_REG_IMM,
				.a = PX_REG_PC,
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
			.x = PX_ADDR_PC,
			.b = PX_REG_IMM,
			.a = PX_REG_PC,
			.o = PX_OP_LEA,
		};
		px_write_insn(ctx, insn, NULL, 0, label, 0);
	} else {
		// Non-PIE alternative
		px_insn_t insn = {
			.y = true,
			.x = PX_ADDR_MEM,
			.b = PX_REG_IMM,
			.a = PX_REG_PC,
			.o = PX_OP_MOV,
		};
		px_write_insn(ctx, insn, NULL, 0, label, 0);
	}
}



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

// Pick a register to use.
reg_t px_pick_reg(asm_ctx_t *ctx, bool do_vacate) {
	// Check for a free register.
	for (reg_t i = 0; i < NUM_REGS; i++) {
		if (!ctx->current_scope->reg_usage[i] && !ctx->reg_temp_usage[i]) {
			return i;
		}
	}
	
	// Otherwise, free up the least used one.
	reg_t pick;
	for (reg_t i = 0; i < NUM_REGS; i++) {
		if (!ctx->reg_temp_usage[ctx->reg_usage_order[i]]) {
			pick = ctx->reg_usage_order[i];
		}
	}
	
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

// Pick a register to use, but only pick empty registers.
bool px_pick_empty_reg(asm_ctx_t *ctx, reg_t *regno, address_t size) {
	if (size == 1) {
		// Check for a free register.
		for (reg_t i = 0; i < NUM_REGS; i++) {
			if (!ctx->current_scope->reg_usage[i]) {
				*regno = i;
				return true;
			}
		}
	} else if (size > 1) {
		reg_t first_free;
		bool  is_free = false;
		// Check for a free register.
		for (reg_t i = 0; i < NUM_REGS; i++) {
			if (!ctx->current_scope->reg_usage[i]) {
				if (is_free && i - first_free + 1 >= size) {
					*regno = first_free;
					return true;
				} else if (!is_free) {
					is_free = true;
					first_free = i;
				}
			} else {
				is_free = false;
			}
		}
	}
	
	return false;
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
	// DEBUG_GEN("// Add temp label %s\n", label);
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

// Gets the constant required for a stack indexing memory access.
 __attribute__((pure))
address_t px_get_depth(asm_ctx_t *ctx, gen_var_t *var, address_t var_offs) {
	return ctx->current_scope->stack_size - var->offset + var_offs - var->ctype->size;
}

// Grab an addressing mode for a parameter.
reg_t px_addr_var(asm_ctx_t *ctx, gen_var_t *var, address_t part, px_addr_t *addrmode, asm_label_t *label, address_t *offs, reg_t dest) {
	static px_addr_t addrmode_dummy;
	if (!addrmode) {
		if (var->type != VAR_TYPE_CONST && var->type != VAR_TYPE_REG) {
			// We must move this to register.
			px_part_to_reg(ctx, var, dest, part);
			return dest;
		}
		
		addrmode = &addrmode_dummy;
	}
	DEBUG_GEN("// Addr var type %d\n", var->type);
	
	switch (var->type) {
			gen_var_t *a, *b;
			
		case VAR_TYPE_CONST:
			// Constant thingy.
			*addrmode = PX_ADDR_IMM;
			*offs = var->iconst >> (MEM_BITS * part);
			return PX_REG_IMM;
			
		case VAR_TYPE_LABEL:
			// At label.
			*addrmode = PX_ADDR_MEM;
			*label = var->label;
			*offs = part;
			return PX_REG_IMM;
			
		case VAR_TYPE_STACKOFFS:
			// In stack.
			*addrmode = PX_ADDR_ST;
			*offs = px_get_depth(ctx, var, part);
			return PX_REG_IMM;
			
		case VAR_TYPE_REG:
			// In register.
			*addrmode = PX_ADDR_IMM;
			return var->reg + part;
			
		case VAR_TYPE_RETVAL:
			// TODO
			break;
			
		case VAR_TYPE_PTR:
			if (var->ptr->type == VAR_TYPE_REG) {
				// This is a bit simpler.
				if (part) {
					*addrmode = var->ptr->reg;
					*offs = part;
					return PX_REG_IMM;
				} else {
					*addrmode = PX_ADDR_MEM;
					return var->ptr->reg;
				}
			} else if (var->ptr->type == VAR_TYPE_CONST) {
				// A constant point might as well be a normal memory access.
				*addrmode = PX_ADDR_MEM;
				*offs = var->ptr->iconst;
				return PX_REG_IMM;
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
				*addrmode = PX_ADDR_MEM;
				// Return the registrex.
				return dest;
			}
			break;
		
		case VAR_TYPE_INDEXED:
			// Array indexing.
			a = var->indexed.location;
			b = var->indexed.index;
			
			if (a->ctype->category == TYPE_CAT_ARRAY && a->type != VAR_TYPE_PTR) {
				// Array type indexing.
				
				// Get components into registers if not already.
				px_var_to_reg(ctx, b, true);
				
				// Determine the underlying type of the operation.
				var_type_t *underlying = var->ctype;
				
				if (b->type == VAR_TYPE_CONST) {
					// B is a constant, modify offset.
					return px_addr_var(ctx, a, part + b->iconst * underlying->size, addrmode, label, offs, dest);
					
				} else if (underlying->size == 1 && a->type == VAR_TYPE_LABEL) {
					// Underlying size is 1, add B to address.
					*addrmode = b->reg;
					*offs     = part;
					return PX_REG_IMM;
					
				} else if (underlying->size == 1) {
					// Underlying size is 1, add B to address.
					gen_var_t *tmp = px_get_tmp(ctx, 1, true);
					tmp->ctype = ctype_simple(ctx, STYPE_U_INT);
					
					// Perform LEA of location to index.
					px_insn_t insn = {
						.y = 1,
						.o = PX_OP_LEA
					};
					asm_label_t label1 = NULL;
					address_t   offs1  = 0;
					insn.b = px_addr_var(ctx, a, 0, &insn.x, &label1, &offs1, dest);
					px_var_to_reg(ctx, tmp, false);
					insn.a = tmp->reg;
					px_write_insn(ctx, insn, NULL, 0, label1, offs1);
					
					// Return a combined form.
					*addrmode = tmp->reg;
					*offs     = 0;
					return b->reg;
					
				} else {
					// B is in a register, perform address calculation.
					// TODO: There is a multiplication here.
				}
				
			} else {
				// Pointer type index.
				
				// Get components into registers if not already.
				px_var_to_reg(ctx, a, true);
				px_var_to_reg(ctx, b, true);
				
				// Check for constants.
				if (a->type == VAR_TYPE_CONST) {
					if (b->type == VAR_TYPE_CONST) {
						// TODO: CONVERT the thingy.
					} else {
						// Swapperoni.
						gen_var_t *tmp = a;
						a = b;
						b = tmp;
					}
				}
				
				// Determine the underlying type of the operation.
				var_type_t *underlying = var->ctype;
				
				if (b->type == VAR_TYPE_CONST && (b->iconst + part)) {
					// Constant (nonzero) offset and variable offset.
					*addrmode = a->reg;
					*offs     = b->iconst * underlying->size + part;
					return PX_REG_IMM;
				} else if (b->type == VAR_TYPE_CONST) {
					// Constant (zero) offset and variable offset.
					*addrmode = PX_ADDR_MEM;
					return a->reg;
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
							.ctype       = underlying,
							.owner       = NULL,
							.default_loc = NULL,
						};
						
						ctx->current_scope->reg_usage[dest] = var->indexed.combined;
					}
					
					// Construct with the offset.
					*addrmode = var->indexed.combined->reg;
					*offs     = part;
					return PX_REG_IMM;
				}
			}
			break;
	}
	return 0;
}
