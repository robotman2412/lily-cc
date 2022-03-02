/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.5.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 4 "src/parser.bison"

#define YYDEBUG 1

#line 74 "src/parser.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
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


#line 129 "src/parser.c"

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

#line 236 "src/parser.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (parser_ctx_t *ctx);

#endif /* !YY_YY_SRC_PARSER_H_INCLUDED  */



#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))

/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  21
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   897

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  70
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  24
/* YYNRULES -- Number of rules.  */
#define YYNRULES  107
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  200

#define YYUNDEFTOK  2
#define YYMAXUTOK   324


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   125,   125,   126,   128,   129,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   152,   154,   156,   157,
     158,   159,   160,   161,   163,   164,   165,   166,   167,   169,
     170,   171,   172,   173,   174,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   187,   188,   189,   190,   191,
     193,   194,   195,   197,   198,   200,   201,   202,   203,   205,
     206,   208,   209,   210,   211,   212,   214,   215,   217,   218,
     219,   220,   221,   222,   223,   224,   225,   226,   227,   229,
     231,   232,   233,   234,   236,   240,   243,   245,   247,   248,
     249,   250,   251,   252,   253,   254,   255,   256
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "\"unsigned\"", "\"signed\"",
  "\"float\"", "\"double\"", "\"_Bool\"", "\"char\"", "\"short\"",
  "\"int\"", "\"long\"", "\"void\"", "\"volatile\"", "\"inline\"",
  "\"goto\"", "\"auto\"", "\"func\"", "\"if\"", "\"else\"", "\"while\"",
  "\"return\"", "\"asm\"", "\"then\"", "\"(\"", "\")\"", "\"{\"", "\"}\"",
  "\"[\"", "\"]\"", "\";\"", "\":\"", "\",\"", "TKN_IVAL", "TKN_STRVAL",
  "TKN_IDENT", "TKN_GARBAGE", "\"+=\"", "\"-=\"", "\"<<=\"", "\">>=\"",
  "\"*=\"", "\"/=\"", "\"%=\"", "\"&=\"", "\"|=\"", "\"^=\"", "\"++\"",
  "\"--\"", "\"&&\"", "\"||\"", "\"+\"", "\"-\"", "\"=\"", "\"&\"",
  "\"*\"", "\"/\"", "\"%\"", "\"!\"", "\"~\"", "\"^\"", "\"|\"", "\"<<\"",
  "\">>\"", "\"<\"", "\"<=\"", "\">\"", "\">=\"", "\"==\"", "\"!=\"",
  "$accept", "library", "global", "opt_int", "opt_signed", "simple_type",
  "funcdef", "vardecls", "opt_params", "params", "idents", "stmts", "stmt",
  "opt_exprs", "exprs", "expr", "inline_asm", "asm_qual", "asm_code",
  "opt_asm_strs", "asm_strs", "opt_asm_regs", "asm_regs", "asm_reg", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324
};
# endif

#define YYPACT_NINF (-155)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-30)

#define yytable_value_is_error(Yyn) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     219,    80,    -7,  -155,  -155,  -155,  -155,    10,  -155,    21,
     219,    15,    -2,  -155,  -155,  -155,    64,  -155,    18,  -155,
    -155,  -155,  -155,    64,  -155,   106,    55,    74,  -155,  -155,
      64,  -155,  -155,    64,  -155,   692,  -155,    70,  -155,  -155,
      85,    88,    90,  -155,  -155,    97,   755,  -155,    89,    60,
    -155,   101,   110,   838,  -155,   830,  -155,  -155,  -155,  -155,
    -155,   830,   830,   830,   830,   830,   105,  -155,  -155,   204,
     111,   830,   830,  -155,   250,   -10,   296,   124,   166,   166,
     166,   166,   166,  -155,   830,   830,  -155,   830,   830,   830,
     830,   830,   830,   830,   830,   830,   830,   830,   830,   830,
     830,   830,   830,   830,   830,   830,   830,   830,   830,   830,
     830,   830,   830,   830,   830,   830,  -155,   342,   388,  -155,
    -155,  -155,  -155,   113,  -155,  -155,  -155,   127,  -155,   434,
     480,   618,   618,   618,   618,   618,   618,   618,   618,   618,
     618,   683,   664,   115,   115,   618,   166,   166,   166,   166,
     748,   729,   109,   109,    45,    45,    45,    45,   794,   794,
     181,   181,    52,  -155,   830,  -155,   130,  -155,  -155,    42,
    -155,   181,   118,   131,    67,  -155,   122,  -155,   133,   830,
    -155,    42,    42,   134,   526,    78,  -155,   132,  -155,  -155,
     139,   830,   142,   150,  -155,   572,   139,  -155,  -155,  -155
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       9,     0,     8,    21,    22,    24,    10,     0,    25,     0,
       9,     0,     0,     4,     5,    12,     7,    18,     7,    11,
      23,     1,     2,     7,    14,     7,    33,     0,     6,    17,
       7,    19,    13,     7,    15,     9,    27,     0,    20,    16,
       0,     0,    28,    32,    31,     0,     9,    35,     0,     9,
      30,     0,     0,     0,    90,     0,    35,    26,    49,    50,
      51,     0,     0,     0,     0,     0,     0,    42,    34,     0,
       0,     0,     0,    40,     0,     0,     0,     9,    55,    58,
      59,    56,    57,    33,    46,     0,    43,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    44,     0,     0,    41,
      91,    92,    93,     0,    89,    54,    36,     0,    45,    48,
       0,    79,    80,    87,    88,    81,    82,    83,    84,    85,
      86,    76,    77,    63,    64,    78,    71,    60,    61,    62,
      72,    73,    74,    75,    65,    66,    67,    68,    69,    70,
       9,     9,     0,    52,     0,    53,    37,    39,    97,   103,
      47,     9,     0,     0,     0,   102,   105,    38,     0,     0,
      96,   103,     0,     0,     0,     0,   104,     0,   107,    95,
      99,     0,   101,     0,    98,     0,     0,    94,   106,   100
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -155,   167,  -155,    -3,  -155,    75,  -155,    13,  -155,  -155,
    -155,   125,  -154,  -155,    16,   -53,  -155,  -155,  -155,  -155,
      -5,    14,    22,  -155
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     9,    10,    29,    11,    66,    13,    67,    41,    42,
      27,    49,    68,   127,   128,    69,    70,    75,   124,   193,
     194,   174,   175,   176
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      74,    19,    76,   120,   121,   122,   166,   167,    78,    79,
      80,    81,    82,    14,   123,    31,    20,   177,   117,   118,
      32,    21,    34,    14,    23,    24,    25,    38,    28,    30,
      39,   129,   130,    26,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,   148,   149,   150,   151,   152,   153,   154,   155,   156,
     157,   158,   159,     1,     2,     3,     4,     5,     6,    84,
     172,     7,     8,    85,    28,    12,   173,   168,    51,    35,
      52,    53,    54,   169,    55,    12,    56,    57,    15,    16,
      17,    18,   180,    58,    59,    60,    99,   100,   181,   102,
     103,   104,   105,   189,    36,    43,    37,   108,   109,   190,
      40,   129,    61,    45,    62,    63,    28,    33,    64,    65,
      44,    48,    46,    47,    50,    71,   184,     1,     2,     3,
       4,     5,     6,    84,    72,     7,     8,    85,   195,    84,
      83,   116,    51,    85,    52,    53,    54,   162,    55,   171,
      56,   126,   163,   178,   182,   179,   191,    58,    59,    60,
      99,   100,   183,   102,   103,   104,   105,   100,   187,   102,
     103,   104,   105,   192,   196,   197,    61,    22,    62,    63,
     170,    77,    64,    65,     1,     2,     3,     4,     5,     6,
      84,   199,     7,     8,    85,   185,     0,     0,     0,    51,
       0,    52,    53,    54,   186,    55,     0,    56,     0,     0,
       0,     0,     0,     0,    58,    59,    60,     0,   100,    -3,
     102,   103,     1,     2,     3,     4,     5,     6,    84,     0,
       7,     8,    85,    61,    86,    62,    63,     0,     0,    64,
      65,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,     0,     0,    97,    98,    99,   100,   101,   102,   103,
     104,   105,     0,     0,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,    84,     0,     0,     0,    85,     0,
     119,     0,     0,     0,     0,     0,     0,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,     0,     0,    97,
      98,    99,   100,   101,   102,   103,   104,   105,     0,     0,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
      84,   125,     0,     0,    85,     0,     0,     0,     0,     0,
       0,     0,     0,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,     0,     0,    97,    98,    99,   100,   101,
     102,   103,   104,   105,     0,     0,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,    84,   160,     0,     0,
      85,     0,     0,     0,     0,     0,     0,     0,     0,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,     0,
       0,    97,    98,    99,   100,   101,   102,   103,   104,   105,
       0,     0,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,    84,   161,     0,     0,    85,     0,     0,     0,
       0,     0,     0,     0,     0,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,     0,     0,    97,    98,    99,
     100,   101,   102,   103,   104,   105,     0,     0,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,    84,     0,
       0,     0,    85,     0,     0,     0,   164,     0,     0,     0,
       0,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,     0,     0,    97,    98,    99,   100,   101,   102,   103,
     104,   105,     0,     0,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,    84,     0,     0,     0,    85,   165,
       0,     0,     0,     0,     0,     0,     0,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,     0,     0,    97,
      98,    99,   100,   101,   102,   103,   104,   105,     0,     0,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
      84,   188,     0,     0,    85,     0,     0,     0,     0,     0,
       0,     0,     0,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,     0,     0,    97,    98,    99,   100,   101,
     102,   103,   104,   105,     0,     0,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,    84,   198,     0,     0,
      85,     0,     0,     0,     0,     0,     0,     0,     0,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,     0,
       0,    97,    98,    99,   100,   101,   102,   103,   104,   105,
       0,     0,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,    84,     0,     0,     0,    85,     0,     0,     0,
       0,     0,     0,     0,     0,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,     0,     0,    97,    98,    99,
     100,   101,   102,   103,   104,   105,     0,     0,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,    84,     0,
       0,     0,    85,     0,     0,     1,     2,     3,     4,     5,
       6,     0,     0,     7,     8,     0,     0,    84,     0,     0,
       0,    85,     0,    97,     0,    99,   100,   -29,   102,   103,
     104,   105,     0,     0,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,    99,   100,     0,   102,   103,   104,
     105,     0,     0,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,    84,     0,     0,     0,    85,     1,     2,
       3,     4,     5,     6,     0,     0,     7,     8,     0,     0,
       0,     0,    84,     0,     0,     0,    85,     0,     0,     0,
      99,   100,     0,   102,   103,   104,   105,     0,     0,   106,
       0,   108,   109,   110,   111,   112,   113,   114,   115,    99,
     100,     0,   102,   103,   104,   105,     0,     0,     0,     0,
     108,   109,   110,   111,   112,   113,   114,   115,    84,     0,
       0,     0,    85,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    99,   100,     0,   102,   103,
     104,   105,     0,     0,    55,     0,   108,   109,   110,   111,
     112,   113,    55,    58,    59,    60,     0,     0,    73,     0,
       0,    58,    59,    60,     0,     0,     0,     0,     0,     0,
       0,     0,    61,     0,    62,    63,     0,     0,    64,    65,
      61,     0,    62,    63,     0,     0,    64,    65
};

static const yytype_int16 yycheck[] =
{
      53,     8,    55,    13,    14,    15,   160,   161,    61,    62,
      63,    64,    65,     0,    24,    18,     6,   171,    71,    72,
      23,     0,    25,    10,     9,    10,    11,    30,    10,    11,
      33,    84,    85,    35,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,     3,     4,     5,     6,     7,     8,    24,
      28,    11,    12,    28,    10,     0,    34,    25,    18,    24,
      20,    21,    22,    31,    24,    10,    26,    27,     8,     9,
      10,    11,    25,    33,    34,    35,    51,    52,    31,    54,
      55,    56,    57,    25,    30,    35,    32,    62,    63,    31,
      35,   164,    52,    25,    54,    55,    10,    11,    58,    59,
      35,    46,    32,    26,    35,    24,   179,     3,     4,     5,
       6,     7,     8,    24,    24,    11,    12,    28,   191,    24,
      35,    30,    18,    28,    20,    21,    22,    34,    24,    19,
      26,    27,    25,    35,    32,    24,    24,    33,    34,    35,
      51,    52,    29,    54,    55,    56,    57,    52,    34,    54,
      55,    56,    57,    34,    32,    25,    52,    10,    54,    55,
     164,    56,    58,    59,     3,     4,     5,     6,     7,     8,
      24,   196,    11,    12,    28,   181,    -1,    -1,    -1,    18,
      -1,    20,    21,    22,   182,    24,    -1,    26,    -1,    -1,
      -1,    -1,    -1,    -1,    33,    34,    35,    -1,    52,     0,
      54,    55,     3,     4,     5,     6,     7,     8,    24,    -1,
      11,    12,    28,    52,    30,    54,    55,    -1,    -1,    58,
      59,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    -1,    -1,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    -1,    -1,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    24,    -1,    -1,    -1,    28,    -1,
      30,    -1,    -1,    -1,    -1,    -1,    -1,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    -1,    -1,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    -1,    -1,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      24,    25,    -1,    -1,    28,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    -1,    -1,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    -1,    -1,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    24,    25,    -1,    -1,
      28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    -1,
      -1,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      -1,    -1,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    24,    25,    -1,    -1,    28,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    -1,    -1,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    -1,    -1,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    24,    -1,
      -1,    -1,    28,    -1,    -1,    -1,    32,    -1,    -1,    -1,
      -1,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    -1,    -1,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    -1,    -1,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    24,    -1,    -1,    -1,    28,    29,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    -1,    -1,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    -1,    -1,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      24,    25,    -1,    -1,    28,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    -1,    -1,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    -1,    -1,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    24,    25,    -1,    -1,
      28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    -1,
      -1,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      -1,    -1,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    24,    -1,    -1,    -1,    28,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    -1,    -1,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    -1,    -1,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    24,    -1,
      -1,    -1,    28,    -1,    -1,     3,     4,     5,     6,     7,
       8,    -1,    -1,    11,    12,    -1,    -1,    24,    -1,    -1,
      -1,    28,    -1,    49,    -1,    51,    52,    25,    54,    55,
      56,    57,    -1,    -1,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    51,    52,    -1,    54,    55,    56,
      57,    -1,    -1,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    24,    -1,    -1,    -1,    28,     3,     4,
       5,     6,     7,     8,    -1,    -1,    11,    12,    -1,    -1,
      -1,    -1,    24,    -1,    -1,    -1,    28,    -1,    -1,    -1,
      51,    52,    -1,    54,    55,    56,    57,    -1,    -1,    60,
      -1,    62,    63,    64,    65,    66,    67,    68,    69,    51,
      52,    -1,    54,    55,    56,    57,    -1,    -1,    -1,    -1,
      62,    63,    64,    65,    66,    67,    68,    69,    24,    -1,
      -1,    -1,    28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    51,    52,    -1,    54,    55,
      56,    57,    -1,    -1,    24,    -1,    62,    63,    64,    65,
      66,    67,    24,    33,    34,    35,    -1,    -1,    30,    -1,
      -1,    33,    34,    35,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    52,    -1,    54,    55,    -1,    -1,    58,    59,
      52,    -1,    54,    55,    -1,    -1,    58,    59
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,    11,    12,    71,
      72,    74,    75,    76,    77,     8,     9,    10,    11,     8,
       6,     0,    71,     9,    10,    11,    35,    80,    10,    73,
      11,    73,    73,    11,    73,    24,    30,    32,    73,    73,
      75,    78,    79,    35,    35,    25,    32,    26,    75,    81,
      35,    18,    20,    21,    22,    24,    26,    27,    33,    34,
      35,    52,    54,    55,    58,    59,    75,    77,    82,    85,
      86,    24,    24,    30,    85,    87,    85,    81,    85,    85,
      85,    85,    85,    35,    24,    28,    30,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    30,    85,    85,    30,
      13,    14,    15,    24,    88,    25,    27,    83,    84,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    85,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    85,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    85,    85,
      25,    25,    34,    25,    32,    29,    82,    82,    25,    31,
      84,    19,    28,    34,    91,    92,    93,    82,    35,    24,
      25,    31,    32,    29,    85,    91,    92,    34,    25,    25,
      31,    24,    34,    89,    90,    85,    32,    25,    25,    90
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_int8 yyr1[] =
{
       0,    70,    71,    71,    72,    72,    73,    73,    74,    74,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    76,    77,    78,    78,
      79,    79,    80,    80,    81,    81,    82,    82,    82,    82,
      82,    82,    82,    82,    82,    83,    83,    84,    84,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    85,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    85,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    85,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    85,    86,
      87,    87,    87,    87,    88,    88,    88,    88,    89,    89,
      90,    90,    91,    91,    92,    92,    93,    93
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     0,     1,     1,     1,     0,     1,     0,
       1,     2,     2,     3,     2,     3,     4,     3,     2,     3,
       4,     1,     1,     2,     1,     1,     8,     3,     1,     0,
       4,     2,     3,     1,     2,     0,     3,     5,     7,     5,
       2,     3,     1,     2,     2,     1,     0,     3,     1,     1,
       1,     1,     4,     4,     3,     2,     2,     2,     2,     2,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       0,     2,     2,     2,     9,     7,     5,     3,     1,     0,
       3,     1,     1,     0,     3,     1,     7,     4
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (ctx, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, ctx); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, parser_ctx_t *ctx)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  YYUSE (ctx);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, parser_ctx_t *ctx)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyo, yytype, yyvaluep, ctx);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, int yyrule, parser_ctx_t *ctx)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[+yyssp[yyi + 1 - yynrhs]],
                       &yyvsp[(yyi + 1) - (yynrhs)]
                                              , ctx);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, ctx); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
#  else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                yy_state_t *yyssp, int yytoken)
{
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Actual size of YYARG. */
  int yycount = 0;
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[+*yyssp];
      YYPTRDIFF_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
      yysize = yysize0;
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYPTRDIFF_T yysize1
                    = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    /* Don't count the "%s"s in the final size, but reserve room for
       the terminator.  */
    YYPTRDIFF_T yysize1 = yysize + (yystrlen (yyformat) - 2 * yycount) + 1;
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, parser_ctx_t *ctx)
{
  YYUSE (yyvaluep);
  YYUSE (ctx);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (parser_ctx_t *ctx)
{
    yy_state_fast_t yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss;
    yy_state_t *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYPTRDIFF_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex (ctx);
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 4:
#line 128 "src/parser.bison"
                                {function_added(ctx, &(yyvsp[0].func));}
#line 1695 "src/parser.c"
    break;

  case 10:
#line 135 "src/parser.bison"
                                                                                                {(yyval.simple_type)=CTYPE_CHAR;}
#line 1701 "src/parser.c"
    break;

  case 11:
#line 136 "src/parser.bison"
                                                                                                        {(yyval.simple_type)=CTYPE_S_CHAR;}
#line 1707 "src/parser.c"
    break;

  case 12:
#line 137 "src/parser.bison"
                                                                                                        {(yyval.simple_type)=CTYPE_U_CHAR;}
#line 1713 "src/parser.c"
    break;

  case 13:
#line 138 "src/parser.bison"
                                                                                                {(yyval.simple_type)=CTYPE_S_SHORT;}
#line 1719 "src/parser.c"
    break;

  case 14:
#line 139 "src/parser.bison"
                                                                                                        {(yyval.simple_type)=CTYPE_S_INT;}
#line 1725 "src/parser.c"
    break;

  case 15:
#line 140 "src/parser.bison"
                                                                                                {(yyval.simple_type)=CTYPE_S_LONG;}
#line 1731 "src/parser.c"
    break;

  case 16:
#line 141 "src/parser.bison"
                                                                                        {(yyval.simple_type)=CTYPE_S_LONGER;}
#line 1737 "src/parser.c"
    break;

  case 17:
#line 142 "src/parser.bison"
                                                                                                {(yyval.simple_type)=CTYPE_U_SHORT;}
#line 1743 "src/parser.c"
    break;

  case 18:
#line 143 "src/parser.bison"
                                                                                                        {(yyval.simple_type)=CTYPE_U_INT;}
#line 1749 "src/parser.c"
    break;

  case 19:
#line 144 "src/parser.bison"
                                                                                                {(yyval.simple_type)=CTYPE_U_LONG;}
#line 1755 "src/parser.c"
    break;

  case 20:
#line 145 "src/parser.bison"
                                                                                        {(yyval.simple_type)=CTYPE_U_LONGER;}
#line 1761 "src/parser.c"
    break;

  case 21:
#line 146 "src/parser.bison"
                                                                                                                {(yyval.simple_type)=CTYPE_FLOAT;}
#line 1767 "src/parser.c"
    break;

  case 22:
#line 147 "src/parser.bison"
                                                                                                                {(yyval.simple_type)=CTYPE_DOUBLE;}
#line 1773 "src/parser.c"
    break;

  case 23:
#line 148 "src/parser.bison"
                                                                                                        {(yyval.simple_type)=CTYPE_LONG_DOUBLE;}
#line 1779 "src/parser.c"
    break;

  case 24:
#line 149 "src/parser.bison"
                                                                                                                {(yyval.simple_type)=CTYPE_BOOL;}
#line 1785 "src/parser.c"
    break;

  case 25:
#line 150 "src/parser.bison"
                                                                                                                {(yyval.simple_type)=CTYPE_VOID;}
#line 1791 "src/parser.c"
    break;

  case 26:
#line 153 "src/parser.bison"
                                                                                                        {(yyval.func)=funcdef_decl(&(yyvsp[-6].strval), &(yyvsp[-4].idents), &(yyvsp[-1].stmts));}
#line 1797 "src/parser.c"
    break;

  case 27:
#line 154 "src/parser.bison"
                                                                                        {(yyval.idents)=(yyvsp[-1].idents); idents_settype(&(yyval.idents), &(yyvsp[-2].simple_type));}
#line 1803 "src/parser.c"
    break;

  case 28:
#line 156 "src/parser.bison"
                                                                                                        {(yyval.idents)=(yyvsp[0].idents);}
#line 1809 "src/parser.c"
    break;

  case 29:
#line 157 "src/parser.bison"
                                                                                                                {(yyval.idents)=idents_empty();}
#line 1815 "src/parser.c"
    break;

  case 30:
#line 158 "src/parser.bison"
                                                                                {(yyval.idents)=idents_cat  (&(yyvsp[-3].idents),  &(yyvsp[-1].simple_type), &(yyvsp[0].strval));}
#line 1821 "src/parser.c"
    break;

  case 31:
#line 159 "src/parser.bison"
                                                                                                {(yyval.idents)=idents_one  (&(yyvsp[-1].simple_type), &(yyvsp[0].strval));}
#line 1827 "src/parser.c"
    break;

  case 32:
#line 160 "src/parser.bison"
                                                                                        {(yyval.idents)=idents_cat  (&(yyvsp[-2].idents),  NULL, &(yyvsp[0].strval));}
#line 1833 "src/parser.c"
    break;

  case 33:
#line 161 "src/parser.bison"
                                                                                                                {(yyval.idents)=idents_one  (NULL, &(yyvsp[0].strval));}
#line 1839 "src/parser.c"
    break;

  case 34:
#line 163 "src/parser.bison"
                                                                                                        {(yyval.stmts)=stmts_cat   (&(yyvsp[-1].stmts), &(yyvsp[0].stmt));}
#line 1845 "src/parser.c"
    break;

  case 35:
#line 164 "src/parser.bison"
                                                                                                                {(yyval.stmts)=stmts_empty ();}
#line 1851 "src/parser.c"
    break;

  case 36:
#line 165 "src/parser.bison"
                                                                                                {(yyval.stmt)=stmt_multi  (&(yyvsp[-1].stmts));}
#line 1857 "src/parser.c"
    break;

  case 37:
#line 166 "src/parser.bison"
                                                                                {(yyval.stmt)=stmt_if     (&(yyvsp[-2].expr), &(yyvsp[0].stmt), NULL);}
#line 1863 "src/parser.c"
    break;

  case 38:
#line 168 "src/parser.bison"
                                                                                                                {(yyval.stmt)=stmt_if     (&(yyvsp[-4].expr), &(yyvsp[-2].stmt), &(yyvsp[0].stmt));}
#line 1869 "src/parser.c"
    break;

  case 39:
#line 169 "src/parser.bison"
                                                                                                {(yyval.stmt)=stmt_while  (&(yyvsp[-2].expr), &(yyvsp[0].stmt));}
#line 1875 "src/parser.c"
    break;

  case 40:
#line 170 "src/parser.bison"
                                                                                                        {(yyval.stmt)=stmt_ret    (NULL);}
#line 1881 "src/parser.c"
    break;

  case 41:
#line 171 "src/parser.bison"
                                                                                                        {(yyval.stmt)=stmt_ret    (&(yyvsp[-1].expr));}
#line 1887 "src/parser.c"
    break;

  case 42:
#line 172 "src/parser.bison"
                                                                                                                {(yyval.stmt)=stmt_var    (&(yyvsp[0].idents));}
#line 1893 "src/parser.c"
    break;

  case 43:
#line 173 "src/parser.bison"
                                                                                                                {(yyval.stmt)=stmt_expr   (&(yyvsp[-1].expr));}
#line 1899 "src/parser.c"
    break;

  case 44:
#line 174 "src/parser.bison"
                                                                                                        {(yyval.stmt)=(yyvsp[-1].stmt);}
#line 1905 "src/parser.c"
    break;

  case 49:
#line 180 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_icnst(&(yyvsp[0].ival));}
#line 1911 "src/parser.c"
    break;

  case 50:
#line 181 "src/parser.bison"
                                                                                                                {(yyval.expr)=expr_scnst(&(yyvsp[0].strval));}
#line 1917 "src/parser.c"
    break;

  case 51:
#line 182 "src/parser.bison"
                                                                                                                {(yyval.expr)=expr_ident(&(yyvsp[0].strval));}
#line 1923 "src/parser.c"
    break;

  case 52:
#line 183 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_call (&(yyvsp[-3].expr), &(yyvsp[-1].exprs));}
#line 1929 "src/parser.c"
    break;

  case 53:
#line 184 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_INDEX,     &(yyvsp[-3].expr), &(yyvsp[-1].expr));}
#line 1935 "src/parser.c"
    break;

  case 54:
#line 185 "src/parser.bison"
                                                                                                        {(yyval.expr)=(yyvsp[-1].expr);}
#line 1941 "src/parser.c"
    break;

  case 55:
#line 187 "src/parser.bison"
                                                                                                                {(yyval.expr)=expr_math1(OP_0_MINUS,   &(yyvsp[0].expr));}
#line 1947 "src/parser.c"
    break;

  case 56:
#line 188 "src/parser.bison"
                                                                                                                {(yyval.expr)=expr_math1(OP_LOGIC_NOT, &(yyvsp[0].expr));}
#line 1953 "src/parser.c"
    break;

  case 57:
#line 189 "src/parser.bison"
                                                                                                                {(yyval.expr)=expr_math1(OP_BIT_NOT,   &(yyvsp[0].expr));}
#line 1959 "src/parser.c"
    break;

  case 58:
#line 190 "src/parser.bison"
                                                                                                                {(yyval.expr)=expr_math1(OP_ADROF,     &(yyvsp[0].expr));}
#line 1965 "src/parser.c"
    break;

  case 59:
#line 191 "src/parser.bison"
                                                                                                                {(yyval.expr)=expr_math1(OP_DEREF,     &(yyvsp[0].expr));}
#line 1971 "src/parser.c"
    break;

  case 60:
#line 193 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_MUL,       &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1977 "src/parser.c"
    break;

  case 61:
#line 194 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_math2(OP_DIV,       &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1983 "src/parser.c"
    break;

  case 62:
#line 195 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_math2(OP_MOD,       &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1989 "src/parser.c"
    break;

  case 63:
#line 197 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_ADD,       &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1995 "src/parser.c"
    break;

  case 64:
#line 198 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_math2(OP_SUB,       &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2001 "src/parser.c"
    break;

  case 65:
#line 200 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_LT,        &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2007 "src/parser.c"
    break;

  case 66:
#line 201 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_math2(OP_LE,        &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2013 "src/parser.c"
    break;

  case 67:
#line 202 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_math2(OP_GT,        &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2019 "src/parser.c"
    break;

  case 68:
#line 203 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_math2(OP_GE,        &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2025 "src/parser.c"
    break;

  case 69:
#line 205 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_NE,        &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2031 "src/parser.c"
    break;

  case 70:
#line 206 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_math2(OP_EQ,        &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2037 "src/parser.c"
    break;

  case 71:
#line 208 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_BIT_AND,   &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2043 "src/parser.c"
    break;

  case 72:
#line 209 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_BIT_XOR,   &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2049 "src/parser.c"
    break;

  case 73:
#line 210 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_BIT_OR,    &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2055 "src/parser.c"
    break;

  case 74:
#line 211 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_SHIFT_L,   &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2061 "src/parser.c"
    break;

  case 75:
#line 212 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_math2(OP_SHIFT_R,   &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2067 "src/parser.c"
    break;

  case 76:
#line 214 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_LOGIC_AND, &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2073 "src/parser.c"
    break;

  case 77:
#line 215 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_LOGIC_OR,  &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2079 "src/parser.c"
    break;

  case 78:
#line 217 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_ASSIGN,    &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2085 "src/parser.c"
    break;

  case 79:
#line 218 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_matha(OP_ADD,       &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2091 "src/parser.c"
    break;

  case 80:
#line 219 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_matha(OP_SUB,       &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2097 "src/parser.c"
    break;

  case 81:
#line 220 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_matha(OP_MUL,       &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2103 "src/parser.c"
    break;

  case 82:
#line 221 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_matha(OP_DIV,       &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2109 "src/parser.c"
    break;

  case 83:
#line 222 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_matha(OP_MOD,       &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2115 "src/parser.c"
    break;

  case 84:
#line 223 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_matha(OP_BIT_AND,   &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2121 "src/parser.c"
    break;

  case 85:
#line 224 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_matha(OP_BIT_OR,    &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2127 "src/parser.c"
    break;

  case 86:
#line 225 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_matha(OP_BIT_XOR,   &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2133 "src/parser.c"
    break;

  case 87:
#line 226 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_matha(OP_SHIFT_L,   &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2139 "src/parser.c"
    break;

  case 88:
#line 227 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_matha(OP_SHIFT_R,   &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 2145 "src/parser.c"
    break;

  case 89:
#line 229 "src/parser.bison"
                                                                                        {(yyval.stmt)=(yyvsp[0].stmt); (yyval.stmt).iasm->qualifiers=(yyvsp[-1].asm_qual); (yyval.stmt).iasm->qualifiers.is_volatile |= !(yyval.stmt).iasm->outputs; }
#line 2151 "src/parser.c"
    break;

  case 90:
#line 231 "src/parser.bison"
                                                                                                        {(yyval.asm_qual)=(iasm_qual_t) {.is_volatile=0, .is_inline=0, .is_goto=0};}
#line 2157 "src/parser.c"
    break;

  case 91:
#line 232 "src/parser.bison"
                                                                                                        {(yyval.asm_qual)=(yyvsp[-1].asm_qual); (yyval.asm_qual).is_volatile=1;}
#line 2163 "src/parser.c"
    break;

  case 92:
#line 233 "src/parser.bison"
                                                                                                        {(yyval.asm_qual)=(yyvsp[-1].asm_qual); (yyval.asm_qual).is_inline=1;}
#line 2169 "src/parser.c"
    break;

  case 93:
#line 234 "src/parser.bison"
                                                                                                        {(yyval.asm_qual)=(yyvsp[-1].asm_qual); (yyval.asm_qual).is_goto=1;}
#line 2175 "src/parser.c"
    break;

  case 94:
#line 239 "src/parser.bison"
                                                                                                {(yyval.stmt)=stmt_iasm(&(yyvsp[-7].strval), &(yyvsp[-5].asm_regs),  &(yyvsp[-3].asm_regs),  NULL);}
#line 2181 "src/parser.c"
    break;

  case 95:
#line 242 "src/parser.bison"
                                                                                                {(yyval.stmt)=stmt_iasm(&(yyvsp[-5].strval), &(yyvsp[-3].asm_regs),  &(yyvsp[-1].asm_regs),  NULL);}
#line 2187 "src/parser.c"
    break;

  case 96:
#line 244 "src/parser.bison"
                                                                                                {(yyval.stmt)=stmt_iasm(&(yyvsp[-3].strval), &(yyvsp[-1].asm_regs),  NULL, NULL);}
#line 2193 "src/parser.c"
    break;

  case 97:
#line 245 "src/parser.bison"
                                                                                                        {(yyval.stmt)=stmt_iasm(&(yyvsp[-1].strval), NULL, NULL, NULL);}
#line 2199 "src/parser.c"
    break;

  case 102:
#line 251 "src/parser.bison"
                                                                                                {(yyval.asm_regs)=(yyvsp[0].asm_regs);}
#line 2205 "src/parser.c"
    break;

  case 103:
#line 252 "src/parser.bison"
                                                                                                                {(yyval.asm_regs)=iasm_regs_empty();}
#line 2211 "src/parser.c"
    break;

  case 104:
#line 253 "src/parser.bison"
                                                                                        {(yyval.asm_regs)=iasm_regs_cat(&(yyvsp[0].asm_regs), &(yyvsp[-2].asm_reg));}
#line 2217 "src/parser.c"
    break;

  case 105:
#line 254 "src/parser.bison"
                                                                                                                {(yyval.asm_regs)=iasm_regs_one(&(yyvsp[0].asm_reg));}
#line 2223 "src/parser.c"
    break;

  case 106:
#line 255 "src/parser.bison"
                                                                        {(yyval.asm_reg)=stmt_iasm_reg(&(yyvsp[-5].strval),  &(yyvsp[-3].strval), &(yyvsp[-1].expr));}
#line 2229 "src/parser.c"
    break;

  case 107:
#line 256 "src/parser.bison"
                                                                                                {(yyval.asm_reg)=stmt_iasm_reg(NULL, &(yyvsp[-3].strval), &(yyvsp[-1].expr));}
#line 2235 "src/parser.c"
    break;


#line 2239 "src/parser.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (ctx, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *, YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (ctx, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, ctx);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp, ctx);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;


#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (ctx, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, ctx);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[+*yyssp], yyvsp, ctx);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 257 "src/parser.bison"


void *make_copy(void *mem, size_t size) {
	if (!mem) return NULL;
	void *copy = malloc(size);
	memcpy(copy, mem, size);
	return copy;
}
