
%require "3.5"

%{
#define YYDEBUG 1
%}

%code requires {
#include "parser-util.h"
#include <malloc.h>
#include <string.h>

}

%param { parser_ctx_t *ctx };

%union {
	int ival;
	char *ident;
	funcdef_t func;
	idents_t idents;
	vardecl_t var;
	vardecls_t vars;
	statement_t statmt;
	statements_t statmts;
	expression_t expr;
	expressions_t exprs;
}

%token TKN_IF "if" TKN_ELSE "else" TKN_WHILE "while" TKN_FOR "for" TKN_RETURN "return"
%token TKN_LPAR "(" TKN_RPAR ")" TKN_LBRAC "{" TKN_RBRAC "}" TKN_SEMI ";" TKN_COMMA ","
%token <ival> TKN_NUM
%token <ident> TKN_IDENT
%token TKN_INC "++" TKN_DEC "--" TKN_ADD "+" TKN_SUB "-" TKN_ASSIGN "="
%token TKN_MUL "*" TKN_DIV "/" TKN_REM "%"
%token TKN_THEN "then"
%token TKN_STRUCT "struct" TKN_ENUM "enum" TKN_VOID "void"
%token TKN_SIGNED "signed" TKN_UNSIGNED "unsigned" TKN_CHAR "char" TKN_SHORT "short" TKN_LONG "long" TKN_INT "int"
%token TKN_FLOAT "float" TKN_DOUBLE "double"

%type <func> funcdef
%type <idents> funcparams
%type <idents> opt_funcparams
%type <var> varassign
%type <vars> vardecl
%type <vars> vardecls
%type <statmt> statmt
%type <statmts> statmts
%type <statmt> opt_else
%type <expr> expr
%type <exprs> exprs
%type <exprs> opt_exprs

%right "="
%left "+" "-"
%left "*" "/" "%"
%precedence "++" "--"
%precedence "("

%precedence "then"
%precedence "else"

%%
library:		{init(ctx);}		funcdefs;
funcdef:		{pre_func(ctx);}	type_spec TKN_IDENT "(" opt_funcparams ")" "{" statmts "}" {$$=post_func(ctx, $3, &$5, &$8);};
funcdefs:		funcdefs funcdef
|				%empty;
opt_funcparams:	funcparams							{$$=$1;}
|				%empty								{$$=param_new   (ctx, NULL);}
funcparams:		funcparams "," type_spec TKN_IDENT	{$$=param_cat   (ctx, &$1, $4);}
|				type_spec TKN_IDENT					{$$=param_new   (ctx, $2);};

vardecl:		type_spec varassign					{$$=vars_new    (ctx, &$2);}
|				type_spec varassign "," vardecls	{$$=vars_cat    (ctx, &$4, &$2);};
vardecls:		vardecls "," type_spec varassign	{$$=vars_cat    (ctx, &$1, &$4);}
|				vardecls "," varassign				{$$=vars_cat    (ctx, &$1, &$3);}
|				type_spec varassign					{$$=vars_new    (ctx, &$2);}
|				varassign							{$$=vars_new    (ctx, &$1);};
varassign:		TKN_IDENT "=" expr					{$$=decl_assign (ctx, $1, &$3);}
|				TKN_IDENT							{$$=decl        (ctx, $1);};
statmt:			"{" statmts "}"						{$$=statmt_multi(ctx, &$2);}
|				"if"	"(" expr ")" statmt
				opt_else							{$$=statmt_if   (ctx, &$3, &$5, &$6);}
|				"while"	"(" expr ")" statmt			{$$=statmt_while(ctx, &$3, &$5);}
|				"for" "(" statmt expr
								";" expr ")" statmt	{$$=statmt_for  (ctx, &$3, &$4, &$6, &$8);}
|				"return" expr	";"					{$$=statmt_ret  (ctx, &$2);}
|				vardecl			";"					{$$=statmt_var  (ctx, &$1);}
|				expr			";"					{$$=statmt_expr (ctx, &$1);}
|				";"									{$$=statmt_nop  (ctx);};
statmts:		%empty								{$$=statmts_new (ctx);}
|				statmts statmt						{$$=statmts_cat (ctx, &$1, &$2);};
expr:			TKN_IDENT							{$$=expr_var    (ctx, $1);}
|				TKN_NUM								{$$=expr_const  (ctx, $1);}
|				"(" expr ")"						{$$=$2;}
|				expr "(" opt_exprs ")"				{$$=expr_call   (ctx, &$1, &$3);}
|				expr "=" expr						{$$=expr_assign (ctx, &$1, &$3);}
|				expr "+" expr						{$$=expr_math2  (ctx, &$1, &$3, OP_ADD);}
|				expr "-" expr		%prec "+"		{$$=expr_math2  (ctx, &$1, &$3, OP_SUB);}
|				expr "*" expr						{$$=expr_math2  (ctx, &$1, &$3, OP_MUL);}
|				expr "/" expr		%prec "*"		{$$=expr_math2  (ctx, &$1, &$3, OP_DIV);}
|				expr "%" expr		%prec "*"		{$$=expr_math2  (ctx, &$1, &$3, OP_REM);}
|				expr "++"							{$$=expr_math1  (ctx, &$1, OP_INC);}
|				expr "--"			%prec "++"		{$$=expr_math1  (ctx, &$1, OP_DEC);};
exprs:			exprs "," expr						{$$=exprs_cat   (ctx, &$1, &$3);}
|				expr								{$$=exprs_new   (ctx, &$1);};
opt_exprs:		exprs								{$$=$1;}
|				%empty								{$$=exprs_new   (ctx, NULL);};
opt_else:		"else" statmt						{$$=$2;}
|				%empty				%prec "then"	{$$=statmt_nop  (ctx);};
type_spec:		"void"
|				"signed" int_spec
|				"unsigned" int_spec
|				int_spec
|				"float"
|				"double"
|				"long" "double"
|				"struct" TKN_IDENT
|				"enum" TKN_IDENT;
int_spec:		"char"
|				"short" opt_int
|				"int"
|				"long" opt_int
|				"long" "long" opt_int;
opt_int:		"int"
|				%empty;
%%

static void *make_copy(void *mem, size_t size) {
	void *copy = malloc(size);
	memcpy(copy, mem, size);
	return copy;
}

void init(parser_ctx_t *ctx) {
	ctx->scope = NULL;
	map_create(&ctx->funcMap);
}

void push_scope(parser_ctx_t *ctx) {
	scope_t *current = (scope_t *) malloc(sizeof(scope_t));
	current->parent = ctx->scope;
	map_create(&current->varMap);
	ctx->scope = current;
}

void pop_scope(parser_ctx_t *ctx) {
	map_delete_with_values(&ctx->scope->varMap);
	void *mem = ctx->scope;
	ctx->scope = ctx->scope->parent;
	free(mem);
}


void pre_func(parser_ctx_t *ctx) {
	push_scope(ctx);
}

idents_t param_cat(parser_ctx_t *ctx, idents_t *other, char *ident) {
	int num = other->numIdents + 1;
	char **mem = (char **) realloc(other->idents, sizeof(char *) * num);
	mem[other->numIdents] = ident;
	return (idents_t) {
		.numIdents = num,
		.idents = mem
	};
}

idents_t param_new(parser_ctx_t *ctx, char *ident) {
	if (ident) {
		char **mem = (char **) malloc(sizeof(char *));
		*mem = ident;
		return (idents_t) {
			.numIdents = 1,
			.idents = mem
		};
	} else {
		return (idents_t) {
			.numIdents = 0,
			.idents = NULL
		};
	}
}

funcdef_t post_func(parser_ctx_t *ctx, char *ident, idents_t *idents, statements_t *statmt) {
	funcdef_t f = {
		.ident = ident,
		.numParams = idents->numIdents,
		.paramIdents = idents->idents,
		.statements = statmt
	};
	pop_scope(ctx);
	funcdef_t *existing = map_get(&ctx->funcMap, ident);
	if (existing) {
		yyerror(ctx, "Function is already declared.");
	} else {
		funcdef_t *add = malloc(sizeof(funcdef_t));
		*add = f;
		map_set(&ctx->funcMap, ident, add);
	}
	function_added(ctx, &f);
	return f;
}


vardecl_t decl_assign(parser_ctx_t *ctx, char *ident, expression_t *expression) {
	if (expression) expression = make_copy(expression, sizeof(expression_t));
	vardecl_t f = {
		.ident = ident,
		.expr = expression
	};
	vardecl_t *existing = map_get(&ctx->scope->varMap, ident);
	if (existing) {
		yyerror(ctx, "Variable is already declared in this scope.");
	} else {
		vardecl_t *add = malloc(sizeof(vardecl_t));
		*add = f;
		map_set(&ctx->scope->varMap, ident, add);
	}
	return f;
}

vardecl_t decl(parser_ctx_t *ctx, char *ident) {
	decl_assign(ctx, ident, NULL);
}


statement_t statmt_nop(parser_ctx_t *ctx) {
	return (statement_t) {
		.type = STATMT_TYPE_NOP,
		.expr = NULL,
		.expr1 = NULL,
		.statement = NULL,
		.statement1 = NULL,
		.statements = NULL,
		.decls = NULL
	};
}

statement_t statmt_expr(parser_ctx_t *ctx, expression_t *expr) {
	return (statement_t) {
		.type = STATMT_TYPE_EXPR,
		.expr = make_copy(expr, sizeof(expression_t)),
		.expr1 = NULL,
		.statement = NULL,
		.statement1 = NULL,
		.statements = NULL,
		.decls = NULL
	};
}

statement_t statmt_ret(parser_ctx_t *ctx, expression_t *expr) {
	return (statement_t) {
		.type = STATMT_TYPE_RET,
		.expr = make_copy(expr, sizeof(expression_t)),
		.expr1 = NULL,
		.statement = NULL,
		.statement1 = NULL,
		.statements = NULL,
		.decls = NULL
	};
}

statement_t statmt_var(parser_ctx_t *ctx, vardecls_t *var) {
	return (statement_t) {
		.type = STATMT_TYPE_VAR,
		.expr = NULL,
		.expr1 = NULL,
		.statement = NULL,
		.statement1 = NULL,
		.statements = NULL,
		.decls = make_copy(var, sizeof(vardecls_t))
	};
}

statement_t statmt_if(parser_ctx_t *ctx, expression_t *expr, statement_t *code, statement_t *else_code) {
	return (statement_t) {
		.type = STATMT_TYPE_IF,
		.expr = make_copy(expr, sizeof(expression_t)),
		.expr1 = NULL,
		.statement = make_copy(code, sizeof(statement_t)),
		.statement1 = make_copy(else_code, sizeof(statement_t)),
		.statements = NULL,
		.decls = NULL
	};
}

statement_t statmt_while(parser_ctx_t *ctx, expression_t *expr, statement_t *code) {
	return (statement_t) {
		.type = STATMT_TYPE_WHILE,
		.expr = make_copy(expr, sizeof(expression_t)),
		.expr1 = NULL,
		.statement = make_copy(code, sizeof(statement_t)),
		.statement1 = NULL,
		.statements = NULL,
		.decls = NULL
	};
}

statement_t statmt_for(parser_ctx_t *ctx, statement_t *setup, expression_t *cond, expression_t *inc, statement_t *code) {
	return (statement_t) {
		.type = STATMT_TYPE_FOR,
		.expr = make_copy(cond, sizeof(expression_t)),
		.expr1 = make_copy(inc, sizeof(expression_t)),
		.statement = make_copy(setup, sizeof(statement_t)),
		.statement1 = make_copy(code, sizeof(statement_t)),
		.statements = NULL,
		.decls = NULL
	};
}

statement_t statmt_multi(parser_ctx_t *ctx, statements_t *code) {
	return (statement_t) {
		.type = STATMT_TYPE_MULTI,
		.expr = NULL,
		.expr1 = NULL,
		.statement = NULL,
		.statement1 = NULL,
		.statements = make_copy(code, sizeof(statements_t)),
		.decls = NULL
	};
}


expression_t expr_var(parser_ctx_t *ctx, char *ident) {
	return (expression_t) {
		.type = EXPR_TYPE_IDENT,
		.ident = ident,
		.expr = NULL,
		.expr1 = NULL,
		.exprs = NULL,
		.value = 0
	};
}

expression_t expr_const(parser_ctx_t *ctx, int iconst) {
	return (expression_t) {
		.type = EXPR_TYPE_ICONST,
		.ident = NULL,
		.expr = NULL,
		.expr1 = NULL,
		.exprs = NULL,
		.value = iconst
	};
}

expression_t expr_call(parser_ctx_t *ctx, expression_t *func, expressions_t *params) {
	return (expression_t) {
		.type = EXPR_TYPE_INVOKE,
		.ident = NULL,
		.expr = make_copy(func, sizeof(expression_t)),
		.expr1 = NULL,
		.exprs = make_copy(params, sizeof(expressions_t)),
		.value = 0
	};
}

expression_t expr_assign(parser_ctx_t *ctx, expression_t *var, expression_t *val) {
	return (expression_t) {
		.type = EXPR_TYPE_ASSIGN,
		.ident = NULL,
		.expr = make_copy(var, sizeof(expression_t)),
		.expr1 = make_copy(val, sizeof(expression_t)),
		.exprs = NULL,
		.value = 0
	};
}

expression_t expr_math2(parser_ctx_t *ctx, expression_t *left, expression_t *right, operator_t oper) {
	return (expression_t) {
		.type = EXPR_TYPE_MATH2,
		.ident = NULL,
		.expr = make_copy(left, sizeof(expression_t)),
		.expr1 = make_copy(right, sizeof(expression_t)),
		.exprs = NULL,
		.value = 0,
		.oper = oper
	};
}

expression_t expr_math1(parser_ctx_t *ctx, expression_t *var, operator_t oper) {
	return (expression_t) {
		.type = EXPR_TYPE_MATH1,
		.ident = NULL,
		.expr = make_copy(var, sizeof(expression_t)),
		.expr1 = NULL,
		.exprs = NULL,
		.value = 0,
		.oper = oper
	};
}


statements_t statmts_cat(parser_ctx_t *ctx, statements_t *other, statement_t *add) {
	statement_t *mem = other->statements;
	mem = realloc(mem, sizeof(statement_t) * (other->num + 1));
	mem[other->num] = *add;
	return (statements_t) {
		.num = other->num + 1,
		.statements = mem
	};
}

statements_t statmts_new(parser_ctx_t *ctx) {
	statement_t *mem = malloc(0);
	return (statements_t) {
		.num = 0,
		.statements = mem
	};
}

expressions_t exprs_cat(parser_ctx_t *ctx, expressions_t *other, expression_t *add) {
	if (other->num) {
		expression_t *mem = other->exprs;
		mem = realloc(mem, sizeof(expression_t) * (other->num + 1));
		mem[other->num] = *add;
		return (expressions_t) {
			.num = other->num + 1,
			.exprs = mem
		};
	} else {
		return exprs_new(ctx, add);
	}
}

expressions_t exprs_new(parser_ctx_t *ctx, expression_t *first) {
	if (first) {
		expression_t *mem = malloc(sizeof(expression_t));
		*mem = *first;
		return (expressions_t) {
			.num = 1,
			.exprs = mem
		};
	} else {
		return (expressions_t) {
			.num = 0,
			.exprs = NULL
		};
	}
}

vardecls_t vars_cat(parser_ctx_t *ctx, vardecls_t *other, vardecl_t *add) {
	vardecl_t *mem = other->vars;
	mem = realloc(mem, sizeof(vardecl_t) * (other->num + 1));
	mem[other->num] = *add;
	return (vardecls_t) {
		.num = other->num + 1,
		.vars = mem
	};
}

vardecls_t vars_new(parser_ctx_t *ctx, vardecl_t *first) {
	vardecl_t *mem = malloc(sizeof(vardecl_t));
	*mem = *first;
	return (vardecls_t) {
		.num = 1,
		.vars = mem
	};
}

