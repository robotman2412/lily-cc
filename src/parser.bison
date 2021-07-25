
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
	pos_t pos;
	ival_t ival;
	ident_t ident;
	ident_t garbage;
	funcdef_t func;
	idents_t idents;
	vardecl_t var;
	vardecls_t vars;
	statement_t statmt;
	statements_t statmts;
	expression_t expr;
	expressions_t exprs;
	type_t type;
}

/* %destructor {free_ident(&$$);} <ident>
%destructor {free_garbage(&$$);} <ident>
%destructor {free_funcdef(&$$);} <func>
%destructor {free_idents(&$$);} <idents>
%destructor {free_vardecl(&$$);} <var>
%destructor {free_vardecls(&$$);} <vars>
%destructor {free_statmt(&$$);} <statmt>
%destructor {free_statmts(&$$);} <statmts>
%destructor {free_expr(&$$);} <expr>
%destructor {free_exprs(&$$);} <exprs> */

%destructor {printf("free_ident(&$$);\n");} <ident>
%destructor {printf("free_garbage(&$$);\n");} <garbage>
%destructor {printf("free_funcdef(&$$);\n");} <func>
%destructor {printf("free_idents(&$$);\n");} <idents>
%destructor {printf("free_vardecl(&$$);\n");} <var>
%destructor {printf("free_vardecls(&$$);\n");} <vars>
%destructor {printf("free_statmt(&$$);\n");} <statmt>
%destructor {printf("free_statmts(&$$);\n");} <statmts>
%destructor {printf("free_expr(&$$);\n");} <expr>
%destructor {printf("free_exprs(&$$);\n");} <exprs>
%destructor {printf("free_pos(&$$);\n");} <pos>

%token <pos> TKN_IF "if" TKN_ELSE "else" TKN_WHILE "while" TKN_FOR "for" TKN_RETURN "return"
%token <pos> TKN_LPAR "(" TKN_RPAR ")" TKN_LBRAC "{" TKN_RBRAC "}" TKN_SEMI ";" TKN_COMMA ","
%token <ival> TKN_NUM
%token <ident> TKN_IDENT
%token <garbage> TKN_GARBAGE
%token <pos> TKN_INC "++" TKN_DEC "--" TKN_ADD "+" TKN_SUB "-" TKN_ASSIGN "="
%token <pos> TKN_MUL "*" TKN_DIV "/" TKN_REM "%"
%token TKN_THEN "then"
%token <pos> TKN_STRUCT "struct" TKN_ENUM "enum" TKN_VOID "void"
%token <pos> TKN_SIGNED "signed" TKN_UNSIGNED "unsigned" TKN_CHAR "char" TKN_SHORT "short" TKN_LONG "long" TKN_INT "int"
%token <pos> TKN_FLOAT "float" TKN_DOUBLE "double"

%type <func> funcdef
%type <vars> funcparams
%type <vars> opt_funcparams
%type <var> varassign
%type <var> varassign_t
%type <vars> vardecl
%type <vars> vardecls
%type <statmt> statmt
%type <statmts> statmts
%type <statmt> opt_else
%type <expr> expr
%type <exprs> exprs
%type <exprs> opt_exprs
// TODO: Make type_spec actually interesting.
%type <type> type_spec
%type <type> int_spec
%type <pos> opt_int

%right "="
%left "+" "-"
%left "*" "/" "%"
%precedence "++" "--"
%precedence "("

%precedence "then"
%precedence "else"

%%
library:		{init(ctx);}		funcdefs;
funcdef:		type_spec TKN_IDENT "(" opt_funcparams ")" "{" statmts "}" 					   {$$=func_impl(ctx, $1, $2, &$4, &$7);$$.pos = pos_merge($1.pos, $5);}
|				error ")" "{" statmts "}"													   {printf("err\n");/*dispose of stuff*/};
funcdefs:		funcdefs funcdef
|				%empty;
opt_funcparams:	funcparams							{$$=$1;}
|				%empty								{$$=param_empty  (ctx);						$$.pos=pos_empty(ctx->tokeniser_ctx);};
funcparams:		funcparams "," type_spec TKN_IDENT	{$$=param_cat    (ctx, &$1, $4, $3);		$$.pos=pos_merge($1.pos, $4.pos);}
|				type_spec TKN_IDENT					{$$=param_new    (ctx, $2, $1);				$$.pos=pos_merge($1.pos, $2.pos);}
|				funcparams error TKN_IDENT			{$$=param_empty  (ctx);						$$.pos=pos_merge($1.pos, $3.pos);	report_error(ctx, "Syntax error", $$.pos, "Expected parameter");}
|				type_spec error TKN_IDENT			{$$=param_empty  (ctx); 					$$.pos=pos_merge($1.pos, $3.pos);	report_error(ctx, "Syntax error", $$.pos, "Expected parameter");};
vardecl:		varassign_t							{$$=vars_new     (ctx, &$1);				$$.pos=$1.pos;}
|				varassign_t "," vardecls			{$$=vars_cat     (ctx, &$3, &$1);			$$.pos=pos_merge($1.pos, $3.pos);};
vardecls:		varassign							{$$=vars_new     (ctx, &$1);				$$.pos=$1.pos;}
|				vardecls "," varassign				{$$=vars_cat     (ctx, &$1, &$3);			$$.pos=pos_merge($1.pos, $3.pos);};
varassign_t:	type_spec TKN_IDENT "=" expr		{$$=declt_assign (ctx, $2, $1, &$4);		$$.pos=pos_merge($1.pos, $4.pos);}
|				type_spec TKN_IDENT					{$$=declt        (ctx, $2, $1);				$$.pos=pos_merge($1.pos, $2.pos);};
varassign:		varassign_t							{$$=$1;}
|				TKN_IDENT "=" expr					{$$=decl_assign  (ctx, $1, &$3);			$$.pos=pos_merge($1.pos, $3.pos);}
|				TKN_IDENT							{$$=decl         (ctx, $1);					$$.pos=$1.pos;};
statmt:			"{" statmts "}"						{$$=statmt_multi (ctx, &$2);				$$.pos=pos_merge($1, $3);}
|				"if"	"(" expr ")" statmt
				opt_else							{$$=statmt_if    (ctx, &$3, &$5, &$6);		$$.pos=pos_merge($1, $6.pos);}
|				"while"	"(" expr ")" statmt			{$$=statmt_while (ctx, &$3, &$5);			$$.pos=pos_merge($1, $5.pos);}
|				"for" "(" statmt expr
								";" expr ")" statmt	{$$=statmt_for   (ctx, &$3, &$4, &$6, &$8);	$$.pos=pos_merge($1, $8.pos);}
|				"return" expr	";"					{$$=statmt_ret   (ctx, &$2);				$$.pos=pos_merge($1, $3);}
|				vardecl			";"					{$$=statmt_var   (ctx, &$1);				$$.pos=pos_merge($1.pos, $2);}
|				expr			";"					{$$=statmt_expr  (ctx, &$1);				$$.pos=pos_merge($1.pos, $2);}
|				";"									{$$=statmt_nop   (ctx);						$$.pos=$1;};
statmts:		%empty								{$$=statmts_new  (ctx);						$$.pos=pos_empty(ctx->tokeniser_ctx);}
|				statmts statmt						{$$=statmts_cat  (ctx, &$1, &$2);			$$.pos=pos_merge($1.pos, $2.pos);};
expr:			TKN_IDENT							{$$=expr_var     (ctx, $1);					$$.pos=$1.pos;}
|				TKN_NUM								{$$=expr_const   (ctx, $1);					$$.pos=$1.pos;}
|				"(" expr ")"						{$$=$2;										$$.pos=pos_merge($1, $3);}
|				expr "(" opt_exprs ")"				{$$=expr_call    (ctx, &$1, &$3);			$$.pos=pos_merge($1.pos, $4);}
|				expr "=" expr						{$$=expr_assign  (ctx, &$1, &$3);			$$.pos=pos_merge($1.pos, $3.pos);}
|				expr "+" expr						{$$=expr_math2   (ctx, &$1, &$3, OP_ADD);	$$.pos=pos_merge($1.pos, $3.pos);}
|				expr "-" expr		%prec "+"		{$$=expr_math2   (ctx, &$1, &$3, OP_SUB);	$$.pos=pos_merge($1.pos, $3.pos);}
|				expr "*" expr						{$$=expr_math2   (ctx, &$1, &$3, OP_MUL);	$$.pos=pos_merge($1.pos, $3.pos);}
|				expr "/" expr		%prec "*"		{$$=expr_math2   (ctx, &$1, &$3, OP_DIV);	$$.pos=pos_merge($1.pos, $3.pos);}
|				expr "%" expr		%prec "*"		{$$=expr_math2   (ctx, &$1, &$3, OP_REM);	$$.pos=pos_merge($1.pos, $3.pos);}
|				expr "++"							{$$=expr_math1   (ctx, &$1, OP_INC);		$$.pos=pos_merge($1.pos, $2);}
|				expr "--"			%prec "++"		{$$=expr_math1   (ctx, &$1, OP_DEC);		$$.pos=pos_merge($1.pos, $2);};
exprs:			exprs "," expr						{$$=exprs_cat    (ctx, &$1, &$3);			$$.pos=pos_merge($1.pos, $3.pos);}
|				expr								{$$=exprs_new    (ctx, &$1);				$$.pos=$1.pos;};
opt_exprs:		exprs								{$$=$1;										$$.pos=$1.pos;}
|				%empty								{$$=exprs_new    (ctx, NULL);				$$.pos=pos_empty(ctx->tokeniser_ctx);};
opt_else:		"else" statmt						{$$=$2;										$$.pos=pos_merge($1, $2.pos);}
|				%empty				%prec "then"	{$$=statmt_nop   (ctx);						$$.pos=pos_empty(ctx->tokeniser_ctx);};
// TODO: Make type_spec actually interesting.
type_spec:		"void"								{$$=type_void    (ctx);						$$.pos=$1;}
|				"signed" int_spec					{$$=type_signed  (ctx, &$2);				$$.pos=pos_merge($1, $2.pos);}
|				"unsigned" int_spec					{$$=type_unsigned(ctx, &$2);				$$.pos=pos_merge($1, $2.pos);}
|				int_spec							{$$=$1;}
|				"float"								{$$=type_number  (ctx, FLOAT);				$$.pos=$1;}
|				"double"							{$$=type_number  (ctx, DOUBLE_FLOAT);		$$.pos=$1;}
|				"long" "double"						{$$=type_number  (ctx, QUAD_FLOAT);			$$.pos=pos_merge($1, $2);}
|				"struct" TKN_IDENT					{$$.pos=pos_merge($1, $2.pos);}
|				"enum" TKN_IDENT					{$$.pos=pos_merge($1, $2.pos);};
int_spec:		"char"								{$$=type_number  (ctx, NUM_HHI);			$$.pos=$1;}
|				"short" opt_int						{$$=type_number  (ctx, NUM_HI);				$$.pos=pos_merge($1, $2);}
|				"int"								{$$=type_number  (ctx, NUM_I);				$$.pos=$1;}
|				"long" opt_int						{$$=type_number  (ctx, NUM_LI);				$$.pos=pos_merge($1, $2);}
|				"long" "long" opt_int				{$$=type_number  (ctx, NUM_LLI);			$$.pos=pos_merge($1, $3);};
opt_int:		"int"								{$$=$1;}
|				%empty								{$$=pos_empty(ctx->tokeniser_ctx);};
%%

static void *make_copy(void *mem, size_t size) {
	void *copy = malloc(size);
	memcpy(copy, mem, size);
	return copy;
}

void init(parser_ctx_t *ctx) {
	ctx->currentError = NULL;
	map_create(&ctx->funcMap);
	map_create(&ctx->varMap);
}


vardecls_t param_cat(parser_ctx_t *ctx, vardecls_t *other, ident_t ident, type_t type) {
	vardecl_t var = declt(ctx, ident, type);
	return vars_cat(ctx, other, &var);
}

vardecls_t param_new(parser_ctx_t *ctx, ident_t ident, type_t type) {
	vardecl_t var = declt(ctx, ident, type);
	return vars_new(ctx, &var);
}

vardecls_t param_empty(parser_ctx_t *ctx) {
	return (vardecls_t) {
		.num = 0,
		.vars = NULL
	};
}

funcdef_t func_impl(parser_ctx_t *ctx, type_t returns, ident_t ident, vardecls_t *params, statements_t *statmt) {
	funcdef_t f = {
		.returns = returns,
		.ident = ident,
		.numParams = params->num,
		.params = params->vars,
		.statements = statmt
	};
	funcdef_t *existing = map_get(&ctx->funcMap, ident.ident);
	if (existing) {
		// TODO: Standard error message.
		yyerror(ctx, "Function is already declared.");
	} else {
		funcdef_t *add = malloc(sizeof(funcdef_t));
		*add = f;
		map_set(&ctx->funcMap, ident.ident, add);
	}
	function_added(ctx, &f);
	return f;
}


vardecl_t decl_assign(parser_ctx_t *ctx, ident_t ident, expression_t *expression) {
	if (expression) expression = make_copy(expression, sizeof(expression_t));
	vardecl_t f = {
		.ident = ident,
		.type = ctx->currentVarType,
		.expr = expression
	};
	/* vardecl_t *existing = map_get(&ctx->varMap, ident.ident);
	if (existing) {
		char buf[20 + strlen(ident.ident)];
		sprintf(buf, "Redefinition of '%s'", ident.ident);
		report_error(ctx, "Error", ident.pos, buf);
	} else {
		vardecl_t *add = malloc(sizeof(vardecl_t));
		*add = f;
		map_set(&ctx->varMap, ident.ident, add);
	} */
	return f;
}

vardecl_t decl(parser_ctx_t *ctx, ident_t ident) {
	return decl_assign(ctx, ident, NULL);
}

vardecl_t declt_assign(parser_ctx_t *ctx, ident_t ident, type_t type, expression_t *expression) {
	if (expression) expression = make_copy(expression, sizeof(expression_t));
	ctx->currentVarType = type;
	vardecl_t f = {
		.ident = ident,
		.type = type,
		.expr = expression
	};
	/* vardecl_t *existing = map_get(&ctx->varMap, ident.ident);
	if (existing) {
		char buf[20 + strlen(ident.ident)];
		sprintf(buf, "Redefinition of '%s'", ident.ident);
		report_error(ctx, "Error", ident.pos, buf);
	} else {
		vardecl_t *add = malloc(sizeof(vardecl_t));
		*add = f;
		map_set(&ctx->varMap, ident.ident, add);
	} */
	return f;
}

vardecl_t declt(parser_ctx_t *ctx, ident_t ident, type_t type) {
	return declt_assign(ctx, ident, type, NULL);
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


expression_t expr_var(parser_ctx_t *ctx, ident_t ident) {
	return (expression_t) {
		.type = EXPR_TYPE_IDENT,
		.ident = ident.ident,
		.expr = NULL,
		.expr1 = NULL,
		.exprs = NULL,
		.value = 0
	};
}

expression_t expr_const(parser_ctx_t *ctx, ival_t iconst) {
	return (expression_t) {
		.type = EXPR_TYPE_ICONST,
		.ident = NULL,
		.expr = NULL,
		.expr1 = NULL,
		.exprs = NULL,
		.value = iconst.ival
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


type_t type_void(parser_ctx_t *ctx) {
	return (type_t) {
		.type_spec = {
			.type = VOID,
			.size = 0
		}
	};
}

type_t type_number(parser_ctx_t *ctx, type_simple_t type) {
	int size;
	switch (type) {
		case (NUM_HHU):
		case (NUM_HHI):
			size = CHAR_BYTES;
			break;
		case (NUM_HU):
		case (NUM_HI):
			size = SHORT_BYTES;
			break;
		case (NUM_U):
		case (NUM_I):
			size = INT_BYTES;
			break;
		case (NUM_LU):
		case (NUM_LI):
			size = LONG_BYTES;
			break;
		case (NUM_LLU):
		case (NUM_LLI):
			size = LONG_LONG_BYTES;
			break;
	}
	return (type_t) {
		.type_spec = {
			.type = type,
			.size = size
		}
	};
}

type_t type_signed(parser_ctx_t *ctx, type_t *other) {
	type_simple_t type = other->type_spec.type;
	int size = other->type_spec.size;
	if (type >= NUM_HHU && type <= NUM_LLU) {
		type = type - NUM_HHU + NUM_HHI;
	}
	return (type_t) {
		.type_spec = {
			.type = type,
			.size = size
		}
	};
}

type_t type_unsigned(parser_ctx_t *ctx, type_t *other) {
	type_simple_t type = other->type_spec.type;
	int size = other->type_spec.size;
	if (type >= NUM_HHI && type <= NUM_LLI) {
		type = type - NUM_HHI + NUM_HHU;
	}
	return (type_t) {
		.type_spec = {
			.type = type,
			.size = size
		}
	};
}

