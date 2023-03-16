
%require "3.8.1"
%language "c++"
%skeleton "lalr1.cc"

%define api.token.raw
%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {

#include "parser-util.h"
#include "tokeniser.h"
#include <string>

}

%param { parser::Ctx &ctx }

%locations

%define parse.trace
%define parse.error detailed
%define parse.lac full

%define api.token.prefix {}

%token TKN_UNSIGNED "unsigned" TKN_SIGNED "signed" TKN_FLOAT "float" TKN_DOUBLE "double" TKN_BOOL "_Bool"
%token TKN_CHAR "char" TKN_SHORT "short" TKN_INT "int" TKN_LONG "long"
%token TKN_VOID "void"
%token TKN_VOLATILE "volatile" TKN_INLINE "inline" TKN_GOTO "goto"
%token TKN_IF "if" TKN_ELSE "else" TKN_WHILE "while" TKN_RETURN "return" TKN_FOR "for" TKN_ASM "asm"
%token TKN_THEN "then"
%token TKN_LPAR "(" TKN_RPAR ")" TKN_LBRAC "{" TKN_RBRAC "}" TKN_LSBRAC "[" TKN_RSBRAC "]"
%token TKN_SEMI ";" TKN_COLON ":" TKN_COMMA ","

%token <long long> TKN_IVAL
%token <std::string> TKN_STRVAL
%token <std::string> TKN_IDENT
%token <std::string> TKN_GARBAGE

%token TKN_ASSIGN_ADD "+=" TKN_ASSIGN_SUB "-="
%token TKN_ASSIGN_SHL "<<=" TKN_ASSIGN_SHR ">>="
%token TKN_ASSIGN_MUL "*=" TKN_ASSIGN_DIV "/=" TKN_ASSIGN_REM "%="
%token TKN_ASSIGN_AND "&=" TKN_ASSIGN_OR "|=" TKN_ASSIGN_XOR "^="
%token TKN_INC "++" TKN_DEC "--"
%token TKN_LOGIC_AND "&&" TKN_LOGIC_OR "||"
%token TKN_ADD "+" TKN_SUB "-" TKN_ASSIGN "=" TKN_AMP "&"
%token TKN_MUL "*" TKN_DIV "/" TKN_REM "%"
%token TKN_NOT "!" TKN_INV "~" TKN_XOR "^" TKN_OR "|"
%token TKN_SHL "<<" TKN_SHR ">>"
%token TKN_LT "<" TKN_LE "<=" TKN_GT ">" TKN_GE ">=" TKN_EQ "==" TKN_NE "!="

%type <idents> opt_params
%type <idents> params
%type <idents> idents
%type <ident> var_nonarr
%type <ident> var
%type <ident> var_stars
%type <ident> var_arrays
%type <func> funcdef
%type <syntax::Expression> expr
%type <std::vector<syntax::Expression>> opt_exprs
%type <std::vector<syntax::Expression>> exprs

%type <std::vector<syntax::Statement>> stmts
%type <syntax::Statement> stmt
%type <syntax::Statement> stmt_no_stmts
/* %type <stmt> inline_asm
%type <stmt> asm_code
%type <asm_qual> asm_qual
%type <asm_regs> opt_asm_regs
%type <asm_regs> asm_regs
%type <asm_reg> asm_reg */

%type <syntax::SimpleType> simple_type



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
global:			funcdef;

// Function declaration and/or definition.
funcdef:		simple_type var_nonarr "(" ")" ";"
|				simple_type var_nonarr "(" ")" "{" stmts "}";

// Multiple statements.
stmts:			stmts stmt		{$$=$1; $$.push_back(std::move($2));}
|				stmt			{$$={$2};};
%%
