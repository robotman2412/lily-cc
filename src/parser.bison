
%require "3.5"

%{
#define YYDEBUG 1
%}

%code requires {

#include "parser-util.h"
#include "debug/pront.h"
#include <malloc.h>
#include <string.h>
#include <gen_util.h>

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
	
	attribute_t     attr;
	
	strval_t		garbage;
}

%token <pos> TKN_UNSIGNED "unsigned" TKN_SIGNED "signed" TKN_FLOAT "float" TKN_DOUBLE "double" TKN_BOOL "_Bool"
%token <pos> TKN_CHAR "char" TKN_SHORT "short" TKN_INT "int" TKN_LONG "long"
%token <pos> TKN_VOID "void"
%token <pos> TKN_VOLATILE "volatile" TKN_INLINE "inline" TKN_GOTO "goto"
%token <pos> TKN_IF "if" TKN_ELSE "else" TKN_WHILE "while" TKN_RETURN "return" TKN_FOR "for" TKN_ASM "asm"
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
%token <attr> TKN_ATTR "__attribute__"

%type <idents> opt_params
%type <idents> params
%type <idents> idents
%type <ident> var_nonarr
%type <ident> var
%type <ident> var_stars
%type <ident> var_arrays
%type <func> funcdef
%type <expr> expr
%type <exprs> opt_exprs
%type <exprs> exprs

%type <stmts> stmts
%type <stmt> stmt
%type <stmt> stmt_no_stmts
%type <stmt> inline_asm
%type <stmt> asm_code
%type <asm_qual> asm_qual
%type <asm_regs> opt_asm_regs
%type <asm_regs> asm_regs
%type <asm_reg> asm_reg

%type <simple_type> simple_long
%type <simple_type> simple_type
%type <simple_type> simple_type_int
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

%right "!" "~" "*" "&" "-" "--" "++"

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
simple_long:	opt_int										{$$.pos=$1;                                  $$.ival=STYPE_S_LONG;}
|				"double"									{$$.pos=$1;                                  $$.ival=STYPE_LONG_DOUBLE;}
|				"long" opt_int								{$$.pos=pos_merge($1, $2);                   $$.ival=STYPE_S_LONGER;};
// Simple types: Non-pointer, non-array types.
simple_type_int:"char"										{$$.pos=$1;                                  $$.ival=STYPE_CHAR;}
|				"signed" "char"								{$$.pos=pos_merge($1, $2);                   $$.ival=STYPE_S_CHAR;}
|				"unsigned" "char"							{$$.pos=pos_merge($1, $2);                   $$.ival=STYPE_U_CHAR;}
|				opt_signed "short" opt_int					{$$.pos=pos_merge($1, $3);                   $$.ival=STYPE_S_SHORT;}
|				opt_signed "int"							{$$.pos=pos_merge($1, $2);                   $$.ival=STYPE_S_INT;}
|				"signed" "long" opt_int						{$$.pos=pos_merge($1, $3);                   $$.ival=STYPE_S_LONG;}
|				"long" simple_long							{$$=$2;                                      $$.pos=pos_merge($1, $2.pos);}
|				"signed" "long" "long" opt_int				{$$.pos=pos_merge($1, $4);                   $$.ival=STYPE_S_LONGER;}
|				"unsigned" "short" opt_int					{$$.pos=pos_merge($1, $3);                   $$.ival=STYPE_U_SHORT;}
|				"unsigned" "int"							{$$.pos=pos_merge($1, $2);                   $$.ival=STYPE_U_INT;}
|				"unsigned" "long" opt_int					{$$.pos=pos_merge($1, $3);                   $$.ival=STYPE_U_LONG;}
|				"unsigned" "long" "long" opt_int			{$$.pos=pos_merge($1, $4);                   $$.ival=STYPE_U_LONGER;}
|				"float"										{$$.pos=$1;                                  $$.ival=STYPE_FLOAT;}
|				"double"									{$$.pos=$1;                                  $$.ival=STYPE_DOUBLE;}
|				"_Bool"										{$$.pos=$1;                                  $$.ival=STYPE_BOOL;}
|				"void"										{$$.pos=$1;                                  $$.ival=STYPE_VOID;};
// Simple type tracker(tm).
simple_type:	simple_type_int								{ctx->s_type = $1.ival;}


// A variable definition (or typedef) with no array type.
var_nonarr:		var_stars TKN_IDENT							{$$=ident_of_strval(ctx, &$1, &$2, NULL); $$.pos=pos_merge($1.pos, $2.pos);};

/* // A variable definition (or typedef) with no array type.
var_nonarr:		"*" var_nonarr								{$$=$2; $$.pos=pos_merge($1, $2.pos); $$.type=ctype_ptr(ctx->asm_ctx, $$.type);}
|				TKN_IDENT									{$$=ident_of_strval(ctx, &$1);        $$.type=ctype_simple(ctx->asm_ctx, ctx->s_type);}; */

// Counting stars for the vars.
var_stars:		var_stars "*"								{$$.type=ctype_ptr   (ctx->asm_ctx, $1.type);}
|				%empty										{$$.type=ctype_simple(ctx->asm_ctx, ctx->s_type);};

// Counting arrays for the vars.
var_arrays:		var_arrays "[" "]"							{$$.type=ctype_arr   (ctx->asm_ctx, $1.type, NULL);}
|				var_arrays "[" expr "]"						{$$.type=ctype_arr   (ctx->asm_ctx, $1.type, NULL);}
|				%empty										{$$.type=ctype_simple(ctx->asm_ctx, ctx->s_type);};

// A variable definition (or typedef).
var:			var_stars TKN_IDENT var_arrays				{$$=ident_of_strval(ctx, &$1, &$2, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				var_stars "(" var ")" var_arrays			{$$=ident_of_types (ctx, &$1, &$3, &$5); $$.pos=pos_merge($1.pos, $5.pos);};

// A variable definition (or typedef).
/* var:			"*" var					%prec "*"			{$$=$2; $$.pos=pos_merge($1, $2.pos); $$.type=ctype_ptr(ctx->asm_ctx, $$.type);}
|				"(" var ")"									{$$=$2; $$.pos=pos_merge($1, $3);}
|				var "[" "]"				%prec "["			{$$=$1; $$.pos=pos_merge($1.pos, $3); $$.type=ctype_arr(ctx->asm_ctx, $1.type, NULL);}
|				var "[" expr "]"		%prec "["			{$$=$1; $$.pos=pos_merge($1.pos, $4);}
|				TKN_IDENT									{$$=ident_of_strval(ctx, &$1);        $$.type=ctype_simple(ctx->asm_ctx, ctx->s_type);}; */

// A function definition (with code).
funcdef:		simple_type var_nonarr "(" opt_params ")"
				"{" stmts "}"								{$$=funcdef_decl(ctx, &$1, &$2, &$4, &$7); $$.pos=pos_merge($1.pos, $8);}
// A function definition (without code).
|				simple_type var_nonarr "(" opt_params ")"
				";"											{$$=funcdef_def(ctx, &$1, &$2, &$4);       $$.pos=pos_merge($1.pos, $6);};
// One or more variable declarations.
vardecls:		simple_type idents ";"						{$$=$2;                                    $$.pos=pos_merge($1.pos, $3);};

// Function parameters.
opt_params:		params										{$$=$1;}
|				%empty										{$$=idents_empty(ctx);                         $$.pos=pos_empty(ctx->tokeniser_ctx);};
params:			params "," simple_type var_nonarr			{$$=idents_cat_ex(ctx, &$1, &$4, NULL);        $$.pos=pos_merge($1.pos, $4.pos);}
|				simple_type var_nonarr						{$$=idents_one_ex(ctx, &$2, NULL);             $$.pos=pos_merge($1.pos, $2.pos);};
idents:			idents "," var "=" expr						{$$=idents_cat_ex(ctx, &$1, &$3, &$5);         $$.pos=pos_merge($1.pos, $3.pos);}
|				idents "," var								{$$=idents_cat_ex(ctx, &$1, &$3, NULL);        $$.pos=pos_merge($1.pos, $3.pos);}
|				var "=" expr								{$$=idents_one_ex(ctx, &$1, &$3);              $$.pos=$1.pos;}
|				var											{$$=idents_one_ex(ctx, &$1, NULL);             $$.pos=$1.pos;};

// Statements.
stmts:			stmts stmt									{$$=stmts_cat   (ctx, &$1, &$2);             $$.pos=pos_merge($1.pos, $2.pos);}
|				%empty										{$$=stmts_empty (ctx);                       $$.pos=pos_empty(ctx->tokeniser_ctx);};
stmt:			"{" stmts "}"								{$$=stmt_multi  (ctx, &$2);                  $$.pos=pos_merge($1, $3);}
|				stmt_no_stmts								{$$=$1;};
stmt_no_stmts:	"if" "(" expr ")" stmt		%prec "then"	{$$=stmt_if     (ctx, &$3, &$5, NULL);       $$.pos=pos_merge($1, $5.pos);}
|				"if" "(" expr ")" stmt
				"else" stmt									{$$=stmt_if     (ctx, &$3, &$5, &$7);        $$.pos=pos_merge($1, $7.pos);}
|				"while" "(" expr ")" stmt					{$$=stmt_while  (ctx, &$3, &$5);             $$.pos=pos_merge($1, $5.pos);}
|				"for" "(" stmt_no_stmts opt_exprs ";"
				opt_exprs ")" stmt							{$$=stmt_for    (ctx, &$3, &$4, &$6, &$8);   $$.pos=pos_merge($1, $8.pos);}
|				"return" ";"								{$$=stmt_ret    (ctx, NULL);                 $$.pos=pos_merge($1, $2);}
|				"return" expr ";"							{$$=stmt_ret    (ctx, &$2);                  $$.pos=pos_merge($1, $3);}
|				vardecls									{$$=stmt_var    (ctx, &$1);                  $$.pos=$1.pos;}
|				expr ";"									{$$=stmt_expr   (ctx, &$1);                  $$.pos=pos_merge($1.pos, $2);}
|				inline_asm ";"								{$$=$1;                                      $$.pos=pos_merge($1.pos, $2);}
|				";"											{$$=stmt_empty  (ctx);                       $$.pos=$1;};

// Expressions.
opt_exprs:		exprs										{$$=$1;}
|				%empty										{$$=exprs_empty(ctx);};
exprs:			exprs "," expr								{$$=exprs_cat (ctx, &$1, &$3);               $$.pos=pos_merge($1.pos, $3.pos);}
|				expr										{$$=exprs_one (ctx, &$1);                    $$.pos=$1.pos;};
expr:			TKN_IVAL									{$$=expr_icnst(ctx, &$1);                    $$.pos=$1.pos;}
|				TKN_STRVAL									{$$=expr_scnst(ctx, &$1);                    $$.pos=$1.pos;}
|				TKN_IDENT									{$$=expr_ident(ctx, &$1);                    $$.pos=$1.pos;}
|				expr "(" opt_exprs ")"						{$$=expr_call (ctx, &$1, &$3);               $$.pos=pos_merge($1.pos, $4);}
|				expr "[" expr "]"							{$$=expr_math2(ctx, OP_INDEX, &$1, &$3);     $$.pos=pos_merge($1.pos, $4);}
|				"(" expr ")"								{$$=$2;                                      $$.pos=pos_merge($1, $3);}

|				"++" expr									{$$=expr_math1a(ctx, OP_ADD,      &$2);      $$.pos=pos_merge($1, $2.pos);}
|				"--" expr									{$$=expr_math1a(ctx, OP_SUB,      &$2);      $$.pos=pos_merge($1, $2.pos);}

|				"-" expr									{$$=expr_math1(ctx, OP_0_MINUS,   &$2);      $$.pos=pos_merge($1, $2.pos);}
|				"!" expr									{$$=expr_math1(ctx, OP_LOGIC_NOT, &$2);      $$.pos=pos_merge($1, $2.pos);}
|				"~" expr									{$$=expr_math1(ctx, OP_BIT_NOT,   &$2);      $$.pos=pos_merge($1, $2.pos);}
|				"&" expr									{$$=expr_math1(ctx, OP_ADROF,     &$2);      $$.pos=pos_merge($1, $2.pos);}
|				"*" expr									{$$=expr_math1(ctx, OP_DEREF,     &$2);      $$.pos=pos_merge($1, $2.pos);}

|				expr "*" expr								{$$=expr_math2(ctx, OP_MUL,       &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "/" expr				%prec "*"		{$$=expr_math2(ctx, OP_DIV,       &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "%" expr				%prec "*"		{$$=expr_math2(ctx, OP_MOD,       &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}

|				expr "+" expr								{$$=expr_math2(ctx, OP_ADD,       &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "-" expr				%prec "+"		{$$=expr_math2(ctx, OP_SUB,       &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}

|				expr "<" expr								{$$=expr_math2(ctx, OP_LT,        &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "<=" expr				%prec "<"		{$$=expr_math2(ctx, OP_LE,        &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr ">" expr				%prec "<"		{$$=expr_math2(ctx, OP_GT,        &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr ">=" expr				%prec "<"		{$$=expr_math2(ctx, OP_GE,        &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}

|				expr "==" expr								{$$=expr_math2(ctx, OP_NE,        &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "!=" expr				%prec "=="		{$$=expr_math2(ctx, OP_EQ,        &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}

|				expr "&" expr								{$$=expr_math2(ctx, OP_BIT_AND,   &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "^" expr								{$$=expr_math2(ctx, OP_BIT_XOR,   &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "|" expr								{$$=expr_math2(ctx, OP_BIT_OR,    &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "<<" expr								{$$=expr_math2(ctx, OP_SHIFT_L,   &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr ">>" expr				%prec "<<"		{$$=expr_math2(ctx, OP_SHIFT_R,   &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}

|				expr "&&" expr								{$$=expr_math2(ctx, OP_LOGIC_AND, &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "||" expr								{$$=expr_math2(ctx, OP_LOGIC_OR,  &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}

|				expr "=" expr								{$$=expr_math2(ctx, OP_ASSIGN,    &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "+=" expr				%prec "="		{$$=expr_math2a(ctx, OP_ADD,      &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "-=" expr				%prec "="		{$$=expr_math2a(ctx, OP_SUB,      &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "*=" expr				%prec "="		{$$=expr_math2a(ctx, OP_MUL,      &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "/=" expr				%prec "="		{$$=expr_math2a(ctx, OP_DIV,      &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "%=" expr				%prec "="		{$$=expr_math2a(ctx, OP_MOD,      &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "&=" expr				%prec "="		{$$=expr_math2a(ctx, OP_BIT_AND,  &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "|=" expr				%prec "="		{$$=expr_math2a(ctx, OP_BIT_OR,   &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "^=" expr				%prec "="		{$$=expr_math2a(ctx, OP_BIT_XOR,  &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr "<<=" expr				%prec "="		{$$=expr_math2a(ctx, OP_SHIFT_L,  &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);}
|				expr ">>=" expr				%prec "="		{$$=expr_math2a(ctx, OP_SHIFT_R,  &$1, &$3); $$.pos=pos_merge($1.pos, $3.pos);};

// Inline assembly snippets.
inline_asm:		"asm" asm_qual asm_code						{$$=$3; $$.iasm->qualifiers=$2; $$.iasm->qualifiers.is_volatile |= !$$.iasm->outputs;};

asm_qual:		%empty										{$$=(iasm_qual_t) {0, 0, 0};                 $$.pos=pos_empty(ctx->tokeniser_ctx);}
|				asm_qual "volatile"							{$$=$1; $$.is_volatile=1;                    $$.pos=pos_merge($1.pos, $2);}
|				asm_qual "inline"							{$$=$1; $$.is_inline=1;                      $$.pos=pos_merge($1.pos, $2);}
|				asm_qual "goto"								{$$=$1; $$.is_goto=1;                        $$.pos=pos_merge($1.pos, $2);};

asm_code:		"(" TKN_STRVAL
				":" opt_asm_regs
				":" opt_asm_regs
				":" opt_asm_strs ")"						{$$=stmt_iasm(ctx, &$2, &$4,  &$6,  NULL);   $$.pos=pos_merge($1, $9);}
|				"(" TKN_STRVAL
				":" opt_asm_regs
				":" opt_asm_regs ")"						{$$=stmt_iasm(ctx, &$2, &$4,  &$6,  NULL);   $$.pos=pos_merge($1, $7);}
|				"(" TKN_STRVAL
				":" opt_asm_regs ")"						{$$=stmt_iasm(ctx, &$2, &$4,  NULL, NULL);   $$.pos=pos_merge($1, $5);}
|				"(" TKN_STRVAL ")"							{$$=stmt_iasm(ctx, &$2, NULL, NULL, NULL);   $$.pos=pos_merge($1, $3);};

opt_asm_strs:	asm_strs
|				%empty;
asm_strs:		TKN_STRVAL "," asm_strs
|				TKN_STRVAL;
opt_asm_regs:	asm_regs									{$$=$1;}
|				%empty										{$$=iasm_regs_empty(ctx);                    $$.pos=pos_empty(ctx->tokeniser_ctx);};
asm_regs:		asm_reg "," asm_regs						{$$=iasm_regs_cat(ctx, &$3, &$1);            $$.pos=pos_merge($1.pos, $3.pos);}
|				asm_reg										{$$=iasm_regs_one(ctx, &$1);                 $$.pos=$1.pos;};
asm_reg:		"[" TKN_IDENT "]" TKN_STRVAL "(" expr ")"	{$$=stmt_iasm_reg(ctx, &$2,  &$4, &$6);      $$.pos=pos_merge($1, $7);}
|				TKN_STRVAL "(" expr ")"						{$$=stmt_iasm_reg(ctx, NULL, &$1, &$3);      $$.pos=pos_merge($1.pos, $4);};
%%

void *xmake_copy(alloc_ctx_t alloc, void *mem, size_t size) {
	if (!mem) return NULL;
	void *copy = xalloc(alloc, size);
	memcpy(copy, mem, size);
	return copy;
}
