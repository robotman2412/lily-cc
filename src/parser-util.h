
#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

enum expr_type;
enum statement_type;
struct expression;
struct expressions;
struct statement;
struct statements;
struct funcdef;
struct idents;
struct vardecl;
struct scope;
struct parser_ctx;

typedef enum expr_type expr_type_t;
typedef enum statement_type statement_type_t;
typedef struct expression expression_t;
typedef struct expressions expressions_t;
typedef struct statement statement_t;
typedef struct statements statements_t;
typedef struct funcdef funcdef_t;
typedef struct idents idents_t;
typedef struct vardecl vardecl_t;
typedef struct vardecls vardecls_t;
typedef struct scope scope_t;
typedef struct parser_ctx parser_ctx_t;

#include "strmap.h"
#include "tokeniser.h"
#include <gen.h>
#include <asm.h>

// Type of operation for an expression.
enum expr_type {
	EXPR_TYPE_IDENT,
	EXPR_TYPE_ICONST,
	EXPR_TYPE_INVOKE,
	EXPR_TYPE_ASSIGN,
	EXPR_TYPE_MATH2,
	EXPR_TYPE_MATH1
};

// Type of operation for an expression.
enum statement_type {
	STATMT_TYPE_NOP,
	STATMT_TYPE_EXPR,
	STATMT_TYPE_RET,
	STATMT_TYPE_VAR,
	STATMT_TYPE_IF,
	STATMT_TYPE_WHILE,
	STATMT_TYPE_MULTI
};

// Expression: a + b * c, a = 123, etc.
struct expression {
	enum expr_type type;
	char *ident;
	expression_t *expr;
	expression_t *expr1;
	expressions_t *exprs;
	int value;
	operator_t oper;
};

// Multiple expressions: function params, array init.
struct expressions {
	int num;
	expression_t *exprs;
};

// Statement: if, while, expression, etc.
struct statement {
	statement_type_t type;
	expression_t *expr;
	statement_t *statement;
	statement_t *statement1;
	statements_t *statements;
	vardecls_t *decls;
};

// Multiple statements.
struct statements {
	int num;
	statement_t *statements;
};

// Definition of a function.
struct funcdef {
	char *ident;
	int numParams;
	char **paramIdents;
	statements_t *statements;
};

// Function parameters.
struct idents {
	int numIdents;
	char **idents;
};

// Definition of a single variable.
struct vardecl {
	char *ident;
	expression_t *expr;
};

// Multiple expressions: function params, array init.
struct vardecls {
	int num;
	vardecl_t *vars;
};

// Scopes of variables.
struct scope {
	scope_t *parent;
	map_t varMap;
};

struct parser_ctx {
	scope_t *scope;
	map_t funcMap;
	tokeniser_ctx_t *tokeniser_ctx;
	asm_ctx_t *asm_ctx;
};

void init					(parser_ctx_t *ctx);

void push_scope				(parser_ctx_t *ctx);
void pop_scope				(parser_ctx_t *ctx);

void pre_func				(parser_ctx_t *ctx);
idents_t param_cat			(parser_ctx_t *ctx, idents_t *other, char *ident);
idents_t param_new			(parser_ctx_t *ctx, char *ident);
funcdef_t post_func			(parser_ctx_t *ctx, char *ident, idents_t *idents, statements_t *statmt);

vardecl_t decl_assign		(parser_ctx_t *ctx, char *ident, expression_t *expression);
vardecl_t decl				(parser_ctx_t *ctx, char *ident);

statement_t statmt_nop		(parser_ctx_t *ctx);
statement_t statmt_expr		(parser_ctx_t *ctx, expression_t *expr);
statement_t statmt_ret		(parser_ctx_t *ctx, expression_t *expr);
statement_t statmt_var		(parser_ctx_t *ctx, vardecls_t *var);
statement_t statmt_if		(parser_ctx_t *ctx, expression_t *expr, statement_t *code, statement_t *else_code);
statement_t statmt_while	(parser_ctx_t *ctx, expression_t *expr, statement_t *code);
statement_t statmt_multi	(parser_ctx_t *ctx, statements_t *code);

expression_t expr_var		(parser_ctx_t *ctx, char *ident);
expression_t expr_const		(parser_ctx_t *ctx, int iconst);
expression_t expr_call		(parser_ctx_t *ctx, expression_t *func, expressions_t *params);
expression_t expr_assign	(parser_ctx_t *ctx, expression_t *var, expression_t *val);
expression_t expr_math2		(parser_ctx_t *ctx, expression_t *left, expression_t *right, operator_t oper);
expression_t expr_math1		(parser_ctx_t *ctx, expression_t *var, operator_t oper);

statements_t statmts_cat	(parser_ctx_t *ctx, statements_t *other, statement_t *add);
statements_t statmts_new	(parser_ctx_t *ctx);
expressions_t exprs_cat		(parser_ctx_t *ctx, expressions_t *other, expression_t *add);
expressions_t exprs_new		(parser_ctx_t *ctx, expression_t *first);
vardecls_t vars_cat			(parser_ctx_t *ctx, vardecls_t *other, vardecl_t *add);
vardecls_t vars_new			(parser_ctx_t *ctx, vardecl_t *first);

int yyparse					(parser_ctx_t *ctx);

extern void function_added	(parser_ctx_t *ctx, funcdef_t *func);
extern int yylex			(parser_ctx_t *ctx);
extern void yyerror			(parser_ctx_t *ctx, char *msg);

#endif // PARSER_UTIL_H
