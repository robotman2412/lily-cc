/* A Bison parser, made by GNU Bison 3.7.6.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
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
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

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


#line 61 "src/parser.h"

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    TKN_UNSIGNED = 258,            /* "unsigned"  */
    TKN_SIGNED = 259,              /* "signed"  */
    TKN_FLOAT = 260,               /* "float"  */
    TKN_DOUBLE = 261,              /* "double"  */
    TKN_BOOL = 262,                /* "_Bool"  */
    TKN_CHAR = 263,                /* "char"  */
    TKN_SHORT = 264,               /* "short"  */
    TKN_INT = 265,                 /* "int"  */
    TKN_LONG = 266,                /* "long"  */
    TKN_VOID = 267,                /* "void"  */
    TKN_VOLATILE = 268,            /* "volatile"  */
    TKN_INLINE = 269,              /* "inline"  */
    TKN_GOTO = 270,                /* "goto"  */
    TKN_IF = 271,                  /* "if"  */
    TKN_ELSE = 272,                /* "else"  */
    TKN_WHILE = 273,               /* "while"  */
    TKN_RETURN = 274,              /* "return"  */
    TKN_ASM = 275,                 /* "asm"  */
    TKN_THEN = 276,                /* "then"  */
    TKN_LPAR = 277,                /* "("  */
    TKN_RPAR = 278,                /* ")"  */
    TKN_LBRAC = 279,               /* "{"  */
    TKN_RBRAC = 280,               /* "}"  */
    TKN_LSBRAC = 281,              /* "["  */
    TKN_RSBRAC = 282,              /* "]"  */
    TKN_SEMI = 283,                /* ";"  */
    TKN_COLON = 284,               /* ":"  */
    TKN_COMMA = 285,               /* ","  */
    TKN_IVAL = 286,                /* TKN_IVAL  */
    TKN_STRVAL = 287,              /* TKN_STRVAL  */
    TKN_IDENT = 288,               /* TKN_IDENT  */
    TKN_GARBAGE = 289,             /* TKN_GARBAGE  */
    TKN_ASSIGN_ADD = 290,          /* "+="  */
    TKN_ASSIGN_SUB = 291,          /* "-="  */
    TKN_ASSIGN_SHL = 292,          /* "<<="  */
    TKN_ASSIGN_SHR = 293,          /* ">>="  */
    TKN_ASSIGN_MUL = 294,          /* "*="  */
    TKN_ASSIGN_DIV = 295,          /* "/="  */
    TKN_ASSIGN_REM = 296,          /* "%="  */
    TKN_ASSIGN_AND = 297,          /* "&="  */
    TKN_ASSIGN_OR = 298,           /* "|="  */
    TKN_ASSIGN_XOR = 299,          /* "^="  */
    TKN_INC = 300,                 /* "++"  */
    TKN_DEC = 301,                 /* "--"  */
    TKN_LOGIC_AND = 302,           /* "&&"  */
    TKN_LOGIC_OR = 303,            /* "||"  */
    TKN_ADD = 304,                 /* "+"  */
    TKN_SUB = 305,                 /* "-"  */
    TKN_ASSIGN = 306,              /* "="  */
    TKN_AMP = 307,                 /* "&"  */
    TKN_MUL = 308,                 /* "*"  */
    TKN_DIV = 309,                 /* "/"  */
    TKN_REM = 310,                 /* "%"  */
    TKN_NOT = 311,                 /* "!"  */
    TKN_INV = 312,                 /* "~"  */
    TKN_XOR = 313,                 /* "^"  */
    TKN_OR = 314,                  /* "|"  */
    TKN_SHL = 315,                 /* "<<"  */
    TKN_SHR = 316,                 /* ">>"  */
    TKN_LT = 317,                  /* "<"  */
    TKN_LE = 318,                  /* "<="  */
    TKN_GT = 319,                  /* ">"  */
    TKN_GE = 320,                  /* ">="  */
    TKN_EQ = 321,                  /* "=="  */
    TKN_NE = 322                   /* "!="  */
  };
  typedef enum yytokentype yytoken_kind_t;
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
	
	ival_t			simple_type;
	
	strval_t		garbage;

#line 171 "src/parser.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (parser_ctx_t *ctx);

#endif /* !YY_YY_SRC_PARSER_H_INCLUDED  */
