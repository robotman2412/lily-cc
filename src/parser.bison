
%require "3.5"

%{
#define YYDEBUG 1
%}

%code requires {
#include "parser-util.h"
#include "debug/pront.h"
#include <malloc.h>
#include <string.h>

extern int  yylex  (parser_ctx_t *ctx);
extern void yyerror(parser_ctx_t *ctx, char *msg);

}

%param { parser_ctx_t *ctx };

%union {
	pos_t		pos;
	
	ival_t		ival;
	strval_t	strval;
	
	ident_t		ident;
	idents_t	idents;
	
	funcdef_t	func;
	
	expr_t		expr;
	exprs_t		exprs;
	
	stmt_t		stmt;
	stmts_t		stmts;
	
	strval_t	garbage;
}

%token <pos> TKN_AUTO "auto" TKN_FUNC "func"
%token <pos> TKN_IF "if" TKN_ELSE "else" TKN_WHILE "while" TKN_RETURN "return" TKN_ASM "asm"
%token TKN_THEN "then"
%token <pos> TKN_LPAR "(" TKN_RPAR ")" TKN_LBRAC "{" TKN_RBRAC "}" TKN_LSBRAC "[" TKN_RSBRAC "]"
%token <pos> TKN_SEMI ";" TKN_COLON ":" TKN_COMMA ","

%token <ival> TKN_IVAL
%token <strval> TKN_STRVAL
%token <ident> TKN_IDENT
%token <garbage> TKN_GARBAGE

%token <pos> TKN_ADD "+" TKN_SUB "-" TKN_ASSIGN "=" TKN_AMP "&"
%token <pos> TKN_MUL "*" TKN_DIV "/" TKN_REM "%"
%token <pos> TKN_NOT "!" TKN_INV "~" TKN_XOR "^" TKN_OR "|"
%token <pos> TKN_SHL "<<" TKN_SHR ">>"
%token <pos> TKN_LT "<" TKN_LE "<=" TKN_GT ">" TKN_GE ">=" TKN_EQ "==" TKN_NE "!="

%type <idents> opt_idents
%type <idents> idents
%type <func> funcdef
%type <expr> expr
%type <exprs> opt_exprs
%type <exprs> exprs

%type <stmts> stmts
%type <stmt> stmt

%type <idents> vardecls

// Precedence: lowest.

%right "="

%left "||"
%left "&&"

%left "|"
%left "^"
//%left "&"

%left "==" "!="
%left "<" ">" "<=" ">="
%left "<<" ">>"

%left "+" //"-"
%left "%" "/" //"*"

%right "!" "~" "*" "&" "-"

%precedence "(" "["

%precedence "then"
%precedence "else"

// Precedence: highest.

%%
library:		global library
|				%empty;

global:			funcdef {function_added(ctx, &$1);}
|				vardecls;

funcdef:		"func" TKN_IDENT "(" opt_idents ")"
				"{" stmts "}"								{$$=funcdef_decl(&$2, &$4, &$7);};
vardecls:		"auto" idents ";"							{$$=$2;};

opt_idents:		idents										{$$=$1;}
|				%empty										{$$=idents_empty();};
idents:			idents "," TKN_IDENT						{$$=idents_cat  (&$1,  &$3);}
|				TKN_IDENT									{$$=idents_one  (&$1);};
stmts:			stmts stmt									{$$=stmts_cat   (&$1, &$2);}
|				%empty										{$$=stmts_empty ();};
stmt:			"{" stmts "}"                               {$$=stmt_multi  (&$2);}
|				"if" "(" expr ")" stmt		%prec "then"    {$$=stmt_if     (&$3, &$5, NULL);}
|				"if" "(" expr ")" stmt
				"else" stmt                                 {$$=stmt_if     (&$3, &$5, &$7);}
|				"while" "(" expr ")" stmt                   {$$=stmt_while  (&$3, &$5);}
|				"return" ";"                                {$$=stmt_ret    (NULL);}
|				"return" expr ";"                           {$$=stmt_ret    (&$2);}
|				vardecls                                    {$$=stmt_var    (&$1);}
|				expr ";"                                    {$$=stmt_expr   (&$1);}
|				inline_asm ";"                              {/*$$=stmt_iasm   (&$1);*/};

opt_exprs:		exprs
|				%empty;
exprs:			expr "," exprs
|				expr;
expr:			TKN_IVAL									{$$=expr_icnst(&$1);}
|				TKN_STRVAL									{$$=expr_scnst(&$1);}
|				TKN_IDENT									{$$=expr_ident(&$1);}
|				expr "(" opt_exprs ")"						{$$=expr_call (&$1, &$3);}
|				expr "[" expr "]"							{$$=expr_math2(OP_INDEX,     &$1, &$3);}
|				"(" expr ")"								{$$=$2;}

|				"-" expr									{$$=expr_math1(OP_0_MINUS,   &$2);}
|				"!" expr									{$$=expr_math1(OP_LOGIC_NOT, &$2);}
|				"~" expr									{$$=expr_math1(OP_BIT_NOT,   &$2);}
|				"&" expr									{$$=expr_math1(OP_ADROF,     &$2);}
|				"*" expr									{$$=expr_math1(OP_DEREF,     &$2);}

|				expr "*" expr								{$$=expr_math2(OP_MUL,       &$1, &$3);}
|				expr "/" expr				%prec "*"		{$$=expr_math2(OP_DIV,       &$1, &$3);}
|				expr "%" expr				%prec "*"		{$$=expr_math2(OP_MOD,       &$1, &$3);}

|				expr "+" expr								{$$=expr_math2(OP_ADD,       &$1, &$3);}
|				expr "-" expr				%prec "+"		{$$=expr_math2(OP_SUB,       &$1, &$3);}

|				expr "<" expr								{$$=expr_math2(OP_LT,        &$1, &$3);}
|				expr "<=" expr				%prec "<"		{$$=expr_math2(OP_LE,        &$1, &$3);}
|				expr ">" expr				%prec "<"		{$$=expr_math2(OP_GT,        &$1, &$3);}
|				expr ">=" expr				%prec "<"		{$$=expr_math2(OP_GE,        &$1, &$3);}

|				expr "==" expr								{$$=expr_math2(OP_NE,        &$1, &$3);}
|				expr "!=" expr				%prec "=="		{$$=expr_math2(OP_EQ,        &$1, &$3);}

|				expr "&" expr								{$$=expr_math2(OP_BIT_AND,   &$1, &$3);}
|				expr "^" expr								{$$=expr_math2(OP_BIT_XOR,   &$1, &$3);}
|				expr "|" expr								{$$=expr_math2(OP_BIT_OR,    &$1, &$3);}
|				expr "<<" expr								{$$=expr_math2(OP_SHIFT_L,   &$1, &$3);}
|				expr ">>" expr				%prec "<<"		{$$=expr_math2(OP_SHIFT_R,   &$1, &$3);}

|				expr "&&" expr								{$$=expr_math2(OP_LOGIC_AND, &$1, &$3);}
|				expr "||" expr								{$$=expr_math2(OP_LOGIC_OR,  &$1, &$3);}
|				expr "=" expr								{$$=expr_math2(OP_ASSIGN,    &$1, &$3);};

inline_asm:		"asm" "(" TKN_STRVAL
				":" opt_asm_regs
				":" opt_asm_regs
				":" opt_asm_strs ")"
|				"asm" "(" TKN_STRVAL
				":" opt_asm_regs
				":" opt_asm_regs ")"
|				"asm" "(" TKN_STRVAL
				":" opt_asm_regs ")"
|				"asm" "(" TKN_STRVAL ")";

opt_asm_strs:	asm_strs
|				%empty;
asm_strs:		TKN_STRVAL "," asm_strs
|				TKN_STRVAL;
opt_asm_regs:	asm_regs
|				%empty;
asm_regs:		TKN_STRVAL "(" expr ")" "," asm_regs
|				TKN_STRVAL "(" expr ")";
%%

void *make_copy(void *mem, size_t size) {
	if (!mem) return NULL;
	void *copy = malloc(size);
	memcpy(copy, mem, size);
	return copy;
}
