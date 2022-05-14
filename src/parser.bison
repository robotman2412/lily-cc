
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
	pos_t			pos;
	
	ival_t			ival;
	strval_t		strval;
	
	ident_t			ident;
	idents_t		idents;
	
	funcdef_t		func;
	
	expr_t			expr;
	exprs_t			exprs;
	
	stmt_t			stmt;
	stmts_t			stmts;
	
	iasm_qual_t		asm_qual;
	iasm_regs_t		asm_regs;
	iasm_reg_t		asm_reg;
	
	ival_t			simple_type;
	
	strval_t		garbage;
}

%token <pos> TKN_UNSIGNED "unsigned" TKN_SIGNED "signed" TKN_FLOAT "float" TKN_DOUBLE "double" TKN_BOOL "_Bool"
%token <pos> TKN_CHAR "char" TKN_SHORT "short" TKN_INT "int" TKN_LONG "long"
%token <pos> TKN_VOID "void"
%token <pos> TKN_VOLATILE "volatile" TKN_INLINE "inline" TKN_GOTO "goto"
%token <pos> TKN_IF "if" TKN_ELSE "else" TKN_WHILE "while" TKN_RETURN "return" TKN_ASM "asm"
%token TKN_THEN "then"
%token <pos> TKN_LPAR "(" TKN_RPAR ")" TKN_LBRAC "{" TKN_RBRAC "}" TKN_LSBRAC "[" TKN_RSBRAC "]"
%token <pos> TKN_SEMI ";" TKN_COLON ":" TKN_COMMA ","

%token <ival> TKN_IVAL
%token <strval> TKN_STRVAL
%token <strval> TKN_IDENT
%token <garbage> TKN_GARBAGE

%token <pos> TKN_ASSIGN_ADD "+=" TKN_ASSIGN_SUB "-="
%token <pos> TKN_ASSIGN_SHL "<<=" TKN_ASSIGN_SHR ">>="
%token <pos> TKN_ASSIGN_MUL "*=" TKN_ASSIGN_DIV "/=" TKN_ASSIGN_REM "%="
%token <pos> TKN_ASSIGN_AND "&=" TKN_ASSIGN_OR "|=" TKN_ASSIGN_XOR "^="
%token <pos> TKN_INC "++" TKN_DEC "--"
%token <pos> TKN_LOGIC_AND "&&" TKN_LOGIC_OR "||"
%token <pos> TKN_ADD "+" TKN_SUB "-" TKN_ASSIGN "=" TKN_AMP "&"
%token <pos> TKN_MUL "*" TKN_DIV "/" TKN_REM "%"
%token <pos> TKN_NOT "!" TKN_INV "~" TKN_XOR "^" TKN_OR "|"
%token <pos> TKN_SHL "<<" TKN_SHR ">>"
%token <pos> TKN_LT "<" TKN_LE "<=" TKN_GT ">" TKN_GE ">=" TKN_EQ "==" TKN_NE "!="

%type <idents> opt_params
%type <idents> params
%type <idents> idents
%type <func> funcdef
%type <expr> expr
%type <exprs> opt_exprs
%type <exprs> exprs

%type <stmts> stmts
%type <stmt> stmt
%type <stmt> inline_asm
%type <stmt> asm_code
%type <asm_qual> asm_qual
%type <asm_regs> opt_asm_regs
%type <asm_regs> asm_regs
%type <asm_reg> asm_reg

%type <simple_type> simple_long
%type <simple_type> simple_type
%type <idents> vardecls
%type <pos> opt_int opt_signed

// Precedence: lowest.

%right "=" "+=" "-=" "*=" "/=" "%=" "&=" "|=" "^=" "<<=" ">>="

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

// Everything that could happen in a global scope.
global:			funcdef {function_added(ctx, &$1);}
|				vardecls;

opt_int:		"int"										{$$=$1;}
|				%empty										{$$=pos_empty(ctx->tokeniser_ctx);};
opt_signed:		"signed"									{$$=$1;}
|				%empty										{$$=pos_empty(ctx->tokeniser_ctx);};

// Used to disambiguate the presense of "long".
simple_long:	opt_int										{$$.pos=$1; $$.ival=STYPE_S_LONG;}
|				"double"									{$$.pos=$1; $$.ival=STYPE_LONG_DOUBLE;}
|				"long" opt_int								{$$.pos=pos_merge($1, $2); $$.ival=STYPE_S_LONGER;};
// Simple types: Non-pointer, non-array types.
simple_type:	"char"										{$$.pos=$1; $$.ival=STYPE_CHAR;}
|				"signed" "char"								{$$.pos=pos_merge($1, $2); $$.ival=STYPE_S_CHAR;}
|				"unsigned" "char"							{$$.pos=pos_merge($1, $2); $$.ival=STYPE_U_CHAR;}
|				opt_signed "short" opt_int					{$$.pos=pos_merge($1, $3); $$.ival=STYPE_S_SHORT;}
|				opt_signed "int"							{$$.pos=pos_merge($1, $2); $$.ival=STYPE_S_INT;}
|				"signed" "long" opt_int						{$$.pos=pos_merge($1, $3); $$.ival=STYPE_S_LONG;}
|				"long" simple_long							{$$=$2; $$.pos=pos_merge($1, $2.pos);}
|				"signed" "long" "long" opt_int				{$$.pos=pos_merge($1, $4); $$.ival=STYPE_S_LONGER;}
|				"unsigned" "short" opt_int					{$$.pos=pos_merge($1, $3); $$.ival=STYPE_U_SHORT;}
|				"unsigned" "int"							{$$.pos=pos_merge($1, $2); $$.ival=STYPE_U_INT;}
|				"unsigned" "long" opt_int					{$$.pos=pos_merge($1, $3); $$.ival=STYPE_U_LONG;}
|				"unsigned" "long" "long" opt_int			{$$.pos=pos_merge($1, $4); $$.ival=STYPE_U_LONGER;}
|				"float"										{$$.pos=$1; $$.ival=STYPE_FLOAT;}
|				"double"									{$$.pos=$1; $$.ival=STYPE_DOUBLE;}
|				"_Bool"										{$$.pos=$1; $$.ival=STYPE_BOOL;}
|				"void"										{$$.pos=$1; $$.ival=STYPE_VOID;};

// A function definition (with code).
funcdef:		simple_type TKN_IDENT "(" opt_params ")"
				"{" stmts "}"								{$$=funcdef_decl(&$2, &$4, &$7); $$.pos=pos_merge($1.pos, $8);}
// A function definition (without code).
|				simple_type TKN_IDENT "(" opt_params ")"
				";"											{$$=funcdef_def(&$2, &$4); $$.pos=pos_merge($1.pos, $6);};
// One or more variable declarations.
vardecls:		simple_type idents ";"						{$$=$2; idents_settype(&$$, $1.ival); $$.pos=pos_merge($1.pos, $3);};

// Function parameters.
opt_params:		params										{$$=$1;}
|				%empty										{$$=idents_empty(); $$.pos=pos_empty(ctx->tokeniser_ctx);};
params:			params "," simple_type TKN_IDENT			{$$=idents_cat  (&$1,  &$3.ival, &$4); $$.pos=pos_merge($1.pos, $4.pos);}
|				simple_type TKN_IDENT						{$$=idents_one  (&$1.ival, &$2); $$.pos=pos_merge($1.pos, $2.pos);};
idents:			idents "," TKN_IDENT						{$$=idents_cat  (&$1,  NULL, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				TKN_IDENT									{$$=idents_one  (NULL, &$1); $$.pos=$1.pos;};

// Statements.
stmts:			stmts stmt									{$$=stmts_cat   (&$1, &$2); $$.pos=pos_merge($1.pos, $2.pos);}
|				%empty										{$$=stmts_empty (); $$.pos=pos_empty(ctx->tokeniser_ctx);};
stmt:			"{" stmts "}"								{$$=stmt_multi  (&$2); $$.pos=pos_merge($1, $3);}
|				"if" "(" expr ")" stmt		%prec "then"	{$$=stmt_if     (&$3, &$5, NULL); $$.pos=pos_merge($1, $5.pos);}
|				"if" "(" expr ")" stmt
				"else" stmt									{$$=stmt_if     (&$3, &$5, &$7); $$.pos=pos_merge($1, $7.pos);}
|				"while" "(" expr ")" stmt					{$$=stmt_while  (&$3, &$5); $$.pos=pos_merge($1, $5.pos);}
|				"return" ";"								{$$=stmt_ret    (NULL); $$.pos=pos_merge($1, $2);}
|				"return" expr ";"							{$$=stmt_ret    (&$2); $$.pos=pos_merge($1, $3);}
|				vardecls									{$$=stmt_var    (&$1); $$.pos=$1.pos;}
|				expr ";"									{$$=stmt_expr   (&$1);  $$.pos=pos_merge($1.pos, $2);}
|				inline_asm ";"								{$$=$1; $$.pos=pos_merge($1.pos, $2);};

// Expressions.
opt_exprs:		exprs										{$$=$1;}
|				%empty										{$$=exprs_empty();};
exprs:			exprs "," expr								{$$=exprs_cat(&$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr										{$$=exprs_one(&$1); $$.pos=$1.pos;};
expr:			TKN_IVAL									{$$=expr_icnst(&$1); $$.pos=$1.pos;}
|				TKN_STRVAL									{$$=expr_scnst(&$1); $$.pos=$1.pos;}
|				TKN_IDENT									{$$=expr_ident(&$1); $$.pos=$1.pos;}
|				expr "(" opt_exprs ")"						{$$=expr_call (&$1, &$3); $$.pos=pos_merge($1.pos, $4);}
|				expr "[" expr "]"							{$$=expr_math2(OP_INDEX,     &$1, &$3); $$.pos=pos_merge($1.pos, $4);}
|				"(" expr ")"								{$$=$2; $$.pos=pos_merge($1, $3);}

|				"-" expr									{$$=expr_math1(OP_0_MINUS,   &$2); $$.pos=pos_merge($1, $2.pos);}
|				"!" expr									{$$=expr_math1(OP_LOGIC_NOT, &$2); $$.pos=pos_merge($1, $2.pos);}
|				"~" expr									{$$=expr_math1(OP_BIT_NOT,   &$2); $$.pos=pos_merge($1, $2.pos);}
|				"&" expr									{$$=expr_math1(OP_ADROF,     &$2); $$.pos=pos_merge($1, $2.pos);}
|				"*" expr									{$$=expr_math1(OP_DEREF,     &$2); $$.pos=pos_merge($1, $2.pos);}

|				expr "*" expr								{$$=expr_math2(OP_MUL,       &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "/" expr				%prec "*"		{$$=expr_math2(OP_DIV,       &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "%" expr				%prec "*"		{$$=expr_math2(OP_MOD,       &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}

|				expr "+" expr								{$$=expr_math2(OP_ADD,       &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "-" expr				%prec "+"		{$$=expr_math2(OP_SUB,       &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}

|				expr "<" expr								{$$=expr_math2(OP_LT,        &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "<=" expr				%prec "<"		{$$=expr_math2(OP_LE,        &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr ">" expr				%prec "<"		{$$=expr_math2(OP_GT,        &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr ">=" expr				%prec "<"		{$$=expr_math2(OP_GE,        &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}

|				expr "==" expr								{$$=expr_math2(OP_NE,        &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "!=" expr				%prec "=="		{$$=expr_math2(OP_EQ,        &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}

|				expr "&" expr								{$$=expr_math2(OP_BIT_AND,   &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "^" expr								{$$=expr_math2(OP_BIT_XOR,   &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "|" expr								{$$=expr_math2(OP_BIT_OR,    &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "<<" expr								{$$=expr_math2(OP_SHIFT_L,   &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr ">>" expr				%prec "<<"		{$$=expr_math2(OP_SHIFT_R,   &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}

|				expr "&&" expr								{$$=expr_math2(OP_LOGIC_AND, &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "||" expr								{$$=expr_math2(OP_LOGIC_OR,  &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}

|				expr "=" expr								{$$=expr_math2(OP_ASSIGN,    &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "+=" expr				%prec "="		{$$=expr_matha(OP_ADD,       &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "-=" expr				%prec "="		{$$=expr_matha(OP_SUB,       &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "*=" expr				%prec "="		{$$=expr_matha(OP_MUL,       &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "/=" expr				%prec "="		{$$=expr_matha(OP_DIV,       &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "%=" expr				%prec "="		{$$=expr_matha(OP_MOD,       &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "&=" expr				%prec "="		{$$=expr_matha(OP_BIT_AND,   &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "|=" expr				%prec "="		{$$=expr_matha(OP_BIT_OR,    &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "^=" expr				%prec "="		{$$=expr_matha(OP_BIT_XOR,   &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "<<=" expr				%prec "="		{$$=expr_matha(OP_SHIFT_L,   &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr ">>=" expr				%prec "="		{$$=expr_matha(OP_SHIFT_R,   &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);};

// Inline assembly snippets.
inline_asm:		"asm" asm_qual asm_code						{$$=$3; $$.iasm->qualifiers=$2; $$.iasm->qualifiers.is_volatile |= !$$.iasm->outputs; };

asm_qual:		%empty										{$$=(iasm_qual_t) {.is_volatile=0, .is_inline=0, .is_goto=0}; $$.pos=pos_empty(ctx->tokeniser_ctx);}
|				asm_qual "volatile"							{$$=$1; $$.is_volatile=1; $$.pos=pos_merge($1.pos, $2);}
|				asm_qual "inline"							{$$=$1; $$.is_inline=1;   $$.pos=pos_merge($1.pos, $2);}
|				asm_qual "goto"								{$$=$1; $$.is_goto=1;     $$.pos=pos_merge($1.pos, $2);};

asm_code:		"(" TKN_STRVAL
				":" opt_asm_regs
				":" opt_asm_regs
				":" opt_asm_strs ")"						{$$=stmt_iasm(&$2, &$4,  &$6,  NULL); $$.pos=pos_merge($1, $9);}
|				"(" TKN_STRVAL
				":" opt_asm_regs
				":" opt_asm_regs ")"						{$$=stmt_iasm(&$2, &$4,  &$6,  NULL); $$.pos=pos_merge($1, $7);}
|				"(" TKN_STRVAL
				":" opt_asm_regs ")"						{$$=stmt_iasm(&$2, &$4,  NULL, NULL); $$.pos=pos_merge($1, $5);}
|				"(" TKN_STRVAL ")"							{$$=stmt_iasm(&$2, NULL, NULL, NULL); $$.pos=pos_merge($1, $3);};

opt_asm_strs:	asm_strs
|				%empty;
asm_strs:		TKN_STRVAL "," asm_strs
|				TKN_STRVAL;
opt_asm_regs:	asm_regs									{$$=$1;}
|				%empty										{$$=iasm_regs_empty();             $$.pos=pos_empty(ctx->tokeniser_ctx);};
asm_regs:		asm_reg "," asm_regs						{$$=iasm_regs_cat(&$3, &$1);       $$.pos=pos_merge($1.pos, $3.pos);}
|				asm_reg										{$$=iasm_regs_one(&$1);            $$.pos=$1.pos;};
asm_reg:		"[" TKN_IDENT "]" TKN_STRVAL "(" expr ")"	{$$=stmt_iasm_reg(&$2,  &$4, &$6); $$.pos=pos_merge($1, $7);}
|				TKN_STRVAL "(" expr ")"						{$$=stmt_iasm_reg(NULL, &$1, &$3); $$.pos=pos_merge($1.pos, $4);};
%%

void *make_copy(void *mem, size_t size) {
	if (!mem) return NULL;
	void *copy = malloc(size);
	memcpy(copy, mem, size);
	return copy;
}
