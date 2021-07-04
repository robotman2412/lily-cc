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
#include <malloc.h>
#include <string.h>


#line 124 "src/parser.c"

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

#line 187 "src/parser.c"

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
typedef yytype_int8 yy_state_t;

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
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   337

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  37
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  20
/* YYNRULES -- Number of rules.  */
#define YYNRULES  66
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  126

#define YYUNDEFTOK  2
#define YYMAXUTOK   291


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
      35,    36
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,    94,    94,    94,    95,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   116,   117,   119,   120,   121,
     122,   123,   124,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   144,   145,   146,   147,   148,   149,   150,   151,   152,
     153,   154,   155,   156,   157,   158,   159
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "\"if\"", "\"else\"", "\"while\"",
  "\"for\"", "\"return\"", "\"(\"", "\")\"", "\"{\"", "\"}\"", "\";\"",
  "\",\"", "TKN_NUM", "TKN_IDENT", "TKN_GARBAGE", "\"++\"", "\"--\"",
  "\"+\"", "\"-\"", "\"=\"", "\"*\"", "\"/\"", "\"%\"", "\"then\"",
  "\"struct\"", "\"enum\"", "\"void\"", "\"signed\"", "\"unsigned\"",
  "\"char\"", "\"short\"", "\"long\"", "\"int\"", "\"float\"",
  "\"double\"", "$accept", "library", "$@1", "funcdef", "$@2", "funcdefs",
  "opt_funcparams", "funcparams", "vardecl", "vardecls", "varassign",
  "statmt", "statmts", "expr", "exprs", "opt_exprs", "opt_else",
  "type_spec", "int_spec", "opt_int", YY_NULLPTR
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
     285,   286,   287,   288,   289,   290,   291
};
# endif

#define YYPACT_NINF (-72)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-10)

#define yytable_value_is_error(Yyn) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -72,     4,   -72,   -72,    62,    12,   -72,   301,    15,    24,
      36,   -72,    83,    83,   -72,    19,     8,   -72,   -72,   -72,
      42,   -72,   -72,   -72,   -72,   -25,   -72,   -72,   -72,   -72,
      19,   -72,   -72,    53,    98,   -72,   301,    57,    58,    63,
      41,    41,   -72,   -72,   -72,   -72,   -72,    60,   -72,    61,
      59,    18,    39,     9,    41,    41,   200,   133,   251,   132,
     -72,    41,   -72,   -72,   -72,    41,    41,    41,    41,    41,
      41,    54,    73,    66,    84,   301,    85,   -72,   268,   285,
      41,   -72,   -72,   -72,    -6,    89,   102,   201,   201,    -6,
      20,    20,    20,    41,   222,   -72,   -72,   103,   -72,   200,
     200,   167,    41,   -72,    -6,   106,   -72,    59,   166,   -72,
     116,   -72,    41,    -6,   222,   -72,   -72,   200,   -72,   302,
     -72,    59,   -72,   200,   -72,   -72
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       2,     0,     8,     1,     0,     0,     7,     0,     0,     0,
       0,    51,     0,     0,    60,    66,    66,    62,    55,    56,
       0,    54,    31,    58,    59,    66,    52,    53,    65,    61,
      66,    57,    63,     0,     0,    64,    10,     0,     0,     0,
       0,     0,    31,     6,    30,    34,    33,     0,    32,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      28,    48,    29,    43,    44,     0,     0,     0,     0,     0,
       0,    22,    15,     0,     0,     0,     0,    12,     0,     0,
       0,    27,    35,    23,    46,    47,     0,    38,    39,    37,
      40,    41,    42,     0,     0,    31,    13,     0,    14,     0,
       0,     0,     0,    36,    21,    16,    20,     0,     0,    11,
      50,    25,     0,    45,     0,    19,     5,     0,    24,     0,
      18,     0,    49,     0,    17,    26
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -72,   -72,   -72,   -72,   -72,   -72,   -72,   -72,   -72,   -72,
     -71,   -53,   -41,   -35,   -72,   -72,   -72,    -7,    47,    -8
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     1,     2,     6,     7,     4,    51,    52,    47,   105,
      72,    48,    34,    49,    85,    86,   118,    50,    21,    32
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      20,    59,    61,    80,     3,    57,    58,    29,    30,    28,
      76,    63,    64,    65,    66,    67,    68,    69,    70,    78,
      79,     8,    35,   106,    77,    22,    84,    73,    61,    53,
      87,    88,    89,    90,    91,    92,   115,    63,    64,    23,
      74,    30,    28,   120,    31,   101,   110,   111,    -9,    41,
     124,    24,    75,    28,   108,    45,    46,    33,   104,    26,
      27,    36,    -3,     5,   122,    54,    55,   113,    97,    61,
     125,    56,    60,    62,    71,    93,    95,   119,    63,    64,
      65,    66,    67,    68,    69,    70,    94,   107,    -4,    -4,
      -4,    -4,    -4,    -4,    -4,    -4,    -4,    -4,    -4,    96,
      98,    37,   102,    38,    39,    40,    41,   121,    42,    43,
      44,   103,    45,    46,    14,    15,    25,    17,   109,   114,
     117,     0,     0,     0,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    37,     0,    38,    39,    40,
      41,    61,    42,    83,    44,    81,    45,    46,     0,     0,
      63,    64,    65,    66,    67,    68,    69,    70,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    37,
       0,    38,    39,    40,    41,    61,    42,   116,    44,   112,
      45,    46,     0,     0,    63,    64,    65,    66,    67,    68,
      69,    70,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    37,     0,    38,    39,    40,    41,    61,
      42,     0,    44,     0,    45,    46,     0,     0,    63,    64,
       0,     0,     0,    68,    69,    70,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    71,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    61,
      82,     0,     0,     0,     0,     0,     0,     0,    63,    64,
      65,    66,    67,    68,    69,    70,    61,    99,     0,     0,
       0,     0,     0,     0,     0,    63,    64,    65,    66,    67,
      68,    69,    70,    61,   100,     0,     0,     0,     0,     0,
       0,     0,    63,    64,    65,    66,    67,    68,    69,    70,
      61,   123,     0,     0,     0,     0,     0,     0,     0,    63,
      64,    65,    66,    67,    68,    69,    70,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19
};

static const yytype_int8 yycheck[] =
{
       7,    42,     8,    56,     0,    40,    41,    15,    33,    34,
       1,    17,    18,    19,    20,    21,    22,    23,    24,    54,
      55,     9,    30,    94,    15,    10,    61,     9,     8,    36,
      65,    66,    67,    68,    69,    70,   107,    17,    18,    15,
       1,    33,    34,   114,    36,    80,    99,   100,     9,     8,
     121,    15,    13,    34,    95,    14,    15,    15,    93,    12,
      13,     8,     0,     1,   117,     8,     8,   102,    75,     8,
     123,     8,    12,    12,    15,    21,    10,   112,    17,    18,
      19,    20,    21,    22,    23,    24,    13,    94,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    15,
      15,     3,    13,     5,     6,     7,     8,   114,    10,    11,
      12,     9,    14,    15,    31,    32,    33,    34,    15,    13,
       4,    -1,    -1,    -1,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,     3,    -1,     5,     6,     7,
       8,     8,    10,    11,    12,    12,    14,    15,    -1,    -1,
      17,    18,    19,    20,    21,    22,    23,    24,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,     3,
      -1,     5,     6,     7,     8,     8,    10,    11,    12,    12,
      14,    15,    -1,    -1,    17,    18,    19,    20,    21,    22,
      23,    24,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,     3,    -1,     5,     6,     7,     8,     8,
      10,    -1,    12,    -1,    14,    15,    -1,    -1,    17,    18,
      -1,    -1,    -1,    22,    23,    24,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    15,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,     8,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,    18,
      19,    20,    21,    22,    23,    24,     8,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    17,    18,    19,    20,    21,
      22,    23,    24,     8,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    17,    18,    19,    20,    21,    22,    23,    24,
       8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,
      18,    19,    20,    21,    22,    23,    24,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    38,    39,     0,    42,     1,    40,    41,     9,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      54,    55,    10,    15,    15,    33,    55,    55,    34,    56,
      33,    36,    56,    15,    49,    56,     8,     3,     5,     6,
       7,     8,    10,    11,    12,    14,    15,    45,    48,    50,
      54,    43,    44,    54,     8,     8,     8,    50,    50,    49,
      12,     8,    12,    17,    18,    19,    20,    21,    22,    23,
      24,    15,    47,     9,     1,    13,     1,    15,    50,    50,
      48,    12,     9,    11,    50,    51,    52,    50,    50,    50,
      50,    50,    50,    21,    13,    10,    15,    54,    15,     9,
       9,    50,    13,     9,    50,    46,    47,    54,    49,    15,
      48,    48,    12,    50,    13,    47,    11,     4,    53,    50,
      47,    54,    48,     9,    47,    48
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_int8 yyr1[] =
{
       0,    37,    39,    38,    41,    40,    40,    42,    42,    43,
      43,    44,    44,    44,    44,    45,    45,    46,    46,    46,
      46,    47,    47,    48,    48,    48,    48,    48,    48,    48,
      48,    49,    49,    50,    50,    50,    50,    50,    50,    50,
      50,    50,    50,    50,    50,    51,    51,    52,    52,    53,
      53,    54,    54,    54,    54,    54,    54,    54,    54,    54,
      55,    55,    55,    55,    55,    56,    56
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     0,     9,     5,     2,     0,     1,
       0,     4,     2,     3,     3,     2,     4,     4,     3,     2,
       1,     3,     1,     3,     6,     5,     8,     3,     2,     2,
       1,     0,     2,     1,     1,     3,     4,     3,     3,     3,
       3,     3,     3,     2,     2,     3,     1,     1,     0,     2,
       0,     1,     2,     2,     1,     1,     1,     2,     2,     2,
       1,     2,     1,     2,     3,     1,     0
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
  switch (yytype)
    {
    case 3: /* "if"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1228 "src/parser.c"
        break;

    case 4: /* "else"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1234 "src/parser.c"
        break;

    case 5: /* "while"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1240 "src/parser.c"
        break;

    case 6: /* "for"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1246 "src/parser.c"
        break;

    case 7: /* "return"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1252 "src/parser.c"
        break;

    case 8: /* "("  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1258 "src/parser.c"
        break;

    case 9: /* ")"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1264 "src/parser.c"
        break;

    case 10: /* "{"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1270 "src/parser.c"
        break;

    case 11: /* "}"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1276 "src/parser.c"
        break;

    case 12: /* ";"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1282 "src/parser.c"
        break;

    case 13: /* ","  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1288 "src/parser.c"
        break;

    case 15: /* TKN_IDENT  */
#line 43 "src/parser.bison"
            {printf("free_ident($$.ident);\n");}
#line 1294 "src/parser.c"
        break;

    case 16: /* TKN_GARBAGE  */
#line 44 "src/parser.bison"
            {printf("free_garbage($$.garbage);\n");}
#line 1300 "src/parser.c"
        break;

    case 17: /* "++"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1306 "src/parser.c"
        break;

    case 18: /* "--"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1312 "src/parser.c"
        break;

    case 19: /* "+"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1318 "src/parser.c"
        break;

    case 20: /* "-"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1324 "src/parser.c"
        break;

    case 21: /* "="  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1330 "src/parser.c"
        break;

    case 22: /* "*"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1336 "src/parser.c"
        break;

    case 23: /* "/"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1342 "src/parser.c"
        break;

    case 24: /* "%"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1348 "src/parser.c"
        break;

    case 26: /* "struct"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1354 "src/parser.c"
        break;

    case 27: /* "enum"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1360 "src/parser.c"
        break;

    case 28: /* "void"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1366 "src/parser.c"
        break;

    case 29: /* "signed"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1372 "src/parser.c"
        break;

    case 30: /* "unsigned"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1378 "src/parser.c"
        break;

    case 31: /* "char"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1384 "src/parser.c"
        break;

    case 32: /* "short"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1390 "src/parser.c"
        break;

    case 33: /* "long"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1396 "src/parser.c"
        break;

    case 34: /* "int"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1402 "src/parser.c"
        break;

    case 35: /* "float"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1408 "src/parser.c"
        break;

    case 36: /* "double"  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1414 "src/parser.c"
        break;

    case 40: /* funcdef  */
#line 45 "src/parser.bison"
            {printf("free_funcdef(&$$);\n");}
#line 1420 "src/parser.c"
        break;

    case 43: /* opt_funcparams  */
#line 46 "src/parser.bison"
            {printf("free_idents(&$$);\n");}
#line 1426 "src/parser.c"
        break;

    case 44: /* funcparams  */
#line 46 "src/parser.bison"
            {printf("free_idents(&$$);\n");}
#line 1432 "src/parser.c"
        break;

    case 45: /* vardecl  */
#line 48 "src/parser.bison"
            {printf("free_vardecls(&$$);\n");}
#line 1438 "src/parser.c"
        break;

    case 46: /* vardecls  */
#line 48 "src/parser.bison"
            {printf("free_vardecls(&$$);\n");}
#line 1444 "src/parser.c"
        break;

    case 47: /* varassign  */
#line 47 "src/parser.bison"
            {printf("free_vardecl(&$$);\n");}
#line 1450 "src/parser.c"
        break;

    case 48: /* statmt  */
#line 49 "src/parser.bison"
            {printf("free_statmt(&$$);\n");}
#line 1456 "src/parser.c"
        break;

    case 49: /* statmts  */
#line 50 "src/parser.bison"
            {printf("free_statmts(&$$);\n");}
#line 1462 "src/parser.c"
        break;

    case 50: /* expr  */
#line 51 "src/parser.bison"
            {printf("free_expr(&$$);\n");}
#line 1468 "src/parser.c"
        break;

    case 51: /* exprs  */
#line 52 "src/parser.bison"
            {printf("free_exprs(&$$);\n");}
#line 1474 "src/parser.c"
        break;

    case 52: /* opt_exprs  */
#line 52 "src/parser.bison"
            {printf("free_exprs(&$$);\n");}
#line 1480 "src/parser.c"
        break;

    case 53: /* opt_else  */
#line 49 "src/parser.bison"
            {printf("free_statmt(&$$);\n");}
#line 1486 "src/parser.c"
        break;

    case 54: /* type_spec  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1492 "src/parser.c"
        break;

    case 55: /* int_spec  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1498 "src/parser.c"
        break;

    case 56: /* opt_int  */
#line 53 "src/parser.bison"
            {printf("free_pos(&$$);\n");}
#line 1504 "src/parser.c"
        break;

      default:
        break;
    }
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
  case 2:
#line 94 "src/parser.bison"
                        {init(ctx);}
#line 1774 "src/parser.c"
    break;

  case 4:
#line 95 "src/parser.bison"
                        {pre_func(ctx);}
#line 1780 "src/parser.c"
    break;

  case 5:
#line 95 "src/parser.bison"
                                                                                                           {(yyval.func)=post_func(ctx, (yyvsp[-6].ident), &(yyvsp[-4].idents), &(yyvsp[-1].statmts));}
#line 1786 "src/parser.c"
    break;

  case 6:
#line 96 "src/parser.bison"
                                                                                                                                                                   {printf("err\n");/*dispose of stuff*/}
#line 1792 "src/parser.c"
    break;

  case 9:
#line 99 "src/parser.bison"
                                                                                {(yyval.idents)=(yyvsp[0].idents);}
#line 1798 "src/parser.c"
    break;

  case 10:
#line 100 "src/parser.bison"
                                                                                                {(yyval.idents)=param_empty (ctx);						(yyval.idents).pos=pos_empty(ctx->tokeniser_ctx);}
#line 1804 "src/parser.c"
    break;

  case 11:
#line 101 "src/parser.bison"
                                                                {(yyval.idents)=param_cat   (ctx, &(yyvsp[-3].idents), (yyvsp[0].ident));				(yyval.idents).pos=pos_merge((yyvsp[-3].idents).pos, (yyvsp[0].ident).pos);}
#line 1810 "src/parser.c"
    break;

  case 12:
#line 102 "src/parser.bison"
                                                                                        {(yyval.idents)=param_new   (ctx, (yyvsp[0].ident));					(yyval.idents).pos=pos_merge((yyvsp[-1].pos)/*.pos*/, (yyvsp[0].ident).pos);}
#line 1816 "src/parser.c"
    break;

  case 13:
#line 103 "src/parser.bison"
                                                                                {(yyval.idents)=param_empty (ctx);						(yyval.idents).pos=pos_merge((yyvsp[-2].idents).pos, (yyvsp[0].ident).pos);		syntax_error(ctx, (yyval.idents).pos, "Expected parameter");}
#line 1822 "src/parser.c"
    break;

  case 14:
#line 104 "src/parser.bison"
                                                                                {(yyval.idents)=param_empty (ctx); 						(yyval.idents).pos=pos_merge((yyvsp[-2].pos)/*.pos*/, (yyvsp[0].ident).pos);	syntax_error(ctx, (yyval.idents).pos, "Expected parameter");}
#line 1828 "src/parser.c"
    break;

  case 15:
#line 105 "src/parser.bison"
                                                                                {(yyval.vars)=vars_new    (ctx, &(yyvsp[0].var));					(yyval.vars).pos=pos_merge((yyvsp[-1].pos)/*.pos*/, (yyvsp[0].var).pos);}
#line 1834 "src/parser.c"
    break;

  case 16:
#line 106 "src/parser.bison"
                                                                        {(yyval.vars)=vars_cat    (ctx, &(yyvsp[0].vars), &(yyvsp[-2].var));			(yyval.vars).pos=pos_merge((yyvsp[-3].pos)/*.pos*/, (yyvsp[0].vars).pos);}
#line 1840 "src/parser.c"
    break;

  case 17:
#line 107 "src/parser.bison"
                                                                {(yyval.vars)=vars_cat    (ctx, &(yyvsp[-3].vars), &(yyvsp[0].var));			(yyval.vars).pos=pos_merge((yyvsp[-3].vars).pos, (yyvsp[0].var).pos);}
#line 1846 "src/parser.c"
    break;

  case 18:
#line 108 "src/parser.bison"
                                                                                {(yyval.vars)=vars_cat    (ctx, &(yyvsp[-2].vars), &(yyvsp[0].var));			(yyval.vars).pos=pos_merge((yyvsp[-2].vars).pos, (yyvsp[0].var).pos);}
#line 1852 "src/parser.c"
    break;

  case 19:
#line 109 "src/parser.bison"
                                                                                        {(yyval.vars)=vars_new    (ctx, &(yyvsp[0].var));					(yyval.vars).pos=pos_merge((yyvsp[-1].pos)/*.pos*/, (yyvsp[0].var).pos);}
#line 1858 "src/parser.c"
    break;

  case 20:
#line 110 "src/parser.bison"
                                                                                                {(yyval.vars)=vars_new    (ctx, &(yyvsp[0].var));					(yyval.vars).pos=(yyvsp[0].var).pos;}
#line 1864 "src/parser.c"
    break;

  case 21:
#line 111 "src/parser.bison"
                                                                                {(yyval.var)=decl_assign (ctx, (yyvsp[-2].ident), &(yyvsp[0].expr));				(yyval.var).pos=pos_merge((yyvsp[-2].ident).pos, (yyvsp[0].expr).pos);}
#line 1870 "src/parser.c"
    break;

  case 22:
#line 112 "src/parser.bison"
                                                                                                {(yyval.var)=decl        (ctx, (yyvsp[0].ident));					(yyval.var).pos=(yyvsp[0].ident).pos;}
#line 1876 "src/parser.c"
    break;

  case 23:
#line 113 "src/parser.bison"
                                                                                {(yyval.statmt)=statmt_multi(ctx, &(yyvsp[-1].statmts));					(yyval.statmt).pos=pos_merge((yyvsp[-2].pos), (yyvsp[0].pos));}
#line 1882 "src/parser.c"
    break;

  case 24:
#line 115 "src/parser.bison"
                                                                                                {(yyval.statmt)=statmt_if   (ctx, &(yyvsp[-3].expr), &(yyvsp[-1].statmt), &(yyvsp[0].statmt));		(yyval.statmt).pos=pos_merge((yyvsp[-5].pos), (yyvsp[0].statmt).pos);}
#line 1888 "src/parser.c"
    break;

  case 25:
#line 116 "src/parser.bison"
                                                                                {(yyval.statmt)=statmt_while(ctx, &(yyvsp[-2].expr), &(yyvsp[0].statmt));			(yyval.statmt).pos=pos_merge((yyvsp[-4].pos), (yyvsp[0].statmt).pos);}
#line 1894 "src/parser.c"
    break;

  case 26:
#line 118 "src/parser.bison"
                                                                                        {(yyval.statmt)=statmt_for  (ctx, &(yyvsp[-5].statmt), &(yyvsp[-4].expr), &(yyvsp[-2].expr), &(yyvsp[0].statmt));	(yyval.statmt).pos=pos_merge((yyvsp[-7].pos), (yyvsp[0].statmt).pos);}
#line 1900 "src/parser.c"
    break;

  case 27:
#line 119 "src/parser.bison"
                                                                                        {(yyval.statmt)=statmt_ret  (ctx, &(yyvsp[-1].expr));					(yyval.statmt).pos=pos_merge((yyvsp[-2].pos), (yyvsp[0].pos));}
#line 1906 "src/parser.c"
    break;

  case 28:
#line 120 "src/parser.bison"
                                                                                                {(yyval.statmt)=statmt_var  (ctx, &(yyvsp[-1].vars));					(yyval.statmt).pos=pos_merge((yyvsp[-1].vars).pos, (yyvsp[0].pos));}
#line 1912 "src/parser.c"
    break;

  case 29:
#line 121 "src/parser.bison"
                                                                                                {(yyval.statmt)=statmt_expr (ctx, &(yyvsp[-1].expr));					(yyval.statmt).pos=pos_merge((yyvsp[-1].expr).pos, (yyvsp[0].pos));}
#line 1918 "src/parser.c"
    break;

  case 30:
#line 122 "src/parser.bison"
                                                                                                        {(yyval.statmt)=statmt_nop  (ctx);						(yyval.statmt).pos=(yyvsp[0].pos);}
#line 1924 "src/parser.c"
    break;

  case 31:
#line 123 "src/parser.bison"
                                                                                        {(yyval.statmts)=statmts_new (ctx);						(yyval.statmts).pos=pos_empty(ctx->tokeniser_ctx);}
#line 1930 "src/parser.c"
    break;

  case 32:
#line 124 "src/parser.bison"
                                                                                        {(yyval.statmts)=statmts_cat (ctx, &(yyvsp[-1].statmts), &(yyvsp[0].statmt));			(yyval.statmts).pos=pos_merge((yyvsp[-1].statmts).pos, (yyvsp[0].statmt).pos);}
#line 1936 "src/parser.c"
    break;

  case 33:
#line 125 "src/parser.bison"
                                                                                        {(yyval.expr)=expr_var    (ctx, (yyvsp[0].ident));					(yyval.expr).pos=(yyvsp[0].ident).pos;}
#line 1942 "src/parser.c"
    break;

  case 34:
#line 126 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_const  (ctx, (yyvsp[0].ival));					(yyval.expr).pos=(yyvsp[0].ival).pos;}
#line 1948 "src/parser.c"
    break;

  case 35:
#line 127 "src/parser.bison"
                                                                                        {(yyval.expr)=(yyvsp[-1].expr);										(yyval.expr).pos=pos_merge((yyvsp[-2].pos), (yyvsp[0].pos));}
#line 1954 "src/parser.c"
    break;

  case 36:
#line 128 "src/parser.bison"
                                                                                {(yyval.expr)=expr_call   (ctx, &(yyvsp[-3].expr), &(yyvsp[-1].exprs));			(yyval.expr).pos=pos_merge((yyvsp[-3].expr).pos, (yyvsp[0].pos));}
#line 1960 "src/parser.c"
    break;

  case 37:
#line 129 "src/parser.bison"
                                                                                        {(yyval.expr)=expr_assign (ctx, &(yyvsp[-2].expr), &(yyvsp[0].expr));			(yyval.expr).pos=pos_merge((yyvsp[-2].expr).pos, (yyvsp[0].expr).pos);}
#line 1966 "src/parser.c"
    break;

  case 38:
#line 130 "src/parser.bison"
                                                                                        {(yyval.expr)=expr_math2  (ctx, &(yyvsp[-2].expr), &(yyvsp[0].expr), OP_ADD);	(yyval.expr).pos=pos_merge((yyvsp[-2].expr).pos, (yyvsp[0].expr).pos);}
#line 1972 "src/parser.c"
    break;

  case 39:
#line 131 "src/parser.bison"
                                                                                {(yyval.expr)=expr_math2  (ctx, &(yyvsp[-2].expr), &(yyvsp[0].expr), OP_SUB);	(yyval.expr).pos=pos_merge((yyvsp[-2].expr).pos, (yyvsp[0].expr).pos);}
#line 1978 "src/parser.c"
    break;

  case 40:
#line 132 "src/parser.bison"
                                                                                        {(yyval.expr)=expr_math2  (ctx, &(yyvsp[-2].expr), &(yyvsp[0].expr), OP_MUL);	(yyval.expr).pos=pos_merge((yyvsp[-2].expr).pos, (yyvsp[0].expr).pos);}
#line 1984 "src/parser.c"
    break;

  case 41:
#line 133 "src/parser.bison"
                                                                                {(yyval.expr)=expr_math2  (ctx, &(yyvsp[-2].expr), &(yyvsp[0].expr), OP_DIV);	(yyval.expr).pos=pos_merge((yyvsp[-2].expr).pos, (yyvsp[0].expr).pos);}
#line 1990 "src/parser.c"
    break;

  case 42:
#line 134 "src/parser.bison"
                                                                                {(yyval.expr)=expr_math2  (ctx, &(yyvsp[-2].expr), &(yyvsp[0].expr), OP_REM);	(yyval.expr).pos=pos_merge((yyvsp[-2].expr).pos, (yyvsp[0].expr).pos);}
#line 1996 "src/parser.c"
    break;

  case 43:
#line 135 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_math1  (ctx, &(yyvsp[-1].expr), OP_INC);			(yyval.expr).pos=pos_merge((yyvsp[-1].expr).pos, (yyvsp[0].pos));}
#line 2002 "src/parser.c"
    break;

  case 44:
#line 136 "src/parser.bison"
                                                                                        {(yyval.expr)=expr_math1  (ctx, &(yyvsp[-1].expr), OP_DEC);			(yyval.expr).pos=pos_merge((yyvsp[-1].expr).pos, (yyvsp[0].pos));}
#line 2008 "src/parser.c"
    break;

  case 45:
#line 137 "src/parser.bison"
                                                                                {(yyval.exprs)=exprs_cat   (ctx, &(yyvsp[-2].exprs), &(yyvsp[0].expr));			(yyval.exprs).pos=pos_merge((yyvsp[-2].exprs).pos, (yyvsp[0].expr).pos);}
#line 2014 "src/parser.c"
    break;

  case 46:
#line 138 "src/parser.bison"
                                                                                                {(yyval.exprs)=exprs_new   (ctx, &(yyvsp[0].expr));					(yyval.exprs).pos=(yyvsp[0].expr).pos;}
#line 2020 "src/parser.c"
    break;

  case 47:
#line 139 "src/parser.bison"
                                                                                        {(yyval.exprs)=(yyvsp[0].exprs);										(yyval.exprs).pos=(yyvsp[0].exprs).pos;}
#line 2026 "src/parser.c"
    break;

  case 48:
#line 140 "src/parser.bison"
                                                                                                {(yyval.exprs)=exprs_new   (ctx, NULL);				(yyval.exprs).pos=pos_empty(ctx->tokeniser_ctx);}
#line 2032 "src/parser.c"
    break;

  case 49:
#line 141 "src/parser.bison"
                                                                                {(yyval.statmt)=(yyvsp[0].statmt);										(yyval.statmt).pos=pos_merge((yyvsp[-1].pos), (yyvsp[0].statmt).pos);}
#line 2038 "src/parser.c"
    break;

  case 50:
#line 142 "src/parser.bison"
                                                                                {(yyval.statmt)=statmt_nop  (ctx);						(yyval.statmt).pos=pos_empty(ctx->tokeniser_ctx);}
#line 2044 "src/parser.c"
    break;

  case 51:
#line 144 "src/parser.bison"
                                                                                        {(yyval.pos)=(yyvsp[0].pos);}
#line 2050 "src/parser.c"
    break;

  case 52:
#line 145 "src/parser.bison"
                                                                                        {(yyval.pos)=pos_merge((yyvsp[-1].pos), (yyvsp[0].pos)/*.pos*/);}
#line 2056 "src/parser.c"
    break;

  case 53:
#line 146 "src/parser.bison"
                                                                                        {(yyval.pos)=pos_merge((yyvsp[-1].pos), (yyvsp[0].pos)/*.pos*/);}
#line 2062 "src/parser.c"
    break;

  case 54:
#line 147 "src/parser.bison"
                                                                                                {(yyval.pos)=(yyvsp[0].pos);}
#line 2068 "src/parser.c"
    break;

  case 55:
#line 148 "src/parser.bison"
                                                                                                {(yyval.pos)=(yyvsp[0].pos);}
#line 2074 "src/parser.c"
    break;

  case 56:
#line 149 "src/parser.bison"
                                                                                                {(yyval.pos)=(yyvsp[0].pos);}
#line 2080 "src/parser.c"
    break;

  case 57:
#line 150 "src/parser.bison"
                                                                                        {(yyval.pos)=pos_merge((yyvsp[-1].pos), (yyvsp[0].pos));}
#line 2086 "src/parser.c"
    break;

  case 58:
#line 151 "src/parser.bison"
                                                                                        {(yyval.pos)=pos_merge((yyvsp[-1].pos), (yyvsp[0].ident).pos);}
#line 2092 "src/parser.c"
    break;

  case 59:
#line 152 "src/parser.bison"
                                                                                        {(yyval.pos)=pos_merge((yyvsp[-1].pos), (yyvsp[0].ident).pos);}
#line 2098 "src/parser.c"
    break;

  case 60:
#line 153 "src/parser.bison"
                                                                                        {(yyval.pos)=(yyvsp[0].pos);}
#line 2104 "src/parser.c"
    break;

  case 61:
#line 154 "src/parser.bison"
                                                                                        {(yyval.pos)=pos_merge((yyvsp[-1].pos), (yyvsp[0].pos)/*.pos*/);}
#line 2110 "src/parser.c"
    break;

  case 62:
#line 155 "src/parser.bison"
                                                                                                {(yyval.pos)=(yyvsp[0].pos);}
#line 2116 "src/parser.c"
    break;

  case 63:
#line 156 "src/parser.bison"
                                                                                        {(yyval.pos)=pos_merge((yyvsp[-1].pos), (yyvsp[0].pos)/*.pos*/);}
#line 2122 "src/parser.c"
    break;

  case 64:
#line 157 "src/parser.bison"
                                                                                {(yyval.pos)=pos_merge((yyvsp[-2].pos), (yyvsp[0].pos)/*.pos*/);}
#line 2128 "src/parser.c"
    break;

  case 65:
#line 158 "src/parser.bison"
                                                                                        {(yyval.pos)=(yyvsp[0].pos);}
#line 2134 "src/parser.c"
    break;

  case 66:
#line 159 "src/parser.bison"
                                                                                                {(yyval.pos)=pos_empty(ctx->tokeniser_ctx);}
#line 2140 "src/parser.c"
    break;


#line 2144 "src/parser.c"

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
#line 160 "src/parser.bison"


static void *make_copy(void *mem, size_t size) {
	void *copy = malloc(size);
	memcpy(copy, mem, size);
	return copy;
}

void init(parser_ctx_t *ctx) {
	ctx->scope = NULL;
	ctx->currentError = NULL;
	map_create(&ctx->funcMap);
}

void push_scope(parser_ctx_t *ctx) {
	scope_t *current = (scope_t *) malloc(sizeof(scope_t));
	current->parent = ctx->scope;
	map_create(&current->varMap);
	ctx->scope = current;
}

void pop_scope(parser_ctx_t *ctx) {
	map_delete_with_values(&ctx->scope->varMap);
	void *mem = ctx->scope;
	ctx->scope = ctx->scope->parent;
	free(mem);
}


void pre_func(parser_ctx_t *ctx) {
	push_scope(ctx);
}

idents_t param_cat(parser_ctx_t *ctx, idents_t *other, ident_t ident) {
	int num = other->numIdents + 1;
	ident_t *mem = (ident_t *) realloc(other->idents, sizeof(ident_t) * num);
	mem[other->numIdents] = ident;
	return (idents_t) {
		.numIdents = num,
		.idents = mem
	};
}

idents_t param_new(parser_ctx_t *ctx, ident_t ident) {
	ident_t *mem = (ident_t *) malloc(sizeof(ident_t));
	*mem = ident;
	return (idents_t) {
		.numIdents = 1,
		.idents = mem
	};
}

idents_t param_empty(parser_ctx_t *ctx) {
	return (idents_t) {
		.numIdents = 0,
		.idents = NULL
	};
}

funcdef_t post_func(parser_ctx_t *ctx, ident_t ident, idents_t *idents, statements_t *statmt) {
	funcdef_t f = {
		.pos = pos_merge(ident.pos, idents->pos),
		.ident = ident,
		.numParams = idents->numIdents,
		.paramIdents = idents->idents,
		.statements = statmt
	};
	pop_scope(ctx);
	funcdef_t *existing = map_get(&ctx->funcMap, ident.ident);
	if (existing) {
		// TODO: Standard error message.
		yyerror(ctx, "Function is already declared.");
	} else {
		funcdef_t *add = malloc(sizeof(funcdef_t));
		*add = f;
		map_set(&ctx->funcMap, ident.ident, add);
	}
	function_added(ctx, &f);
	return f;
}


vardecl_t decl_assign(parser_ctx_t *ctx, ident_t ident, expression_t *expression) {
	if (expression) expression = make_copy(expression, sizeof(expression_t));
	pos_t pos = ident.pos;
	if (expression) {
		pos = pos_merge(ident.pos, expression->pos);
	}
	vardecl_t f = {
		.ident = ident,
		.expr = expression
	};
	vardecl_t *existing = map_get(&ctx->scope->varMap, ident.ident);
	if (existing) {
		yyerror(ctx, "Variable is already declared in this scope.");
	} else {
		vardecl_t *add = malloc(sizeof(vardecl_t));
		*add = f;
		map_set(&ctx->scope->varMap, ident.ident, add);
	}
	return f;
}

vardecl_t decl(parser_ctx_t *ctx, ident_t ident) {
	decl_assign(ctx, ident, NULL);
}


statement_t statmt_nop(parser_ctx_t *ctx) {
	return (statement_t) {
		.type = STATMT_TYPE_NOP,
		.expr = NULL,
		.expr1 = NULL,
		.statement = NULL,
		.statement1 = NULL,
		.statements = NULL,
		.decls = NULL
	};
}

statement_t statmt_expr(parser_ctx_t *ctx, expression_t *expr) {
	return (statement_t) {
		.type = STATMT_TYPE_EXPR,
		.expr = make_copy(expr, sizeof(expression_t)),
		.expr1 = NULL,
		.statement = NULL,
		.statement1 = NULL,
		.statements = NULL,
		.decls = NULL
	};
}

statement_t statmt_ret(parser_ctx_t *ctx, expression_t *expr) {
	return (statement_t) {
		.type = STATMT_TYPE_RET,
		.expr = make_copy(expr, sizeof(expression_t)),
		.expr1 = NULL,
		.statement = NULL,
		.statement1 = NULL,
		.statements = NULL,
		.decls = NULL
	};
}

statement_t statmt_var(parser_ctx_t *ctx, vardecls_t *var) {
	return (statement_t) {
		.type = STATMT_TYPE_VAR,
		.expr = NULL,
		.expr1 = NULL,
		.statement = NULL,
		.statement1 = NULL,
		.statements = NULL,
		.decls = make_copy(var, sizeof(vardecls_t))
	};
}

statement_t statmt_if(parser_ctx_t *ctx, expression_t *expr, statement_t *code, statement_t *else_code) {
	return (statement_t) {
		.type = STATMT_TYPE_IF,
		.expr = make_copy(expr, sizeof(expression_t)),
		.expr1 = NULL,
		.statement = make_copy(code, sizeof(statement_t)),
		.statement1 = make_copy(else_code, sizeof(statement_t)),
		.statements = NULL,
		.decls = NULL
	};
}

statement_t statmt_while(parser_ctx_t *ctx, expression_t *expr, statement_t *code) {
	return (statement_t) {
		.type = STATMT_TYPE_WHILE,
		.expr = make_copy(expr, sizeof(expression_t)),
		.expr1 = NULL,
		.statement = make_copy(code, sizeof(statement_t)),
		.statement1 = NULL,
		.statements = NULL,
		.decls = NULL
	};
}

statement_t statmt_for(parser_ctx_t *ctx, statement_t *setup, expression_t *cond, expression_t *inc, statement_t *code) {
	return (statement_t) {
		.type = STATMT_TYPE_FOR,
		.expr = make_copy(cond, sizeof(expression_t)),
		.expr1 = make_copy(inc, sizeof(expression_t)),
		.statement = make_copy(setup, sizeof(statement_t)),
		.statement1 = make_copy(code, sizeof(statement_t)),
		.statements = NULL,
		.decls = NULL
	};
}

statement_t statmt_multi(parser_ctx_t *ctx, statements_t *code) {
	return (statement_t) {
		.type = STATMT_TYPE_MULTI,
		.expr = NULL,
		.expr1 = NULL,
		.statement = NULL,
		.statement1 = NULL,
		.statements = make_copy(code, sizeof(statements_t)),
		.decls = NULL
	};
}


expression_t expr_var(parser_ctx_t *ctx, ident_t ident) {
	return (expression_t) {
		.type = EXPR_TYPE_IDENT,
		.ident = ident.ident,
		.expr = NULL,
		.expr1 = NULL,
		.exprs = NULL,
		.value = 0
	};
}

expression_t expr_const(parser_ctx_t *ctx, ival_t iconst) {
	return (expression_t) {
		.type = EXPR_TYPE_ICONST,
		.ident = NULL,
		.expr = NULL,
		.expr1 = NULL,
		.exprs = NULL,
		.value = iconst.ival
	};
}

expression_t expr_call(parser_ctx_t *ctx, expression_t *func, expressions_t *params) {
	return (expression_t) {
		.type = EXPR_TYPE_INVOKE,
		.ident = NULL,
		.expr = make_copy(func, sizeof(expression_t)),
		.expr1 = NULL,
		.exprs = make_copy(params, sizeof(expressions_t)),
		.value = 0
	};
}

expression_t expr_assign(parser_ctx_t *ctx, expression_t *var, expression_t *val) {
	return (expression_t) {
		.type = EXPR_TYPE_ASSIGN,
		.ident = NULL,
		.expr = make_copy(var, sizeof(expression_t)),
		.expr1 = make_copy(val, sizeof(expression_t)),
		.exprs = NULL,
		.value = 0
	};
}

expression_t expr_math2(parser_ctx_t *ctx, expression_t *left, expression_t *right, operator_t oper) {
	return (expression_t) {
		.type = EXPR_TYPE_MATH2,
		.ident = NULL,
		.expr = make_copy(left, sizeof(expression_t)),
		.expr1 = make_copy(right, sizeof(expression_t)),
		.exprs = NULL,
		.value = 0,
		.oper = oper
	};
}

expression_t expr_math1(parser_ctx_t *ctx, expression_t *var, operator_t oper) {
	return (expression_t) {
		.type = EXPR_TYPE_MATH1,
		.ident = NULL,
		.expr = make_copy(var, sizeof(expression_t)),
		.expr1 = NULL,
		.exprs = NULL,
		.value = 0,
		.oper = oper
	};
}


statements_t statmts_cat(parser_ctx_t *ctx, statements_t *other, statement_t *add) {
	statement_t *mem = other->statements;
	mem = realloc(mem, sizeof(statement_t) * (other->num + 1));
	mem[other->num] = *add;
	return (statements_t) {
		.num = other->num + 1,
		.statements = mem
	};
}

statements_t statmts_new(parser_ctx_t *ctx) {
	statement_t *mem = malloc(0);
	return (statements_t) {
		.num = 0,
		.statements = mem
	};
}

expressions_t exprs_cat(parser_ctx_t *ctx, expressions_t *other, expression_t *add) {
	if (other->num) {
		expression_t *mem = other->exprs;
		mem = realloc(mem, sizeof(expression_t) * (other->num + 1));
		mem[other->num] = *add;
		return (expressions_t) {
			.num = other->num + 1,
			.exprs = mem
		};
	} else {
		return exprs_new(ctx, add);
	}
}

expressions_t exprs_new(parser_ctx_t *ctx, expression_t *first) {
	if (first) {
		expression_t *mem = malloc(sizeof(expression_t));
		*mem = *first;
		return (expressions_t) {
			.num = 1,
			.exprs = mem
		};
	} else {
		return (expressions_t) {
			.num = 0,
			.exprs = NULL
		};
	}
}

vardecls_t vars_cat(parser_ctx_t *ctx, vardecls_t *other, vardecl_t *add) {
	vardecl_t *mem = other->vars;
	mem = realloc(mem, sizeof(vardecl_t) * (other->num + 1));
	mem[other->num] = *add;
	return (vardecls_t) {
		.num = other->num + 1,
		.vars = mem
	};
}

vardecls_t vars_new(parser_ctx_t *ctx, vardecl_t *first) {
	vardecl_t *mem = malloc(sizeof(vardecl_t));
	*mem = *first;
	return (vardecls_t) {
		.num = 1,
		.vars = mem
	};
}

