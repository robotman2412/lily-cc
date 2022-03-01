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


#line 59 "src/parser.h"

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    TKN_AUTO = 258,
    TKN_FUNC = 259,
    TKN_IF = 260,
    TKN_ELSE = 261,
    TKN_WHILE = 262,
    TKN_RETURN = 263,
    TKN_ASM = 264,
    TKN_THEN = 265,
    TKN_LPAR = 266,
    TKN_RPAR = 267,
    TKN_LBRAC = 268,
    TKN_RBRAC = 269,
    TKN_LSBRAC = 270,
    TKN_RSBRAC = 271,
    TKN_SEMI = 272,
    TKN_COLON = 273,
    TKN_COMMA = 274,
    TKN_IVAL = 275,
    TKN_STRVAL = 276,
    TKN_IDENT = 277,
    TKN_GARBAGE = 278,
    TKN_ASSIGN_ADD = 279,
    TKN_ASSIGN_SUB = 280,
    TKN_ASSIGN_SHL = 281,
    TKN_ASSIGN_SHR = 282,
    TKN_ASSIGN_MUL = 283,
    TKN_ASSIGN_DIV = 284,
    TKN_ASSIGN_REM = 285,
    TKN_ASSIGN_AND = 286,
    TKN_ASSIGN_OR = 287,
    TKN_ASSIGN_XOR = 288,
    TKN_INC = 289,
    TKN_DEC = 290,
    TKN_ADD = 291,
    TKN_SUB = 292,
    TKN_ASSIGN = 293,
    TKN_AMP = 294,
    TKN_MUL = 295,
    TKN_DIV = 296,
    TKN_REM = 297,
    TKN_NOT = 298,
    TKN_INV = 299,
    TKN_XOR = 300,
    TKN_OR = 301,
    TKN_SHL = 302,
    TKN_SHR = 303,
    TKN_LT = 304,
    TKN_LE = 305,
    TKN_GT = 306,
    TKN_GE = 307,
    TKN_EQ = 308,
    TKN_NE = 309
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 21 "src/parser.bison"

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

#line 145 "src/parser.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (parser_ctx_t *ctx);

#endif /* !YY_YY_SRC_PARSER_H_INCLUDED  */
