
#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

enum expr_type;
enum statement_type;
struct type;
struct ival;
struct ident;
struct expression;
struct expressions;
struct statement;
struct statements;
struct funcdef;
struct idents;
struct vardecl;
struct vardecls;
struct parser_ctx;

typedef enum expr_type expr_type_t;
typedef enum statement_type statement_type_t;
typedef struct type type_t;
typedef struct ival ival_t;
typedef struct ident ident_t;
typedef struct expression expression_t;
typedef struct expressions expressions_t;
typedef struct statement statement_t;
typedef struct statements statements_t;
typedef struct funcdef funcdef_t;
typedef struct idents idents_t;
typedef struct vardecl vardecl_t;
typedef struct vardecls vardecls_t;
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
	STATMT_TYPE_MULTI,
	STATMT_TYPE_FOR
};

// Type specification.
struct type {
	pos_t pos;
	type_spec_t type_spec;
};

// Combination of pos_t and int.
struct ival {
	pos_t pos;
	int ival;
};

// Combination of pos_t and char *.
struct ident {
	pos_t pos;
	char *ident;
};

// Expression: a + b * c, a = 123, etc.
struct expression {
	pos_t pos;
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
	pos_t pos;
	int num;
	expression_t *exprs;
};

// Statement: if, while, expression, etc.
struct statement {
	pos_t pos;
	statement_type_t type;
	expression_t *expr;
	expression_t *expr1;
	statement_t *statement;
	statement_t *statement1;
	statements_t *statements;
	vardecls_t *decls;
};

// Multiple statements.
struct statements {
	pos_t pos;
	int num;
	statement_t *statements;
};

// Definition of a function.
struct funcdef {
	pos_t pos;
	type_t returns;
	ident_t ident;
	int numParams;
	vardecl_t *params;
	statements_t *statements;
};

// Function parameters.
struct idents {
	pos_t pos;
	int numIdents;
	ident_t *idents;
};

// Definition of a single variable.
struct vardecl {
	pos_t pos;
	type_t type;
	ident_t ident;
	expression_t *expr;
};

// Multiple expressions: function params, array init.
struct vardecls {
	pos_t pos;
	int num;
	vardecl_t *vars;
};

// Wow, context!
struct parser_ctx {
	map_t varMap;
	map_t funcMap;
	tokeniser_ctx_t *tokeniser_ctx;
	asm_ctx_t *asm_ctx;
	type_t currentVarType;
	char *currentError;
	pos_t errorPos;
};

void init					(parser_ctx_t *ctx);

vardecls_t param_cat		(parser_ctx_t *ctx, vardecls_t *other, ident_t ident, type_t type);
vardecls_t param_new		(parser_ctx_t *ctx, ident_t ident, type_t type);
vardecls_t param_empty		(parser_ctx_t *ctx);
funcdef_t func_decl			(parser_ctx_t *ctx, type_t returns, ident_t ident, vardecls_t *params);
funcdef_t func_impl			(parser_ctx_t *ctx, type_t returns, ident_t ident, vardecls_t *params, statements_t *statmt);

vardecl_t decl_assign		(parser_ctx_t *ctx, ident_t ident, expression_t *expression);
vardecl_t decl				(parser_ctx_t *ctx, ident_t ident);
vardecl_t declt_assign		(parser_ctx_t *ctx, ident_t ident, type_t type, expression_t *expression);
vardecl_t declt				(parser_ctx_t *ctx, ident_t ident, type_t type);

statement_t statmt_nop		(parser_ctx_t *ctx);
statement_t statmt_expr		(parser_ctx_t *ctx, expression_t *expr);
statement_t statmt_ret		(parser_ctx_t *ctx, expression_t *expr);
statement_t statmt_var		(parser_ctx_t *ctx, vardecls_t *var);
statement_t statmt_if		(parser_ctx_t *ctx, expression_t *expr, statement_t *code, statement_t *else_code);
statement_t statmt_while	(parser_ctx_t *ctx, expression_t *expr, statement_t *code);
statement_t statmt_for		(parser_ctx_t *ctx, statement_t *setup, expression_t *cond, expression_t *inc, statement_t *code);
statement_t statmt_multi	(parser_ctx_t *ctx, statements_t *code);

expression_t expr_var		(parser_ctx_t *ctx, ident_t ident);
expression_t expr_const		(parser_ctx_t *ctx, ival_t iconst);
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

type_t type_void			(parser_ctx_t *ctx);
type_t type_number			(parser_ctx_t *ctx, type_simple_t type);
type_t type_signed			(parser_ctx_t *ctx, type_t *other);
type_t type_unsigned		(parser_ctx_t *ctx, type_t *other);

int yyparse					(parser_ctx_t *ctx);

extern void function_added	(parser_ctx_t *ctx, funcdef_t *func);
extern int yylex			(parser_ctx_t *ctx);
extern void yyerror			(parser_ctx_t *ctx, char *msg);

void free_ident				(parser_ctx_t *ctx, ident_t *ident);
void free_garbage			(parser_ctx_t *ctx, ident_t *garbage);
void free_funcdef			(parser_ctx_t *ctx, funcdef_t *funcdef);
void free_idents			(parser_ctx_t *ctx, idents_t *idents);
void free_vardecl			(parser_ctx_t *ctx, vardecl_t *vardecl);
void free_vardecls			(parser_ctx_t *ctx, vardecls_t *vardecls);
void free_statmt			(parser_ctx_t *ctx, statement_t *statmt);
void free_statmts			(parser_ctx_t *ctx, statements_t *statmts);
void free_expr				(parser_ctx_t *ctx, expression_t *expr);
void free_exprs				(parser_ctx_t *ctx, expressions_t *exprs);

#endif // PARSER_UTIL_H
