
#include "parser-util.h"
#include <malloc.h>

funcdef_t funcdef_decl(ident_t *ident, idents_t *args, stmts_t *code) {
	return (funcdef_t) {
		.ident = *ident,
		.args  = *args,
		/*.code  = *code*/
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

