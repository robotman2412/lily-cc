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
#include <malloc.h>
#include <string.h>


#line 55 "src/parser.h"

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    TKN_IF = 258,
    TKN_ELSE = 259,
    TKN_WHILE = 260,
    TKN_FOR = 261,
    TKN_RETURN = 262,
    TKN_LPAR = 263,
    TKN_RPAR = 264,
    TKN_LBRAC = 265,
    TKN_RBRAC = 266,
    TKN_SEMI = 267,
    TKN_COMMA = 268,
    TKN_NUM = 269,
    TKN_IDENT = 270,
    TKN_GARBAGE = 271,
    TKN_INC = 272,
    TKN_DEC = 273,
    TKN_ADD = 274,
    TKN_SUB = 275,
    TKN_ASSIGN = 276,
    TKN_MUL = 277,
    TKN_DIV = 278,
    TKN_REM = 279,
    TKN_THEN = 280,
    TKN_STRUCT = 281,
    TKN_ENUM = 282,
    TKN_VOID = 283,
    TKN_SIGNED = 284,
    TKN_UNSIGNED = 285,
    TKN_CHAR = 286,
    TKN_SHORT = 287,
    TKN_LONG = 288,
    TKN_INT = 289,
    TKN_FLOAT = 290,
    TKN_DOUBLE = 291
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 17 "src/parser.bison"

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

#line 119 "src/parser.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (parser_ctx_t *ctx);

#endif /* !YY_YY_SRC_PARSER_H_INCLUDED  */
