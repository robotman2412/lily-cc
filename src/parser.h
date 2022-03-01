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
    TKN_VOLATILE = 258,
    TKN_INLINE = 259,
    TKN_GOTO = 260,
    TKN_AUTO = 261,
    TKN_FUNC = 262,
    TKN_IF = 263,
    TKN_ELSE = 264,
    TKN_WHILE = 265,
    TKN_RETURN = 266,
    TKN_ASM = 267,
    TKN_THEN = 268,
    TKN_LPAR = 269,
    TKN_RPAR = 270,
    TKN_LBRAC = 271,
    TKN_RBRAC = 272,
    TKN_LSBRAC = 273,
    TKN_RSBRAC = 274,
    TKN_SEMI = 275,
    TKN_COLON = 276,
    TKN_COMMA = 277,
    TKN_IVAL = 278,
    TKN_STRVAL = 279,
    TKN_IDENT = 280,
    TKN_GARBAGE = 281,
    TKN_ASSIGN_ADD = 282,
    TKN_ASSIGN_SUB = 283,
    TKN_ASSIGN_SHL = 284,
    TKN_ASSIGN_SHR = 285,
    TKN_ASSIGN_MUL = 286,
    TKN_ASSIGN_DIV = 287,
    TKN_ASSIGN_REM = 288,
    TKN_ASSIGN_AND = 289,
    TKN_ASSIGN_OR = 290,
    TKN_ASSIGN_XOR = 291,
    TKN_INC = 292,
    TKN_DEC = 293,
    TKN_LOGIC_AND = 294,
    TKN_LOGIC_OR = 295,
    TKN_ADD = 296,
    TKN_SUB = 297,
    TKN_ASSIGN = 298,
    TKN_AMP = 299,
    TKN_MUL = 300,
    TKN_DIV = 301,
    TKN_REM = 302,
    TKN_NOT = 303,
    TKN_INV = 304,
    TKN_XOR = 305,
    TKN_OR = 306,
    TKN_SHL = 307,
    TKN_SHR = 308,
    TKN_LT = 309,
    TKN_LE = 310,
    TKN_GT = 311,
    TKN_GE = 312,
    TKN_EQ = 313,
    TKN_NE = 314
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
	
	iasm_qual_t asm_qual;
	iasm_regs_t asm_regs;
	iasm_reg_t  asm_reg;
	
	strval_t	garbage;

#line 154 "src/parser.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (parser_ctx_t *ctx);

#endif /* !YY_YY_SRC_PARSER_H_INCLUDED  */
