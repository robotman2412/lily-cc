
#include "pront.h"
#include "parser-util.h"

static char *op_names[] = {
	// Unary operators.
	"LOGIC_NOT",
	"ADROF",
	"DEREF",
	"0_MINUS",
	//Binary operators.
	"ADD",
	"SUB",
	"MUL",
	"DIV",
	"MOD",
	"LOGIC_AND",
	"LOGIC_OR",
	//Bitwise operators.
	"BIT_NOT",
	"BIT_AND",
	"OP_BIT_OR",
	"BIT_XOR",
	// Comparison operators.
	"BGT",
	"BGE",
	"BLT",
	"BLE",
	"BEQ",
	"BNE",
	// Assignment operators.
	"ASSIGN",
	// Miscellaneous operators.
	"INDEX"
};

void pront_exprs(exprs_t *expr) {
	for (int i = 0; i < expr->num; i++) {
		pront_expr(&expr->arr[i]);
	}
}

void pront_expr(expr_t *expr) {
	switch (expr->type) {
		case (EXPR_TYPE_CALL):
			printf("(");
			pront_expr(expr->func);
			printf(")(");
			pront_exprs(expr->args);
			printf(") CALL ");
			break;
		case (EXPR_TYPE_CONST):
			printf("%ld ", expr->iconst);
			break;
		case (EXPR_TYPE_CSTR):
			printf("\"%s\" ", expr->ident->strval);
			break;
		case (EXPR_TYPE_IDENT):
			printf("%s ", expr->ident->strval);
			break;
		case (EXPR_TYPE_MATH1):
			pront_expr(expr->par_a);
			printf("%s ", op_names[expr->oper]);
			break;
		case (EXPR_TYPE_MATH2):
			pront_expr(expr->par_a);
			pront_expr(expr->par_b);
			printf("%s ", op_names[expr->oper]);
			break;
	}
}
