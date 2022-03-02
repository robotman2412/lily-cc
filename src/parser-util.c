
#include "parser-util.h"
#include <malloc.h>

// Incomplete function definition (without code).
funcdef_t funcdef_def (strval_t *ident, idents_t *args) {
	return (funcdef_t) {
		.ident = *ident,
		.args  = *args,
		.stmts = NULL
	};
}

// Complete function declaration (with code).
funcdef_t funcdef_decl(strval_t *ident, idents_t *args, stmts_t *code) {
	return (funcdef_t) {
		.ident = *ident,
		.args  = *args,
		.stmts = code
	};
}



// An empty list of statements.
stmts_t stmts_empty() {
	return (stmts_t) {
		.arr = NULL,
		.num = 0,
	};
}

// Concatenate to a list of statements.
stmts_t stmts_cat(stmts_t *stmts, stmt_t *stmt) {
	stmts->num ++;
	stmts->arr = realloc(stmts->arr, stmts->num * sizeof(stmt_t));
	stmts->arr[stmts->num - 1] = *stmt;
	return *stmts;
}



// Statements contained in curly brackets.
stmt_t stmt_multi(stmts_t *stmts) {
	return (stmt_t) {
		.type  = STMT_TYPE_MULTI,
		.stmts = COPY(stmts, stmts_t)
	};
}

// If-else statements.
stmt_t stmt_if(expr_t *cond, stmt_t *s_if, stmt_t *s_else) {
	return (stmt_t) {
		.type       = STMT_TYPE_IF,
		.cond       = COPY(cond,   expr_t),
		.code_true  = COPY(s_if,   stmt_t),
		.code_false = COPY(s_else, stmt_t)
	};
}

// While loops.
stmt_t stmt_while(expr_t *cond, stmt_t *code) {
	return (stmt_t) {
		.type       = STMT_TYPE_WHILE,
		.cond       = COPY(cond,   expr_t),
		.code_true  = COPY(code,   stmt_t)
	};
}

// Return statements.
stmt_t stmt_ret(expr_t *expr) {
	return (stmt_t) {
		.type  = STMT_TYPE_RET,
		.expr  = COPY(expr, expr_t)
	};
}

// Variable declaration statements.
stmt_t stmt_var(idents_t *decls) {
	return (stmt_t) {
		.type  = STMT_TYPE_VAR,
		.vars  = COPY(decls, idents_t)
	};
}

// Expression statements (most code).
stmt_t stmt_expr(expr_t *expr) {
	return (stmt_t) {
		.type  = STMT_TYPE_EXPR,
		.expr  = COPY(expr, expr_t)
	};
}

// Assembly statements (archtitecture-specific assembly code).
stmt_t stmt_iasm(strval_t *text, iasm_regs_t *outputs, iasm_regs_t *inputs, void *clobbers) {
	iasm_t *iasm = (iasm_t *) malloc(sizeof(iasm_t));
	*iasm = (iasm_t) {
		.text     = *text,
		.outputs  = COPY(outputs,  iasm_regs_t),
		.inputs   = COPY(inputs,   iasm_regs_t),
		// .clobbers = COPY(clobbers, lolwutidownknow)
	};
	return (stmt_t) {
		.type  = STMT_TYPE_IASM,
		.iasm  = iasm
	};
}



// Assembly parameter definition.
iasm_reg_t stmt_iasm_reg(strval_t *symbol, strval_t *mode, expr_t *expr) {
	return (iasm_reg_t) {
		.symbol = symbol ? symbol->strval : NULL,
		.mode   = mode->strval,
		.expr   = expr
	};
}

// An empty list of iasm_reg.
iasm_regs_t iasm_regs_empty() {
	return (iasm_regs_t) {
		.arr = NULL,
		.num = 0
	};
}

// Concatenate to a list of iasm_reg.
iasm_regs_t iasm_regs_cat(iasm_regs_t *iasm_regs, iasm_reg_t *iasm_reg) {
	iasm_regs->num ++;
	iasm_regs->arr = realloc(iasm_regs->arr, iasm_regs->num * sizeof(ident_t));
	iasm_regs->arr[iasm_regs->num - 1] = *iasm_reg;
	return *iasm_regs;
}

// A list of one iasm_reg.
iasm_regs_t iasm_regs_one(iasm_reg_t *iasm_reg) {
	return (iasm_regs_t) {
		.arr = COPY(iasm_reg, iasm_reg_t),
		.num = 1
	};
}



// An empty list of identities.
idents_t idents_empty() {
	return (idents_t) {
		.arr = NULL,
		.num = 0
	};
}

// Concatenate to a list of identities.
idents_t idents_cat(idents_t *idents, simple_type_t *s_type, strval_t *name) {
	idents->num ++;
	idents->arr = realloc(idents->arr, idents->num * sizeof(ident_t));
	var_type_t type;
	if (s_type) {
		type = (var_type_t) {
			.category    = TYPE_CAT_SIMPLE,
			.is_complete = true,
			.simple_type = *s_type,
			.size        = CSIZE_SIMPLE(*s_type)
		};
	}
	idents->arr[idents->num - 1] = (ident_t) {
		.pos    = name->pos,
		.strval = name->strval,
		.type   = s_type ? COPY(&type, var_type_t) : NULL
	};
	return *idents;
}

// A list of one identity.
idents_t idents_one(simple_type_t *s_type, strval_t *name) {
	var_type_t type;
	if (s_type) {
		type = (var_type_t) {
			.category    = TYPE_CAT_SIMPLE,
			.is_complete = true,
			.simple_type = *s_type,
			.size        = CSIZE_SIMPLE(*s_type)
		};
	}
	ident_t ident = {
		.pos    = name->pos,
		.strval = name->strval,
		.type   = s_type ? COPY(&type, var_type_t) : NULL
	};
	return (idents_t) {
		.arr = COPY(&ident, ident_t),
		.num = 1
	};
}

// Set the type of all identities contained.
idents_t idents_settype(idents_t *idents, simple_type_t *s_type) {
	var_type_t type = {
		.category    = TYPE_CAT_SIMPLE,
		.is_complete = true,
		.simple_type = *s_type,
		.size        = CSIZE_SIMPLE(*s_type)
	};
	for (size_t i = 0; i < idents->num; i++) {
		idents->arr[i].type = COPY(&type, var_type_t);
	}
}


// Numeric constant expression.
expr_t expr_icnst(ival_t *val) {
	return (expr_t) {
		.type     = EXPR_TYPE_CONST,
		.iconst   = val->ival
	};
}

// String constant expression.
expr_t expr_scnst(strval_t *val) {
	return (expr_t) {
		.type     = EXPR_TYPE_CONST,
		.strconst = val->strval
	};
}

// Identity expression (things like variables and functions).
expr_t expr_ident(strval_t *ident) {
	return (expr_t) {
		.type     = EXPR_TYPE_IDENT,
		.ident    = COPY(ident, strval_t)
	};
}

// Unary math expression (things like &a, b++ and !c).
expr_t expr_math1(oper_t type, expr_t *val) {
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
		.par_a    = COPY(val, expr_t)
	};
}

// Function call expression.
expr_t expr_call(expr_t *func, exprs_t *args) {
	return (expr_t) {
		.type     = EXPR_TYPE_CALL,
		.func     = func,
		.args     = COPY(args, exprs_t)
	};
}

// Binary math expression (things like a + b, c = d and e[f]).
expr_t expr_math2(oper_t type, expr_t *val1, expr_t *val2) {
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
		.par_a    = COPY(val1, expr_t),
		.par_b    = COPY(val2, expr_t)
	};
}

// Assignment math expression (things like a += b, c *= d and e |= f).
// Generalises to a combination of an assignment and expr_math2.
expr_t expr_matha(oper_t type, expr_t *val1, expr_t *val2) {
	// This is quite simple: val1 = val1 operator val2.
	expr_t param_b = expr_math2(type, val1, val2);
	return expr_math2(OP_ASSIGN, val1, &param_b);
}
