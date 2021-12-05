
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
		.num = 0
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

//stmt_t stmt_if(expr_t *cond, stmt_t *s_if, stmt_t *s_else);
//stmt_t stmt_while(expr_t *cond, stmt_t *code);

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

//stmt_t stmt_iasm(iasm_t *iasm);



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
	return (expr_t) {
		.type     = EXPR_TYPE_MATH2,
		.oper     = type,
		.par_a    = COPY(val1, expr_t),
		.par_b    = COPY(val2, expr_t)
	};
}

