
#include "parser-util.h"
#include "gen_util.h"
#include <malloc.h>

// Incomplete function definition (without code).
funcdef_t funcdef_def(parser_ctx_t *ctx, ival_t *type, strval_t *ident, idents_t *args) {
	return (funcdef_t) {
		.ident   = *ident,
		.args    = *args,
		.stmts   = NULL,
		.returns = ctype_simple(ctx->asm_ctx, type->ival),
	};
}

// Complete function declaration (with code).
funcdef_t funcdef_decl(parser_ctx_t *ctx, ival_t *type, strval_t *ident, idents_t *args, stmts_t *code) {
	return (funcdef_t) {
		.ident   = *ident,
		.args    = *args,
		.stmts   = code,
		.returns = ctype_simple(ctx->asm_ctx, type->ival),
	};
}


// An empty list of statements.
stmts_t stmts_empty(parser_ctx_t *ctx) {
	return (stmts_t) {
		.arr = NULL,
		.num = 0,
	};
}

// Concatenate to a list of statements.
stmts_t stmts_cat(parser_ctx_t *ctx, stmts_t *stmts, stmt_t *stmt) {
	stmts->num ++;
	stmts->arr = xrealloc(ctx->allocator, stmts->arr, stmts->num * sizeof(stmt_t));
	stmts->arr[stmts->num - 1] = *stmt;
	return *stmts;
}



// An emppty statement that does nothing.
stmt_t stmt_empty(parser_ctx_t *ctx) {
	return (stmt_t) {
		.type  = STMT_TYPE_EMPTY,
	};
}

// Statements contained in curly brackets.
stmt_t stmt_multi(parser_ctx_t *ctx, stmts_t *stmts) {
	return (stmt_t) {
		.type  = STMT_TYPE_MULTI,
		.stmts = XCOPY(ctx->allocator, stmts, stmts_t),
	};
}

// If-else statements.
stmt_t stmt_if(parser_ctx_t *ctx, expr_t *cond, stmt_t *s_if, stmt_t *s_else) {
	return (stmt_t) {
		.type       = STMT_TYPE_IF,
		.cond       = XCOPY(ctx->allocator, cond,   expr_t),
		.code_true  = XCOPY(ctx->allocator, s_if,   stmt_t),
		.code_false = XCOPY(ctx->allocator, s_else, stmt_t)
	};
}

// While loops.
stmt_t stmt_while(parser_ctx_t *ctx, expr_t *cond, stmt_t *code) {
	return (stmt_t) {
		.type       = STMT_TYPE_WHILE,
		.cond       = XCOPY(ctx->allocator, cond,   expr_t),
		.code_true  = XCOPY(ctx->allocator, code,   stmt_t)
	};
}

// For loops.
stmt_t stmt_for(parser_ctx_t *ctx, stmt_t *initial, exprs_t *cond, exprs_t *after, stmt_t *code) {
	return (stmt_t) {
		.type     = STMT_TYPE_FOR,
		.for_init = XCOPY(ctx->allocator, initial, stmt_t),
		.for_cond = XCOPY(ctx->allocator, cond,    exprs_t),
		.for_next = XCOPY(ctx->allocator, after,   exprs_t),
		.for_code = XCOPY(ctx->allocator, code,    stmt_t),
	};
}

// Return statements.
stmt_t stmt_ret(parser_ctx_t *ctx, expr_t *expr) {
	return (stmt_t) {
		.type  = STMT_TYPE_RET,
		.expr  = XCOPY(ctx->allocator, expr, expr_t)
	};
}

// Variable declaration statements.
stmt_t stmt_var(parser_ctx_t *ctx, idents_t *decls) {
	return (stmt_t) {
		.type  = STMT_TYPE_VAR,
		.vars  = XCOPY(ctx->allocator, decls, idents_t)
	};
}

// Expression statements (most code).
stmt_t stmt_expr(parser_ctx_t *ctx, expr_t *expr) {
	return (stmt_t) {
		.type  = STMT_TYPE_EXPR,
		.expr  = XCOPY(ctx->allocator, expr, expr_t)
	};
}

// Assembly statements (archtitecture-specific assembly code).
stmt_t stmt_iasm(parser_ctx_t *ctx, strval_t *text, iasm_regs_t *outputs, iasm_regs_t *inputs, void *clobbers) {
	iasm_t *iasm = (iasm_t *) xalloc(ctx->allocator, sizeof(iasm_t));
	if (outputs) {
		outputs = XCOPY(ctx->allocator, outputs,  iasm_regs_t);
	} else {
		iasm_regs_t dummy = iasm_regs_empty(ctx);
		outputs = XCOPY(ctx->allocator, &dummy,  iasm_regs_t);
	}
	if (inputs) {
		inputs = XCOPY(ctx->allocator, inputs,  iasm_regs_t);
	} else {
		iasm_regs_t dummy = iasm_regs_empty(ctx);
		inputs = XCOPY(ctx->allocator, &dummy,  iasm_regs_t);
	}
	*iasm = (iasm_t) {
		.text     = *text,
		.outputs  = outputs,
		.inputs   = inputs,
		// .clobbers = XCOPY(ctx->allocator, clobbers, lolwutidownknow)
	};
	return (stmt_t) {
		.type  = STMT_TYPE_IASM,
		.iasm  = iasm
	};
}

// Check whether a statement is effectively an empty statement.
bool stmt_is_empty(stmt_t *stmt) {
	if (stmt->type == STMT_TYPE_EMPTY) {
		return true;
	} else if (stmt->type == STMT_TYPE_MULTI) {
		for (size_t i = 0; i < stmt->stmts->num; i++) {
			if (!stmt_is_empty(&stmt->stmts->arr[i])) return false;
		}
		return true;
	} else {
		return false;
	}
}



// Assembly parameter definition.
iasm_reg_t stmt_iasm_reg(parser_ctx_t *ctx, strval_t *symbol, strval_t *mode, expr_t *expr) {
	return (iasm_reg_t) {
		.symbol = symbol ? symbol->strval : NULL,
		.mode   = mode->strval,
		.expr   = expr
	};
}

// An empty list of iasm_reg.
iasm_regs_t iasm_regs_empty(parser_ctx_t *ctx) {
	return (iasm_regs_t) {
		.arr = NULL,
		.num = 0
	};
}

// Concatenate to a list of iasm_reg.
iasm_regs_t iasm_regs_cat(parser_ctx_t *ctx, iasm_regs_t *iasm_regs, iasm_reg_t *iasm_reg) {
	iasm_regs->num ++;
	iasm_regs->arr = xrealloc(ctx->allocator, iasm_regs->arr, iasm_regs->num * sizeof(ident_t));
	iasm_regs->arr[iasm_regs->num - 1] = *iasm_reg;
	return *iasm_regs;
}

// A list of one iasm_reg.
iasm_regs_t iasm_regs_one(parser_ctx_t *ctx, iasm_reg_t *iasm_reg) {
	return (iasm_regs_t) {
		.arr = XCOPY(ctx->allocator, iasm_reg, iasm_reg_t),
		.num = 1
	};
}



// Creates an empty ident_t from a strval.
ident_t ident_of_strval(parser_ctx_t *ctx, strval_t *strval) {
	return (ident_t) {
		.pos         = strval->pos,
		.strval      = strval->strval,
		.type        = NULL,
		.initialiser = NULL,
	};
}



// An empty list of identities.
idents_t idents_empty(parser_ctx_t *ctx) {
	return (idents_t) {
		.arr = NULL,
		.num = 0
	};
}

// Concatenate to a list of identities.
idents_t idents_cat(parser_ctx_t *ctx, idents_t *idents, int *s_type_ptr, strval_t *name, expr_t *init) {
	simple_type_t s_type;
	if (!s_type_ptr) {
		s_type = ctx->s_type;
	} else {
		s_type = *s_type_ptr;
	}
	
	idents->num ++;
	idents->arr = xrealloc(ctx->allocator, idents->arr, idents->num * sizeof(ident_t));
	idents->arr[idents->num - 1] = (ident_t) {
		.pos         = name->pos,
		.strval      = name->strval,
		.type        = ctype_simple(ctx->asm_ctx, s_type),
		.initialiser = init ? XCOPY(ctx->allocator, init, expr_t) : NULL,
	};
	return *idents;
}

// A list of one identity.
idents_t idents_one(parser_ctx_t *ctx, int *s_type_ptr, strval_t *name, expr_t *init) {
	simple_type_t s_type;
	if (!s_type_ptr) {
		s_type = ctx->s_type;
	} else {
		s_type = *s_type_ptr;
	}
	
	ident_t ident = {
		.pos         = name->pos,
		.strval      = name->strval,
		.type        = ctype_simple(ctx->asm_ctx, s_type),
		.initialiser = init ? XCOPY(ctx->allocator, init, expr_t) : NULL,
	};
	return (idents_t) {
		.arr = XCOPY(ctx->allocator, &ident, ident_t),
		.num = 1
	};
}

// Concatenate to a list of identities (using existing ident_t).
idents_t idents_cat_ex(parser_ctx_t *ctx, idents_t *idents, ident_t *ident, expr_t *init) {
	ident->initialiser = init ? XCOPY(ctx->allocator, init, expr_t) : NULL;
	idents->num ++;
	idents->arr = xrealloc(ctx->allocator, idents->arr, idents->num * sizeof(ident_t));
	idents->arr[idents->num - 1] = *ident;
	return *idents;
}

// A list of one identity (using existing ident_t).
idents_t idents_one_ex(parser_ctx_t *ctx, ident_t *ident, expr_t *init) {
	ident->initialiser = init ? XCOPY(ctx->allocator, init, expr_t) : NULL;
	return (idents_t) {
		.arr = XCOPY(ctx->allocator, ident, ident_t),
		.num = 1
	};
}



// An empty list of expressions.
exprs_t exprs_empty(parser_ctx_t *ctx) {
	return (exprs_t) {
		.arr = NULL,
		.num = 0
	};
}

// A list of one expression.
exprs_t exprs_one(parser_ctx_t *ctx, expr_t *expr) {
	return (exprs_t) {
		.arr = XCOPY(ctx->allocator, expr, expr_t),
		.num = 1
	};
}

// Concatenate to a list of expressions.
exprs_t exprs_cat(parser_ctx_t *ctx, exprs_t *exprs, expr_t *expr) {
	exprs->num ++;
	exprs->arr = xrealloc(ctx->allocator, exprs->arr, exprs->num * sizeof(expr_t));
	exprs->arr[exprs->num - 1] = *expr;
	return *exprs;
}


// Numeric constant expression.
expr_t expr_icnst(parser_ctx_t *ctx, ival_t *val) {
	return (expr_t) {
		.type     = EXPR_TYPE_CONST,
		.iconst   = val->ival
	};
}

// String constant expression.
expr_t expr_scnst(parser_ctx_t *ctx, strval_t *val) {
	// Get a label for this string.
	asm_label_t label = xalloc(ctx->tokeniser_ctx->allocator, 64);
	sprintf(label, "__const%zu", ctx->n_const++);
	
	// Write the string into .rodata.
	char *old_id = xstrdup(ctx->allocator, ctx->asm_ctx->current_section_id);
	asm_use_sect(ctx->asm_ctx, ".rodata", ASM_NOT_ALIGNED);
	asm_write_label(ctx->asm_ctx, label);
	for (char *str = val->strval; *str; str++) {
		asm_write_memword(ctx->asm_ctx, *str);
	}
	asm_write_memword(ctx->asm_ctx, 0);
	char *temp = esc_cstr(ctx->allocator, val->strval, strlen(val->strval));
	DEBUG_GEN("  .db \"%s\", 0\n", temp);
	xfree(ctx->allocator, temp);
	
	// Switch back to old section.
	asm_use_sect(ctx->asm_ctx, old_id, ASM_NOT_ALIGNED);
	xfree(ctx->allocator, old_id);
	
	// Package the assembly reference back up.
	return (expr_t) {
		.type  = EXPR_TYPE_CSTR,
		.label = label,
	};
}

// Identity expression (things like variables and functions).
expr_t expr_ident(parser_ctx_t *ctx, strval_t *ident) {
	return (expr_t) {
		.type     = EXPR_TYPE_IDENT,
		.ident    = XCOPY(ctx->allocator, ident, strval_t)
	};
}

// Unary math expression non-additive (things like &a, *b and !c).
expr_t expr_math1(parser_ctx_t *ctx, oper_t type, expr_t *val) {
	if (val->type == EXPR_TYPE_CONST) {
		// Optimise out numbers.
		switch (type) {
			case OP_0_MINUS:
				val->iconst = -val->iconst;
				break;
			case OP_BIT_NOT:
				val->iconst = ~val->iconst;
				break;
			case OP_LOGIC_NOT:
				val->iconst = !val->iconst;
				break;
			case OP_ADROF:
				// This isn't acceptable.
			case OP_DEREF:
				// This can't be simplified.
				goto the_usual;
		}
		return *val;
	}
	the_usual:
	return (expr_t) {
		.type     = EXPR_TYPE_MATH1,
		.oper     = type,
		.par_a    = XCOPY(ctx->allocator, val, expr_t)
	};
}

// Unary math expression additive (things like ++a and --b).
expr_t expr_math1a(parser_ctx_t *ctx, oper_t type, expr_t *val) {
	if (val->type == EXPR_TYPE_CONST) {
		// Optimise out numbers.
		switch (type) {
			case OP_ADD:
				val->iconst = val->iconst + 1;
				break;
			case OP_SUB:
				val->iconst = val->iconst - 1;
				break;
			default:
				goto the_usual;
		}
		return *val;
	}
	
	expr_t *one;
	the_usual:
	// This is quite simple: val = val operator 1.
	one = xalloc(ctx->allocator, sizeof(expr_t));
	*one = (expr_t) {
		.type = EXPR_TYPE_CONST,
		.iconst = 1,
	};
	expr_t param_b = expr_math2(ctx, type, val, one);
	return expr_math2(ctx, OP_ASSIGN, val, &param_b);
}

// Function call expression.
expr_t expr_call(parser_ctx_t *ctx, expr_t *func, exprs_t *args) {
	return (expr_t) {
		.type     = EXPR_TYPE_CALL,
		.func     = XCOPY(ctx->allocator, func, expr_t),
		.args     = XCOPY(ctx->allocator, args, exprs_t)
	};
}

// Binary math expression (things like a + b, c = d and e[f]).
expr_t expr_math2(parser_ctx_t *ctx, oper_t type, expr_t *val1, expr_t *val2) {
	if (val1->type == EXPR_TYPE_CONST && val2->type == EXPR_TYPE_CONST) {
		// Optimise out numbers.
		address_t a = val1->iconst;
		address_t b = val2->iconst;
		address_t o;
		switch (type) {
			case OP_ADD:
				o = a + b;
				break;
			case OP_SUB:
				o = a - b;
				break;
			case OP_MUL:
				o = a * b;
				break;
			case OP_DIV:
				o = a / b;
				break;
			case OP_MOD:
				o = a % b;
				break;
			case OP_BIT_AND:
				o = a & b;
				break;
			case OP_BIT_OR:
				o = a | b;
				break;
			case OP_BIT_XOR:
				o = a ^ b;
				break;
			case OP_LOGIC_AND:
				o = a && b;
				break;
			case OP_LOGIC_OR:
				o = a || b;
				break;
			case OP_SHIFT_L:
				o = a << b;
				break;
			case OP_SHIFT_R:
				o = a >> b;
				break;
			case OP_EQ:
				o = a == b;
				break;
			case OP_NE:
				o = a != b;
				break;
			case OP_LE:
				o = a <= b;
				break;
			case OP_GE:
				o = a >= b;
				break;
			case OP_LT:
				o = a < b;
				break;
			case OP_GT:
				o = a > b;
				break;
		}
		return (expr_t) {
			.type   = EXPR_TYPE_CONST,
			.iconst = o
		};
	}
	return (expr_t) {
		.type     = EXPR_TYPE_MATH2,
		.oper     = type,
		.par_a    = XCOPY(ctx->allocator, val1, expr_t),
		.par_b    = XCOPY(ctx->allocator, val2, expr_t)
	};
}

// Assignment math expression (things like a += b, c *= d and e |= f).
// Generalises to a combination of an assignment and expr_math2.
expr_t expr_math2a(parser_ctx_t *ctx, oper_t type, expr_t *val1, expr_t *val2) {
	// This is quite simple: val1 = val1 operator val2.
	expr_t param_b = expr_math2(ctx, type, val1, val2);
	return expr_math2(ctx, OP_ASSIGN, val1, &param_b);
}
