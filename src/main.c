
#include "main.h"
#include <stdio.h>

#include <asm.h>
#include <config.h>
#include <tokeniser.h>
#include <gen.h>

typedef struct token {
	enum yytokentype type;
	YYSTYPE val;
} token_t;

/*
func FNNAME (a) {
	var b = 2 + a * 3;
	return b;
}
*/

char *someCode = \
"func FNNAME (a) {\n" \
"var b = 2 + a * 3;\n" \
"return b;\n" \
"}\n";

int main(int argc, char **argv) {
#if defined(ENABLE_DEBUG_LOGS) && YYDEBUG
	yydebug = 1;
#endif
	
	// Read file.
	char *filename = "test/test0.txt";
	FILE *file = fopen(filename, "r");
	fseek(file, 0, SEEK_END);
	size_t len = ftell(file);
	rewind(file);
	char *source = malloc(len + 1);
	size_t read = fread(source, 1, len, file);
	source = realloc(source, read + 1);
	source[read] = 0;
	
	// Initialise.
	parser_ctx_t ctx;
	tokeniser_ctx_t tokeniser_ctx;
	asm_ctx_t asm_ctx;
	ctx.tokeniser_ctx = &tokeniser_ctx;
	ctx.asm_ctx = &asm_ctx;
	tokeniser_init(&tokeniser_ctx, source);
	asm_init(&asm_ctx);
	
	// Lol who compiles.
	yyparse(&ctx);
}

int yylex(parser_ctx_t *ctx) {
	return tokenise(ctx->tokeniser_ctx);
}

void yyerror(parser_ctx_t *ctx, char *msg) {
	fputs(msg, stderr);
	fflush(stderr);
}

void prontExprs(expressions_t *expr);
void prontExpr(expression_t *expr);
void prontVardecls(vardecls_t *var, int depth);
void prontVardecl(vardecl_t *var, int depth);
void prontStatmts(statements_t *statmt, int depth);
void prontStatmt(statement_t *statmt, int depth);

void prontVardecls(vardecls_t *var, int depth) {
	for (int i = 0; i < var->num; i++) {
		prontVardecl(&var->vars[i], depth);
	}
}

void prontVardecl(vardecl_t *var, int depth) {
	char *deep = malloc(sizeof(char) * (depth * 2 + 1));
	deep[depth * 2] = 0;
	for (int i = 0; i < depth * 2; i++) {
		deep[i] = ' ';
	}
	printf("%svardecl_t '%s'", deep, var->ident);
	if (var->expr) {
		printf(":\n%s  expression_t: ", deep);
		prontExpr(var->expr);
	}
	printf("\n");
	free(deep);
}

void prontExprs(expressions_t *expr) {
	if (expr->num) {
		prontExpr(expr->exprs);
		for (int i = 1; i < expr->num; i++) {
			printf(", ");
			prontExpr(&expr->exprs[i]);
		}
	}
}

void prontExpr(expression_t *expr) {
	switch (expr->type) {
		case (EXPR_TYPE_IDENT):
			printf("%s ", expr->ident);
			break;
		case (EXPR_TYPE_ICONST):
			printf("%d ", expr->value);
			break;
		case (EXPR_TYPE_INVOKE):
			prontExpr(expr->expr);
			printf("( ");
			prontExprs(expr->exprs);
			printf(") ");
			break;
		case (EXPR_TYPE_ASSIGN):
			prontExpr(expr->expr);
			prontExpr(expr->expr1);
			printf("ASSIGN ");
			break;
		case (EXPR_TYPE_MATH2):
			prontExpr(expr->expr);
			prontExpr(expr->expr1);
			switch (expr->oper) {
				case (OP_ADD):
					printf("ADD ");
					break;
				case (OP_SUB):
					printf("SUB ");
					break;
				case (OP_MUL):
					printf("MUL ");
					break;
				case (OP_DIV):
					printf("DIV ");
					break;
				case (OP_REM):
					printf("REM ");
					break;
			}
			break;
		case (EXPR_TYPE_MATH1):
			prontExpr(expr->expr);
			switch (expr->oper) {
				case (OP_INC):
					printf("INC ");
					break;
				case (OP_DEC):
					printf("DEC ");
					break;
			}
			break;
	}
}

void prontStatmts(statements_t *statmt, int depth) {
	for (int i = 0; i < statmt->num; i++) {
		prontStatmt(&statmt->statements[i], depth);
	}
}

void prontStatmt(statement_t *statmt, int depth) {
	char *deep = (char *) malloc(sizeof(char) * (depth * 2 + 1));
	deep[depth * 2] = 0;
	for (int i = 0; i < depth * 2; i++) {
		deep[i] = ' ';
	}
	if (statmt->type == STATMT_TYPE_MULTI) {
		printf("%sstatement_t: multi\n", deep);
		prontStatmts(statmt->statements, depth + 1);
	} else {
		switch (statmt->type) {
			case(STATMT_TYPE_NOP):
				printf("%sstatement_t: nop\n", deep);
				break;
			case(STATMT_TYPE_EXPR):
				printf("%sstatement_t: expr\n", deep);
				printf("%s  expression_t ", deep);
				prontExpr(statmt->expr);
				printf("\n");
				break;
			case(STATMT_TYPE_RET):
				printf("%sstatement_t: return\n", deep);
				printf("%s  expression_t ", deep);
				prontExpr(statmt->expr);
				printf("\n");
				break;
			case(STATMT_TYPE_VAR):
				printf("%sstatement_t: vardecl\n", deep);
				prontVardecls(statmt->decls, depth + 1);
				break;
			case(STATMT_TYPE_IF):
				printf("%sstatement_t: if\n", deep);
				printf("%s  expression_t ", deep);
				prontExpr(statmt->expr);
				printf("\n");
				prontStatmt(statmt->statement, depth + 1);
				if (statmt->statement1->type != STATMT_TYPE_NOP) {
					printf("%selse:\n", deep);
					prontStatmt(statmt->statement1, depth + 1);
				}
				break;
			case(STATMT_TYPE_WHILE):
				printf("%sstatement_t: while\n", deep);
				printf("%s  expression_t ", deep);
				prontExpr(statmt->expr);
				printf("\n");
				prontStatmt(statmt->statement, depth + 1);
				break;
		}
	}
	free(deep);
}

void function_added(parser_ctx_t *parser_ctx, funcdef_t *func) {
	// Some debug printening.
	printf("funcdef_t '%s' (", func->ident);
	if (func->numParams) fputs(func->paramIdents[0], stdout);
	for (int i = 1; i < func->numParams; i ++) {
		fputc(',', stdout);
		fputs(func->paramIdents[i], stdout);
	}
	printf("):\n");
	prontStatmts(func->statements, 1);
	// Create function asm.
	asm_preproc_function(parser_ctx, func);
	asm_write_function(parser_ctx, func);
}


