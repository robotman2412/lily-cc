/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_YY_SRC_PARSER_H_INCLUDED
# define YY_YY_SRC_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 8 "src/parser.bison"


#include "parser-util.h"
#include "debug/pront.h"
#include <malloc.h>
#include <string.h>

extern int  yylex  (parser_ctx_t *ctx);
extern void yyerror(parser_ctx_t *ctx, char *msg);


#line 60 "src/parser.h"

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    TKN_UNSIGNED = 258,
    TKN_SIGNED = 259,
    TKN_FLOAT = 260,
    TKN_DOUBLE = 261,
    TKN_BOOL = 262,
    TKN_CHAR = 263,
    TKN_SHORT = 264,
    TKN_INT = 265,
    TKN_LONG = 266,
    TKN_VOID = 267,
    TKN_VOLATILE = 268,
    TKN_INLINE = 269,
    TKN_GOTO = 270,
    TKN_AUTO = 271,
    TKN_FUNC = 272,
    TKN_IF = 273,
    TKN_ELSE = 274,
    TKN_WHILE = 275,
    TKN_RETURN = 276,
    TKN_ASM = 277,
    TKN_THEN = 278,
    TKN_LPAR = 279,
    TKN_RPAR = 280,
    TKN_LBRAC = 281,
    TKN_RBRAC = 282,
    TKN_LSBRAC = 283,
    TKN_RSBRAC = 284,
    TKN_SEMI = 285,
    TKN_COLON = 286,
    TKN_COMMA = 287,
    TKN_IVAL = 288,
    TKN_STRVAL = 289,
    TKN_IDENT = 290,
    TKN_GARBAGE = 291,
    TKN_ASSIGN_ADD = 292,
    TKN_ASSIGN_SUB = 293,
    TKN_ASSIGN_SHL = 294,
    TKN_ASSIGN_SHR = 295,
    TKN_ASSIGN_MUL = 296,
    TKN_ASSIGN_DIV = 297,
    TKN_ASSIGN_REM = 298,
    TKN_ASSIGN_AND = 299,
    TKN_ASSIGN_OR = 300,
    TKN_ASSIGN_XOR = 301,
    TKN_INC = 302,
    TKN_DEC = 303,
    TKN_LOGIC_AND = 304,
    TKN_LOGIC_OR = 305,
    TKN_ADD = 306,
    TKN_SUB = 307,
    TKN_ASSIGN = 308,
    TKN_AMP = 309,
    TKN_MUL = 310,
    TKN_DIV = 311,
    TKN_REM = 312,
    TKN_NOT = 313,
    TKN_INV = 314,
    TKN_XOR = 315,
    TKN_OR = 316,
    TKN_SHL = 317,
    TKN_SHR = 318,
    TKN_LT = 319,
    TKN_LE = 320,
    TKN_GT = 321,
    TKN_GE = 322,
    TKN_EQ = 323,
    TKN_NE = 324
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 22 "src/parser.bison"

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
	
	simple_type_t	simple_type;
	
	strval_t		garbage;

#line 167 "src/parser.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (parser_ctx_t *ctx);

#endif /* !YY_YY_SRC_PARSER_H_INCLUDED  */
