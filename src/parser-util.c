
#include "parser-util.h"
#include <malloc.h>

funcdef_t funcdef_decl(ident_t *ident, idents_t *args, stmts_t *code) {
	return (funcdef_t) {
		.ident = *ident,
		.args  = *args,
		.stmts = code
	};
}



stmts_t stmts_empty() {
	return (stmts_t) {
		.arr = NULL,
		.num = 0,
	};
}

stmts_t stmts_cat(stmts_t *stmts, stmt_t *stmt) {
	stmts->num ++;
	stmts->arr = realloc(stmts->arr, stmts->num * sizeof(stmt_t));
	stmts->arr[stmts->num - 1] = *stmt;
	return *stmts;
}



stmt_t stmt_multi(stmts_t *stmts) {
	return (stmt_t) {
		.type  = STMT_TYPE_MULTI,
		.stmts = COPY(stmts, stmts_t)
	};
}

stmt_t stmt_if(expr_t *cond, stmt_t *s_if, stmt_t *s_else) {
	return (stmt_t) {
		.type       = STMT_TYPE_IF,
		.cond       = COPY(cond,   expr_t),
		.code_true  = COPY(s_if,   stmt_t),
		.code_false = COPY(s_else, stmt_t)
	};
}

stmt_t stmt_while(expr_t *cond, stmt_t *code) {
	return (stmt_t) {
		.type       = STMT_TYPE_WHILE,
		.cond       = COPY(cond,   expr_t),
		.code_true  = COPY(code,   stmt_t)
	};
}

stmt_t stmt_ret(expr_t *expr) {
	return (stmt_t) {
		.type  = STMT_TYPE_RET,
		.expr  = COPY(expr, expr_t)
	};
}

stmt_t stmt_var(idents_t *decls) {
	return (stmt_t) {
		.type  = STMT_TYPE_VAR,
		.vars  = COPY(decls, idents_t)
	};
}

stmt_t stmt_expr(expr_t *expr) {
	return (stmt_t) {
		.type  = STMT_TYPE_EXPR,
		.expr  = COPY(expr, expr_t)
	};
}

stmt_t stmt_iasm(strval_t *text, iasm_regs_t *outputs, iasm_regs_t *inputs, void *clobbers) {
	iasm_t *iasm = (iasm_t *) malloc(sizeof(iasm_t));
	*iasm = (iasm_t) {
		.text     = *text,
		// .outputs  = outputs,
		// .inputs   = inputs,
		// .clobbers = clobbers
	};
	return (stmt_t) {
		.type  = STMT_TYPE_IASM,
		.iasm  = iasm
	};
}



idents_t idents_empty() {
	return (idents_t) {
		.arr = NULL,
		.num = 0
	};
}

idents_t idents_cat(idents_t *idents, ident_t *ident) {
	idents->num ++;
	idents->arr = realloc(idents->arr, idents->num * sizeof(ident_t));
	idents->arr[idents->num - 1] = *ident;
	return *idents;
}

idents_t idents_one(ident_t *ident) {
	return (idents_t) {
		.arr = COPY(ident, ident_t),
		.num = 1
	};
}



expr_t expr_icnst(ival_t *val) {
	return (expr_t) {
		.type     = EXPR_TYPE_CONST,
		.iconst   = val->ival
	};
}

expr_t expr_scnst(strval_t *val) {
	return (expr_t) {
		.type     = EXPR_TYPE_CONST,
		.strconst = val->strval
	};
}

expr_t expr_ident(ident_t *ident) {
	return (expr_t) {
		.type     = EXPR_TYPE_IDENT,
		.ident    = COPY(ident, ident_t)
	};
}

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

expr_t expr_call(expr_t *func, exprs_t *args) {
	return (expr_t) {
		.type     = EXPR_TYPE_CALL,
		.func     = func,
		.args     = COPY(args, exprs_t)
	};
}

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

