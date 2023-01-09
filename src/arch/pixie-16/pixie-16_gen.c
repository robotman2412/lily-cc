
#include "pixie-16_gen.h"
#include "pixie-16_options.h"
#include "pixie-16_instruction.h"
#include "gen_util.h"
#include "definitions.h"
#include "asm.h"
#include "malloc.h"
#include "string.h"
#include "signal.h"

#include "pixie-16_internal.h"

/* ======== Gen-specific helper functions ======== */

// Creates a branch condition from a variable.
cond_t px_var_to_cond(asm_ctx_t *ctx, expr_t *expr, gen_var_t *var) {
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
		gen_expr_math1(ctx, expr, OP_GE, &cond_hint, var);
		return COND_UGE;
	}
}

// Move part of a value to a register.
void px_part_to_reg(asm_ctx_t *ctx, gen_var_t *var, reg_t dest, address_t index) {
	if (var->type == VAR_TYPE_CONST) {
		word_t iconst = var->iconst >> (MEM_BITS * index);
		if (!iconst) {
			// XOR dest, dest optimisation.
			px_insn_t insn = {
				.y = 0,
				.x = PX_ADDR_IMM,
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

// Variables: Move stored variablue out of the given register.
void px_vacate_reg(asm_ctx_t *ctx, reg_t regno) {
	// Check for existing data.
	gen_var_t *stored = ctx->current_scope->reg_usage[regno];
	if (!stored) {
		DEBUG_GEN("// Nothing to vacate.\n");
		return;
	}
	
	// Does it have a default which is not register?
	if (stored->default_loc && stored->default_loc->type != VAR_TYPE_REG) {
		// Then move it to it's default.
		gen_mov(ctx, stored->default_loc, stored);
		*stored = *stored->default_loc;
		DEBUG_GEN("// Vacated to default loc.\n");
	} else {
		// Otherwise, give it a non-register temp location.
		gen_var_t *temp_loc   = px_get_tmp(ctx, stored->ctype->size, false);
		temp_loc->ctype       = stored->ctype;
		temp_loc->owner       = stored->owner;
		temp_loc->default_loc = stored->default_loc;
		
		// Move it in.
		gen_mov(ctx, temp_loc, stored);
		*stored = *temp_loc;
		xfree(ctx->current_scope->allocator, temp_loc);
		DEBUG_GEN("// Vacated to temp loc.\n");
	}
	
	// Mark as free.
	ctx->current_scope->reg_usage[regno] = NULL;
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
	bool      a_big      = a->ctype->size > b->ctype->size;
	address_t n_words    = a_big ? b->ctype->size : a->ctype->size;
	address_t n_ext      = (a_big ? a->ctype->size : b->ctype->size) - n_words;
	
	bool       swappable = opcode == PX_OP_ADD || opcode == PX_OP_XOR || opcode == PX_OP_AND || opcode == PX_OP_OR;
	
	// Translate retval into it's real location.
	if (out_hint && out_hint->type == VAR_TYPE_RETVAL) {
		out_hint->type = VAR_TYPE_REG;
		out_hint->reg  = PX_REG_R0;
		if (b->type == VAR_TYPE_REG && b->reg == PX_REG_R0) {
			if (swappable) {
				gen_var_t *tmp = a;
				a = b;
				b = tmp;
			} else {
				px_vacate_reg(ctx, PX_REG_R0);
			}
		}
	}
	
	gen_var_t *output    = out_hint;
	bool       do_copy   = !gen_cmp(ctx, output, a) && opcode != PX_OP_CMP;
	if (!output) {
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
		px_touch_reg(ctx, reg_b);
		ctx->reg_temp_usage[reg_b] = true;
	} else if (b->type == VAR_TYPE_CONST) {
		reg_b = PX_REG_IMM;
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
		} else if (insn.x == PX_ADDR_IMM) {
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
	
	if (conv_b) {
		// We're done with any temp registers.
		ctx->reg_temp_usage[reg_b] = false;
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

// Determine the calling conventions to use.
void px_update_cc(asm_ctx_t *ctx, funcdef_t *funcdef) {
	address_t arg_size = 0;
	for (size_t i = 0; i < funcdef->args.num; i++) {
		arg_size += funcdef->args.arr[i].type->size;
	}
	
	if (arg_size > NUM_REGS) {
		funcdef->call_conv = PX_CC_STACK;
		if (funcdef->returns->size < 4) {
			funcdef->num_reg_to_push = 4 - funcdef->returns->size;
		} else {
			funcdef->num_reg_to_push = 4;
		}
		
	} else if (arg_size) {
		funcdef->call_conv = PX_CC_REGS;
		if (funcdef->returns->size > 4 || funcdef->returns->size == 0) {
			funcdef->num_reg_to_push = 4 - arg_size;
		} else {
			int a = 4 - arg_size;
			int b = 4 - funcdef->returns->size;
			funcdef->num_reg_to_push = a < b ? a : b;
		}
		
	} else {
		funcdef->call_conv = PX_CC_NONE;
		if (funcdef->returns->size < 4) {
			funcdef->num_reg_to_push = 4 - funcdef->returns->size;
		} else {
			funcdef->num_reg_to_push = 4;
		}
	}
	
	funcdef->is_entry = entrypoint && !strcmp(funcdef->ident.strval, entrypoint);
	funcdef->is_irq   = irqvector  && !strcmp(funcdef->ident.strval, irqvector);
	funcdef->is_nmi   = nmivector  && !strcmp(funcdef->ident.strval, nmivector);
}

// Helper for generating register push.
static void px_entry_push_regs(asm_ctx_t *ctx, funcdef_t *funcdef) {
	// Is this entry point?
	if (!funcdef->is_entry) {
		// If not, push the non-param registers.
		for (int i = 0; i < funcdef->num_reg_to_push; i++) {
			px_insn_t insn = {
				.y = 0,
				.x = PX_ADDR_MEM,
				.b = 3 - i,
				.a = PX_REG_ST,
				.o = PX_OP_MOV,
			};
			px_write_insn_raw(ctx, insn, NULL, 0, NULL, 0);
		}
	}
}

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
		
		// Push regs now.
		px_entry_push_regs(ctx, funcdef);
		
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
				.offset      = ctx->current_scope->stack_size,
				.owner       = funcdef->args.arr[i].strval,
				.ctype       = funcdef->args.arr[i].type,
				.default_loc = NULL
			};
			var->default_loc = loc;
			
			// Mark the register as used.
			ctx->current_scope->reg_usage[i] = var;
			px_touch_reg(ctx, i);
			
			gen_define_var(ctx, var, funcdef->args.arr[i].strval);
			arg_size += var->ctype->size;
			ctx->current_scope->stack_size += var->ctype->size;
		}
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
				.offset      = ctx->current_scope->stack_size,
				.owner       = funcdef->args.arr[i].strval,
				.ctype       = funcdef->args.arr[i].type,
				.default_loc = NULL
			};
			// Which is also it's default location.
			var->default_loc = XCOPY(ctx->allocator, var, gen_var_t);
			
			gen_define_var(ctx, var, funcdef->args.arr[i].strval);
			arg_size += var->ctype->size;
			ctx->current_scope->stack_size += var->ctype->size;
			ctx->current_scope->real_stack_size += var->ctype->size;
		}
		
		// Account for return address in stack.
		ctx->current_scope->stack_size ++;
		ctx->current_scope->real_stack_size ++;
		
		// Push regs now.
		px_entry_push_regs(ctx, funcdef);
	} else {
		// None.
		DEBUG_GEN("// calling convention: no parameters\n");
		
		// Push regs now.
		px_entry_push_regs(ctx, funcdef);
	}
	
	funcdef->base_stack_size = ctx->current_scope->stack_size;
	DEBUG_GEN("// base stack size: %u\n", funcdef->base_stack_size);
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
		px_mov_to_reg(ctx, retval, PX_REG_R0);
	}
	
	// Pop some stuff off the stack.
	ctx->current_scope->stack_size = funcdef->base_stack_size;
	px_memclobber(ctx, true);
	
	// Is this entry point?
	if (!funcdef->is_entry) {
		// If not, pop the non-parameter registers.
		for (int i = 0; i < funcdef->num_reg_to_push; i++) {
			px_insn_t insn = {
				.y = 1,
				.x = PX_ADDR_MEM,
				.b = PX_REG_ST,
				.a = 4 - funcdef->num_reg_to_push + i,
				.o = PX_OP_MOV,
			};
			px_write_insn(ctx, insn, NULL, 0, NULL, 0);
		}
	}
	
	if (funcdef->is_irq || funcdef->is_nmi) {
		// Append the special interrupt return.
		px_insn_t insn = {
			.y = 1,
			.x = PX_ADDR_MEM,
			.b = PX_REG_ST,
			.a = PX_REG_PF,
			.o = PX_OP_MOV,
		};
		px_write_insn(ctx, insn, NULL, 0, NULL, 0);
	}
	
	// Append the return.
	px_insn_t insn = {
		.y = 1,
		.x = PX_ADDR_MEM,
		.b = PX_REG_ST,
		.a = PX_REG_PC,
		.o = PX_OP_MOV,
	};
	px_write_insn(ctx, insn, NULL, 0, NULL, 0);
}

/* ================== Statements ================= */

// Check whether a statement can be reduced to some MOV.
bool px_is_mov_stmt(asm_ctx_t *ctx, stmt_t *stmt) {
	if (!stmt) return false;
	
	if (stmt->type == STMT_TYPE_MULTI) {
		for (size_t i = 0; i < stmt->stmts->num; i++) {
			if (!px_is_mov_stmt(ctx, &stmt->stmts->arr[i])) return false;
		}
		return true;
	} else if (stmt->type == STMT_TYPE_EXPR) {
		expr_t *expr = stmt->expr;
		return expr->type == EXPR_TYPE_MATH2 && expr->par_a->type == EXPR_TYPE_IDENT
				&& (expr->par_b->type == EXPR_TYPE_IDENT
				||  expr->par_b->type == EXPR_TYPE_CONST
				||  expr->par_b->type == EXPR_TYPE_CSTR);
	} else {
		return false;
	}
}

// Generate some conditional MOV statements.
void px_gen_cond_mov_stmt(asm_ctx_t *ctx, stmt_t *stmt, cond_t cond) {
	if (stmt->type == STMT_TYPE_MULTI) {
		for (size_t i = 0; i < stmt->stmts->num; i++) {
			px_gen_cond_mov_stmt(ctx, &stmt->stmts->arr[i], cond);
		}
		return;
	} else if (stmt->type != STMT_TYPE_EXPR) {
		return;
	}
	
	oper_t oper = stmt->expr->oper;
}

// Check whether an if statement can be reduced to conditional MOV.
bool px_cond_mov_applicable(asm_ctx_t *ctx, gen_var_t *cond, stmt_t *s_if, stmt_t *s_else) {
	bool mov_if   = !s_if   || px_is_mov_stmt(ctx, s_if);
	bool mov_else = !s_else || px_is_mov_stmt(ctx, s_else);
	
	return mov_if && mov_else;
}

// If statement implementation.
bool gen_if(asm_ctx_t *ctx, stmt_t *stmt, gen_var_t *cond, stmt_t *s_if, stmt_t *s_else) {
	// Optimise out empty statements.
	if (s_if   && stmt_is_empty(s_if)) {
		s_if = NULL;
	}
	if (s_else && stmt_is_empty(s_else)) {
		s_else = NULL;
	}
	
	if (!s_if && !s_else) return false;
	
	if (0 && px_cond_mov_applicable(ctx, cond, s_if, s_else)) {
		// Conditional MOV branch.
		
	} else if (stmt->cond->type == EXPR_TYPE_MATH2 && OP_IS_LOGIC(stmt->cond->oper)) {
		// Logic operator branch.
		asm_label_t l_true  = NULL;
		asm_label_t l_false = NULL;
		asm_label_t l_skip  = NULL;
		
		// Get some labels.
		if (s_if) {
			l_true  = asm_get_label(ctx);
			l_skip  = asm_get_label(ctx);
		}
		if (s_else) {
			l_false = asm_get_label(ctx);
		} else {
			l_false = l_skip;
		}
		
		// Write branches.
		px_logic(ctx, stmt->cond, l_true, l_false, !!s_if);
		
		// Write stataments.
		if (s_if) {
			asm_write_label(ctx, l_true);
			gen_stmt(ctx, s_if, false);
			if (s_else) px_jump(ctx, l_skip);
		}
		if (s_else) {
			asm_write_label(ctx, l_false);
			gen_stmt(ctx, s_else, false);
		}
		
		// Skip label.
		asm_write_label(ctx, l_skip);
		
	} else {
		// Traditional branch.
		if (s_else) {
			// Write the branch.
			char *l_true = asm_get_label(ctx);
			char *l_skip;
			px_branch(ctx, stmt->cond, cond, l_true, NULL);
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
			px_branch(ctx, stmt->cond, cond, NULL, l_skip);
			// True:
			gen_stmt(ctx, s_if, false);
			// Skip label (to skip over if code when codition is not met).
			asm_write_label(ctx, l_skip);
			return false;
		}
	}
}

// While statement implementation.
void gen_while(asm_ctx_t *ctx, stmt_t *stmt, expr_t *cond, stmt_t *code, bool is_do_while) {
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
		px_branch(ctx, stmt->cond, cond_res, loop_label, NULL);
		if (cond_res != &cond_hint) {
			gen_unuse(ctx, cond_res);
		}
	}
}

// For loop implementation.
void gen_for(asm_ctx_t *ctx, stmt_t *stmt, exprs_t *cond, stmt_t *code, exprs_t *next) {
	bool is_forever = cond->num == 0;
	
	char *loop_label  = asm_get_label(ctx);
	char *check_label;
	
	// Fix the stack.
	px_memclobber(ctx, true);
	
	if (!is_forever) {
		check_label = asm_get_label(ctx);
		// Jump to check.
		px_jump(ctx, check_label);
	}
	
	// Loop code.
	asm_write_label(ctx, loop_label);
	gen_stmt(ctx, code, false);
	
	// Fix the stack.
	px_memclobber(ctx, true);
	
	// Perform increment code.
	for (size_t i = 0; i < next->num; i++) {
		gen_var_t *ignore = gen_expression(ctx, &next->arr[i], NULL);
		gen_unuse(ctx, ignore);
	}
	
	// Check condition.
	asm_write_label(ctx, check_label);
	if (is_forever) {
		px_jump(ctx, loop_label);
	} else {
		// Do everything but the last condition, which is the only one that influences looping.
		for (size_t i = 0; i < cond->num - 1; i++) {
			gen_var_t *ignore = gen_expression(ctx, &cond->arr[i], NULL);
			gen_unuse(ctx, ignore);
		}
		
		// Condition check.
		gen_var_t cond_hint = {
			.type = VAR_TYPE_COND,
		};
		gen_var_t *cond_res = gen_expression(ctx, &cond->arr[cond->num - 1], &cond_hint);
		px_branch(ctx, &cond->arr[cond->num - 1], cond_res, loop_label, NULL);
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
		DEBUG_GEN("// Writing call for convention: registers\n");
		
		// Vacate the registers used by the function.
		reg_t regindex = 0;
		for (size_t i = 0; i < n_args; i++) {
			for (reg_t x = 0; x < funcdef->args.arr[i].type->size; x++) {
				px_vacate_reg(ctx, regindex);
				regindex ++;
			}
		}
		
		// Output hints for locations, if any.
		gen_var_t *hints[NUM_REGS];
		// Actual locations of parameters.
		gen_var_t *locations[NUM_REGS];
		
		// Generate parameter values.
		regindex = 0;
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
		DEBUG_GEN("// Writing call for convention: stack\n");
		
		// Actual locations of parameters.
		gen_var_t *locations[n_args];
		
		// Generate parameter values.
		for (size_t i = 0; i < n_args; i++) {
			// Generate the expression.
			gen_var_t *res = gen_expression(ctx, &args[i], NULL);
			if (!res) {
				// Abort.
				return NULL;
			}
			// Save the result for later so that it can be moved to the right stack index.
			locations[i] = res;
		}
		
		// Transfer to actual stack locations.
		address_t child_stack = 0;
		for (size_t i = 0; i < n_args; i++) {
			
		}
	}
	
	DEBUG_GEN("// Jumping to subroutine %s\n", funcdef->ident.strval);
	
	// Jump to the subroutine.
	switch (var->type) {
		
		case (VAR_TYPE_CONST): {
			// Jump to constant.
			px_insn_t insn = {
				.y = true,
				.x = PX_ADDR_IMM,
				.b = PX_REG_IMM,
				.a = PX_REG_PC,
				.o = PX_OFFS_MOV | COND_JSR,
			};
			px_write_insn(ctx, insn, NULL, 0, NULL, var->iconst);
		} break;
		case (VAR_TYPE_LABEL): {
			if (DET_PIE(ctx)) {
				// Jump to label (PIE).
				px_insn_t insn = {
					.y = true,
					.x = PX_ADDR_PC,
					.b = PX_REG_IMM,
					.a = PX_REG_PC,
					.o = PX_OFFS_LEA | COND_JSR,
				};
				px_write_insn(ctx, insn, NULL, 0, var->label, 0);
			} else {
				// Jump to label (Non-PIE).
				px_insn_t insn = {
					.y = true,
					.x = PX_ADDR_IMM,
					.b = PX_REG_IMM,
					.a = PX_REG_PC,
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
				.x = PX_ADDR_IMM,
				.b = var->reg,
				.a = PX_REG_PC,
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
		gen_var_t *retval = xalloc(ctx->current_scope->allocator, sizeof(gen_var_t));
		if (funcdef->returns->size <= NUM_REGS) {
			// Registrex.
			*retval = (gen_var_t) {
				.type  = VAR_TYPE_REG,
				.reg   = PX_REG_R0,
				.ctype = funcdef->returns,
				.owner = NULL
			};
			
			// Mark REGISTER USAGE.
			for (reg_t i = 0; i < funcdef->returns->size; i++) {
				ctx->current_scope->reg_usage[i] = retval;
			}
			
		} else {
			// TODO!
			*retval = (gen_var_t) {
				.type        = VAR_TYPE_VOID,
				.ctype       = ctype_simple(ctx, STYPE_VOID),
				.owner       = NULL,
				.default_loc = NULL,
			};
		}
		
		return retval;
		
	} else {
		// Make a return void.
		gen_var_t dummy  = {
			.type        = VAR_TYPE_VOID,
			.ctype       = ctype_simple(ctx, STYPE_VOID),
			.owner       = NULL,
			.default_loc = NULL,
		};
		return XCOPY(ctx->allocator, &dummy, gen_var_t);
	}
}

// Writes logical AND/OR code for jumping to labels.
// Flow type indicates what condition happens on flow through.
void px_logic(asm_ctx_t *ctx, expr_t *expr, asm_label_t l_true, asm_label_t l_false, bool flow_type) {
	if (expr->type == EXPR_TYPE_MATH2 && expr->oper == OP_LOGIC_AND) {
		// AND code.
		// Evaluate A.
		gen_var_t cond_hint = { .type = VAR_TYPE_COND };
		gen_var_t *a = gen_expression(ctx, expr->par_a, &cond_hint);
		// Branch A.
		px_branch(ctx, expr->par_a, a, NULL, l_false);
		
		// Evaluate B.
		gen_var_t *b = gen_expression(ctx, expr->par_b, &cond_hint);
		// Eliminate unnecessary branches.
		if (flow_type) l_true = NULL;
		else l_false = NULL;
		// Branch B.
		px_branch(ctx, expr->par_b, b, l_true, l_false);
		
	} else if (expr->type == EXPR_TYPE_MATH2 && expr->oper == OP_LOGIC_OR) {
		// OR code.
		// Evaluate A.
		gen_var_t cond_hint = { .type = VAR_TYPE_COND };
		gen_var_t *a = gen_expression(ctx, expr->par_a, &cond_hint);
		// Branch A.
		px_branch(ctx, expr->par_a, a, l_true, NULL);
		
		// Evaluate B.
		gen_var_t *b = gen_expression(ctx, expr->par_b, &cond_hint);
		// Eliminate unnecessary branches.
		if (flow_type) l_true = NULL;
		else l_false = NULL;
		// Branch B.
		px_branch(ctx, expr->par_b, b, l_true, l_false);
		
	} else if (expr->type == EXPR_TYPE_MATH1 && expr->oper == OP_LOGIC_NOT) {
		// NOT code.
		// Invert labels.
		px_logic(ctx, expr->par_a, l_false, l_true, !flow_type);
		
	} else if (expr->type == EXPR_TYPE_CONST) {
		// Constant expression.
		// Write jump.
		px_jump(ctx, expr->iconst ? l_true : l_false);
		
	} else {
		// Other expression.
		gen_var_t cond_hint = { .type = VAR_TYPE_COND };
		gen_var_t *a = gen_expression(ctx, expr, &cond_hint);
		
		// Eliminate unnecessary branches.
		if (flow_type) l_true = NULL;
		else l_false = NULL;
		// Write branch.
		px_branch(ctx, expr, a, l_true, l_false);
	}
}

// Expression: Logical operation.
gen_var_t *gen_expr_logic2(asm_ctx_t *ctx, expr_t *expr, gen_var_t *out_hint) {
	gen_var_t *output = out_hint;
	if (!output) {
		output        = px_get_tmp(ctx, 1, true);
		output->ctype = ctype_simple(ctx, STYPE_BOOL);
	}
	
	// Create labels for true and false outcomes.
	asm_label_t l_true  = asm_get_label(ctx);
	asm_label_t l_false = asm_get_label(ctx);
	asm_label_t l_skip  = asm_get_label(ctx);
	
	// Use the internal device.
	px_logic(ctx, expr, l_true, l_false, true);
	
	// Write setter for true outcome.
	asm_write_label(ctx, l_true);
	px_insn_t insn = {
		.y = 0,
		.x = PX_ADDR_IMM,
		.b = PX_REG_IMM,
		.o = PX_OP_MOV,
	};
	px_write_insn(ctx, insn, NULL, 0, NULL, 1);
	px_jump(ctx, l_skip);
	
	// Write setter for false outcome.
	asm_write_label(ctx, l_false);
	px_write_insn(ctx, insn, NULL, 0, NULL, 0);
	asm_write_label(ctx, l_skip);
	
	return output;
}

// Expression: Binary math operation.
gen_var_t *gen_expr_math2(asm_ctx_t *ctx, expr_t *expr, oper_t oper, gen_var_t *out_hint, gen_var_t *a, gen_var_t *b) {
	address_t n_words = 1;
	bool isSigned     = STYPE_IS_SIGNED(a->ctype->simple_type) && STYPE_IS_SIGNED(b->ctype->simple_type);
	
	// Check for unassigned.
	if (a->type == VAR_TYPE_UNASSIGNED) {
		// Emit a warning i guess.
		if (expr && expr->par_a->type == EXPR_TYPE_IDENT) {
			report_errorf(ctx->tokeniser_ctx, E_WARN, expr->par_a->pos, "%s is uninitialised at this point", expr->par_a->ident->strval);
		} else {
			report_errorf(ctx->tokeniser_ctx, E_WARN, expr->pos, "<anonymous variable> is uninitialised at this point\n");
		}
		*a = *a->default_loc;
	}
	if (b->type == VAR_TYPE_UNASSIGNED) {
		// Emit a warning i guess.
		if (expr && expr->par_b->type == EXPR_TYPE_IDENT) {
			report_errorf(ctx->tokeniser_ctx, E_WARN, expr->par_b->pos, "%s is uninitialised at this point", expr->par_b->ident->strval);
		} else {
			report_errorf(ctx->tokeniser_ctx, E_WARN, expr->pos, "<anonymous variable> is uninitialised at this point\n");
		}
		*b = *b->default_loc;
	}
	
	
	// Can we simplify?
	if ((OP_IS_SHIFT(oper) || OP_IS_ADD(oper) || OP_IS_COMP(oper)) && b->type == VAR_TYPE_CONST && b->iconst == 1) {
		// B is constant 1: math1 will do.
		return gen_expr_math1(ctx, expr, oper, out_hint, a);
		
	} else if ((oper == OP_EQ || oper == OP_NE) && b->type == VAR_TYPE_CONST && b->iconst == 0) {
		// B is constant 0 (for == or !=), math1 will do.
		gen_var_t *output = out_hint;
		if (!output || output->type != VAR_TYPE_COND) {
			// Make a new output.
			output  = xalloc(ctx->allocator, sizeof(gen_var_t));
			*output = (gen_var_t) {
				.type        = VAR_TYPE_COND,
				.ctype       = ctype_simple(ctx, STYPE_BOOL),
				.owner       = NULL,
				.default_loc = NULL,
			};
		}
		px_math1(ctx, PX_OP_CMP1, output, a);
		// Translate comparison operators.
		switch (oper) {
			case OP_EQ:
				output->cond = COND_ULT;
				break;
			case OP_NE:
				output->cond = COND_UGE;
				break;
		}
		return output;
	}
	
	if (oper == OP_INDEX) {
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
		// A comparison.
		gen_var_t *ignored = px_math2(ctx, PX_OP_CMP, NULL, a, b);
		gen_unuse(ctx, ignored);
		
		// Give back a condition representation.
		gen_var_t *cond = xalloc(ctx->current_scope->allocator, sizeof(gen_var_t));
		*cond = (gen_var_t) {
			.type        = VAR_TYPE_COND,
			.ctype       = ctype_simple(ctx, STYPE_BOOL),
			.owner       = NULL,
			.default_loc = NULL,
		};
		
		// Determine interpretation of flags.
		switch (oper) {
			case OP_GT: cond->cond = isSigned ? COND_SGT : COND_UGT; break;
			case OP_GE: cond->cond = isSigned ? COND_SGE : COND_UGE; break;
			case OP_LT: cond->cond = isSigned ? COND_SLT : COND_ULT; break;
			case OP_LE: cond->cond = isSigned ? COND_SLE : COND_ULE; break;
			case OP_EQ: cond->cond = COND_EQ; break;
			case OP_NE: cond->cond = COND_NE; break;
		}
		
		return cond;

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
gen_var_t *gen_expr_math1(asm_ctx_t *ctx, expr_t *expr, oper_t oper, gen_var_t *output, gen_var_t *a) {
	bool isSigned     = true;
	
	// Check for unassigned.
	if (a->type == VAR_TYPE_UNASSIGNED) {
		// Emit a warning for uninitialised variables.
		if (expr && expr->par_a->type == EXPR_TYPE_IDENT) {
			report_errorf(ctx->tokeniser_ctx, E_WARN, expr->par_a->pos, "%s is uninitialised at this point", expr->par_a->ident->strval);
		} else if (expr->type == EXPR_TYPE_IDENT) {
			report_errorf(ctx->tokeniser_ctx, E_WARN, expr->pos, "%s is uninitialised at this point", expr->ident->strval);
		} else {
			report_errorf(ctx->tokeniser_ctx, E_WARN, expr->pos, "<unknown variable> is uninitialised at this point\n");
		}
		*a = *a->default_loc;
	}
	
	if (oper == OP_POST_INC || oper == OP_POST_DEC) {
		// Create a temp variable with a copy.
		gen_var_t *temp;
		if (output) {
			temp = output;
		} else {
			temp = px_get_tmp(ctx, a->ctype->size, true);
			temp->ctype = a->ctype;
		}
		
		// Copy current value to temporary.
		gen_mov(ctx, temp, a);
		
		// Perform math operation.
		gen_var_t *ignored = px_math1(ctx, oper == OP_POST_INC ? PX_OP_INC : PX_OP_DEC, a, a);
		
		// Return copied value.
		return temp;
		
	} else if (oper == OP_LOGIC_NOT) {
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
				.ctype       = ctype_simple(ctx, STYPE_BOOL),
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
			return gen_expr_math1(ctx, expr, oper, output, a->default_loc);
		} else if (a->type == VAR_TYPE_LABEL) {
			bool use_hint = output && output->type == VAR_TYPE_REG;
			reg_t regno = use_hint ? output->reg : px_pick_reg(ctx, true);
			ctx->reg_temp_usage[regno] = true;
			if (DET_PIE(ctx)) {
				// LEA (pie).
				px_insn_t insn = {
					.y = true,
					.x = PX_ADDR_PC,
					.b = PX_REG_IMM,
					.a = regno,
					.o = PX_OP_LEA,
				};
				px_write_insn(ctx, insn, NULL, 0, a->label, 0);
			} else {
				// LEA (non-pie).
				px_insn_t insn = {
					.y = true,
					.x = PX_ADDR_MEM,
					.b = PX_REG_IMM,
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
			ctx->reg_temp_usage[regno] = false;
			return var;
		} else if (a->type == VAR_TYPE_STACKOFFS) {
			bool use_hint = output && output->type == VAR_TYPE_REG;
			// LEA (stack).
			reg_t regno = use_hint ? output->reg : px_pick_reg(ctx, true);
			px_insn_t insn = {
				.y = true,
				.x = PX_ADDR_ST,
				.b = PX_REG_IMM,
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
			return gen_expr_math1(ctx, expr, oper, output, tmp);
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
				ctx->reg_temp_usage[regno] = true;
				
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
				
				ctx->reg_temp_usage[regno] = false;
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
				.x = PX_ADDR_IMM,
				.b = 0,
				.a = PX_REG_ST,
				.o = diff == 1 ? PX_OP_INC : PX_OP_DEC,
			};
			px_write_insn_raw(ctx, insn, NULL, 0, NULL, 0);
		} else if (diff > 1) {
			// Add to ST.
			px_insn_t insn = {
				.y = 0,
				.x = PX_ADDR_IMM,
				.b = PX_REG_IMM,
				.a = PX_REG_ST,
				.o = PX_OP_ADD,
			};
			px_write_insn_raw(ctx, insn, NULL, 0, NULL, diff);
		} else if (diff < -1) {
			// Sub from ST for clarity.
			px_insn_t insn = {
				.y = 0,
				.x = PX_ADDR_IMM,
				.b = PX_REG_IMM,
				.a = PX_REG_ST,
				.o = PX_OP_SUB,
			};
			px_write_insn_raw(ctx, insn, NULL, 0, NULL, -diff);
		}
	}
}

// Variables: Move variable to another location.
void px_mov_n(asm_ctx_t *ctx, gen_var_t *dst, gen_var_t *src, address_t n_words) {
	if (gen_cmp(ctx, dst, src)) return;
	
	// Translate retval into it's real location.
	if (dst->type == VAR_TYPE_RETVAL) {
		dst->type = VAR_TYPE_REG;
		dst->reg  = PX_REG_R0;
	}
	
	if (dst->type == VAR_TYPE_REG) {
		// To register move.
		for (address_t i = 0; i < n_words; i++) {
			px_part_to_reg(ctx, src, dst->reg + i, i);
		}
	} else if (dst->type == VAR_TYPE_COND) {
		// Convert to condition.
		dst->cond = px_var_to_cond(ctx, NULL, src);
	} else {
		// Move through register.
		reg_t regno;
		bool  mov_to_reg = false;
		if (src->type == VAR_TYPE_CONST) {
			// We can encode this differently.
			regno = PX_REG_IMM;
		} else if (src->type == VAR_TYPE_REG) {
			// Use the existing register.
			regno = src->reg;
		} else {
			// Pick a new register.
			regno = px_pick_reg(ctx, true);
			px_touch_reg(ctx, regno);
			mov_to_reg = true;
		}
		ctx->reg_temp_usage[regno] = true;
		
		bool revloop = dst->type == VAR_TYPE_STACKOFFS;
		address_t i   = revloop ? n_words - 1 : 0;
		address_t lim = revloop ? -1          : n_words;
		address_t di  = revloop ? -1          : +1;
		for (; i != lim; i += di) {
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
			} else if (insn.x == PX_ADDR_IMM) {
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
		
		ctx->reg_temp_usage[regno] = false;
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
		gen_mov(ctx, var, orig);
		
		// Clean up.
		if (orig->default_loc) {
			xfree(ctx->current_scope->allocator, orig);
		}
	}
}

// Variables: Move variable to another location.
void gen_mov(asm_ctx_t *ctx, gen_var_t *dst, gen_var_t *src) {
	address_t n_words;
	/*if (dst->type == VAR_TYPE_UNASSIGNED && !src->owner) {
		// Unassigned variable optimisation.
		// Make default location.
		src->default_loc = dst->default_loc;
		// Swap src and dst.
		gen_var_t tmp = *dst;
		*dst = *src;
		*src = tmp;
		return;
	} else*/ if (dst->type == VAR_TYPE_UNASSIGNED) {
		// Replace dst by it's default location before copying.
		gen_var_t *to_free = dst->default_loc;
		*dst = *dst->default_loc;
		// Free up old memory.
		xfree(ctx->allocator, to_free);
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

// Variables: Populate the value from initialiser expression.
void gen_init_var(asm_ctx_t *ctx, gen_var_t *var, expr_t *expr) {
	gen_var_t *res = gen_expression(ctx, expr, var);
	if (!gen_cmp(ctx, var, res)) {
		if (var->type != VAR_TYPE_UNASSIGNED) {
			// Have it moved.
			gen_mov(ctx, var, res);
		} else {
			reg_t regno;
			if (px_pick_empty_reg(ctx, &regno, var->ctype->size)) {
				// Have it moved to a register for Exxtra Speeds.
				var->type = VAR_TYPE_REG;
				var->reg  = regno;
				for (reg_t i = 0; i < var->ctype->size; i++) {
					ctx->current_scope->reg_usage[regno + i] = var;
					px_touch_reg(ctx, regno + i);
				}
				gen_mov(ctx, var, res);
			} else {
				// Have it moved to the default location.
				*var = *var->default_loc;
				gen_mov(ctx, var, res);
			}
		}
	}
}
