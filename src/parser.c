/* A Bison parser, made by GNU Bison 3.7.6.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30706

/* Bison version string.  */
#define YYBISON_VERSION "3.7.6"

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

#line 75 "src/parser.c"

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

#include "parser.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_TKN_UNSIGNED = 3,               /* "unsigned"  */
  YYSYMBOL_TKN_SIGNED = 4,                 /* "signed"  */
  YYSYMBOL_TKN_FLOAT = 5,                  /* "float"  */
  YYSYMBOL_TKN_DOUBLE = 6,                 /* "double"  */
  YYSYMBOL_TKN_BOOL = 7,                   /* "_Bool"  */
  YYSYMBOL_TKN_CHAR = 8,                   /* "char"  */
  YYSYMBOL_TKN_SHORT = 9,                  /* "short"  */
  YYSYMBOL_TKN_INT = 10,                   /* "int"  */
  YYSYMBOL_TKN_LONG = 11,                  /* "long"  */
  YYSYMBOL_TKN_VOID = 12,                  /* "void"  */
  YYSYMBOL_TKN_VOLATILE = 13,              /* "volatile"  */
  YYSYMBOL_TKN_INLINE = 14,                /* "inline"  */
  YYSYMBOL_TKN_GOTO = 15,                  /* "goto"  */
  YYSYMBOL_TKN_AUTO = 16,                  /* "auto"  */
  YYSYMBOL_TKN_FUNC = 17,                  /* "func"  */
  YYSYMBOL_TKN_IF = 18,                    /* "if"  */
  YYSYMBOL_TKN_ELSE = 19,                  /* "else"  */
  YYSYMBOL_TKN_WHILE = 20,                 /* "while"  */
  YYSYMBOL_TKN_RETURN = 21,                /* "return"  */
  YYSYMBOL_TKN_ASM = 22,                   /* "asm"  */
  YYSYMBOL_TKN_THEN = 23,                  /* "then"  */
  YYSYMBOL_TKN_LPAR = 24,                  /* "("  */
  YYSYMBOL_TKN_RPAR = 25,                  /* ")"  */
  YYSYMBOL_TKN_LBRAC = 26,                 /* "{"  */
  YYSYMBOL_TKN_RBRAC = 27,                 /* "}"  */
  YYSYMBOL_TKN_LSBRAC = 28,                /* "["  */
  YYSYMBOL_TKN_RSBRAC = 29,                /* "]"  */
  YYSYMBOL_TKN_SEMI = 30,                  /* ";"  */
  YYSYMBOL_TKN_COLON = 31,                 /* ":"  */
  YYSYMBOL_TKN_COMMA = 32,                 /* ","  */
  YYSYMBOL_TKN_IVAL = 33,                  /* TKN_IVAL  */
  YYSYMBOL_TKN_STRVAL = 34,                /* TKN_STRVAL  */
  YYSYMBOL_TKN_IDENT = 35,                 /* TKN_IDENT  */
  YYSYMBOL_TKN_GARBAGE = 36,               /* TKN_GARBAGE  */
  YYSYMBOL_TKN_ASSIGN_ADD = 37,            /* "+="  */
  YYSYMBOL_TKN_ASSIGN_SUB = 38,            /* "-="  */
  YYSYMBOL_TKN_ASSIGN_SHL = 39,            /* "<<="  */
  YYSYMBOL_TKN_ASSIGN_SHR = 40,            /* ">>="  */
  YYSYMBOL_TKN_ASSIGN_MUL = 41,            /* "*="  */
  YYSYMBOL_TKN_ASSIGN_DIV = 42,            /* "/="  */
  YYSYMBOL_TKN_ASSIGN_REM = 43,            /* "%="  */
  YYSYMBOL_TKN_ASSIGN_AND = 44,            /* "&="  */
  YYSYMBOL_TKN_ASSIGN_OR = 45,             /* "|="  */
  YYSYMBOL_TKN_ASSIGN_XOR = 46,            /* "^="  */
  YYSYMBOL_TKN_INC = 47,                   /* "++"  */
  YYSYMBOL_TKN_DEC = 48,                   /* "--"  */
  YYSYMBOL_TKN_LOGIC_AND = 49,             /* "&&"  */
  YYSYMBOL_TKN_LOGIC_OR = 50,              /* "||"  */
  YYSYMBOL_TKN_ADD = 51,                   /* "+"  */
  YYSYMBOL_TKN_SUB = 52,                   /* "-"  */
  YYSYMBOL_TKN_ASSIGN = 53,                /* "="  */
  YYSYMBOL_TKN_AMP = 54,                   /* "&"  */
  YYSYMBOL_TKN_MUL = 55,                   /* "*"  */
  YYSYMBOL_TKN_DIV = 56,                   /* "/"  */
  YYSYMBOL_TKN_REM = 57,                   /* "%"  */
  YYSYMBOL_TKN_NOT = 58,                   /* "!"  */
  YYSYMBOL_TKN_INV = 59,                   /* "~"  */
  YYSYMBOL_TKN_XOR = 60,                   /* "^"  */
  YYSYMBOL_TKN_OR = 61,                    /* "|"  */
  YYSYMBOL_TKN_SHL = 62,                   /* "<<"  */
  YYSYMBOL_TKN_SHR = 63,                   /* ">>"  */
  YYSYMBOL_TKN_LT = 64,                    /* "<"  */
  YYSYMBOL_TKN_LE = 65,                    /* "<="  */
  YYSYMBOL_TKN_GT = 66,                    /* ">"  */
  YYSYMBOL_TKN_GE = 67,                    /* ">="  */
  YYSYMBOL_TKN_EQ = 68,                    /* "=="  */
  YYSYMBOL_TKN_NE = 69,                    /* "!="  */
  YYSYMBOL_YYACCEPT = 70,                  /* $accept  */
  YYSYMBOL_library = 71,                   /* library  */
  YYSYMBOL_global = 72,                    /* global  */
  YYSYMBOL_opt_int = 73,                   /* opt_int  */
  YYSYMBOL_opt_signed = 74,                /* opt_signed  */
  YYSYMBOL_simple_type = 75,               /* simple_type  */
  YYSYMBOL_funcdef = 76,                   /* funcdef  */
  YYSYMBOL_vardecls = 77,                  /* vardecls  */
  YYSYMBOL_opt_params = 78,                /* opt_params  */
  YYSYMBOL_params = 79,                    /* params  */
  YYSYMBOL_idents = 80,                    /* idents  */
  YYSYMBOL_stmts = 81,                     /* stmts  */
  YYSYMBOL_stmt = 82,                      /* stmt  */
  YYSYMBOL_opt_exprs = 83,                 /* opt_exprs  */
  YYSYMBOL_exprs = 84,                     /* exprs  */
  YYSYMBOL_expr = 85,                      /* expr  */
  YYSYMBOL_inline_asm = 86,                /* inline_asm  */
  YYSYMBOL_asm_qual = 87,                  /* asm_qual  */
  YYSYMBOL_asm_code = 88,                  /* asm_code  */
  YYSYMBOL_opt_asm_strs = 89,              /* opt_asm_strs  */
  YYSYMBOL_asm_strs = 90,                  /* asm_strs  */
  YYSYMBOL_opt_asm_regs = 91,              /* opt_asm_regs  */
  YYSYMBOL_asm_regs = 92,                  /* asm_regs  */
  YYSYMBOL_asm_reg = 93                    /* asm_reg  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




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

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
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
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
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

#if !defined yyoverflow

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
#endif /* !defined yyoverflow */

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

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   324


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

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
       0,   129,   129,   130,   132,   133,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
     149,   150,   151,   152,   153,   154,   156,   158,   160,   161,
     162,   163,   164,   165,   167,   168,   169,   170,   171,   173,
     174,   175,   176,   177,   178,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   191,   192,   193,   194,   195,
     197,   198,   199,   201,   202,   204,   205,   206,   207,   209,
     210,   212,   213,   214,   215,   216,   218,   219,   221,   222,
     223,   224,   225,   226,   227,   228,   229,   230,   231,   233,
     235,   236,   237,   238,   240,   244,   247,   249,   251,   252,
     253,   254,   255,   256,   257,   258,   259,   260
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "\"unsigned\"",
  "\"signed\"", "\"float\"", "\"double\"", "\"_Bool\"", "\"char\"",
  "\"short\"", "\"int\"", "\"long\"", "\"void\"", "\"volatile\"",
  "\"inline\"", "\"goto\"", "\"auto\"", "\"func\"", "\"if\"", "\"else\"",
  "\"while\"", "\"return\"", "\"asm\"", "\"then\"", "\"(\"", "\")\"",
  "\"{\"", "\"}\"", "\"[\"", "\"]\"", "\";\"", "\":\"", "\",\"",
  "TKN_IVAL", "TKN_STRVAL", "TKN_IDENT", "TKN_GARBAGE", "\"+=\"", "\"-=\"",
  "\"<<=\"", "\">>=\"", "\"*=\"", "\"/=\"", "\"%=\"", "\"&=\"", "\"|=\"",
  "\"^=\"", "\"++\"", "\"--\"", "\"&&\"", "\"||\"", "\"+\"", "\"-\"",
  "\"=\"", "\"&\"", "\"*\"", "\"/\"", "\"%\"", "\"!\"", "\"~\"", "\"^\"",
  "\"|\"", "\"<<\"", "\">>\"", "\"<\"", "\"<=\"", "\">\"", "\">=\"",
  "\"==\"", "\"!=\"", "$accept", "library", "global", "opt_int",
  "opt_signed", "simple_type", "funcdef", "vardecls", "opt_params",
  "params", "idents", "stmts", "stmt", "opt_exprs", "exprs", "expr",
  "inline_asm", "asm_qual", "asm_code", "opt_asm_strs", "asm_strs",
  "opt_asm_regs", "asm_regs", "asm_reg", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#ifdef YYPRINT
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
#endif

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
static const yytype_uint8 yydefgoto[] =
{
       0,     9,    10,    29,    11,    66,    13,    67,    41,    42,
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


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

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

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


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
# ifndef YY_LOCATION_PRINT
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, ctx); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, parser_ctx_t *ctx)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (ctx);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yykind < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yykind], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, parser_ctx_t *ctx)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep, ctx);
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
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule, parser_ctx_t *ctx)
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
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)], ctx);
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
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
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






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, parser_ctx_t *ctx)
{
  YY_USE (yyvaluep);
  YY_USE (ctx);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
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
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

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
  YY_STACK_PRINT (yyss, yyssp);

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
#  undef YYSTACK_RELOCATE
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

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (ctx);
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
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
  case 4: /* global: funcdef  */
#line 132 "src/parser.bison"
                                {function_added(ctx, &(yyvsp[0].func));}
#line 1446 "src/parser.c"
    break;

  case 10: /* simple_type: "char"  */
#line 139 "src/parser.bison"
                                                                                                {(yyval.simple_type)=CTYPE_CHAR;}
#line 1452 "src/parser.c"
    break;

  case 11: /* simple_type: "signed" "char"  */
#line 140 "src/parser.bison"
                                                                                                        {(yyval.simple_type)=CTYPE_S_CHAR;}
#line 1458 "src/parser.c"
    break;

  case 12: /* simple_type: "unsigned" "char"  */
#line 141 "src/parser.bison"
                                                                                                        {(yyval.simple_type)=CTYPE_U_CHAR;}
#line 1464 "src/parser.c"
    break;

  case 13: /* simple_type: opt_signed "short" opt_int  */
#line 142 "src/parser.bison"
                                                                                                {(yyval.simple_type)=CTYPE_S_SHORT;}
#line 1470 "src/parser.c"
    break;

  case 14: /* simple_type: opt_signed "int"  */
#line 143 "src/parser.bison"
                                                                                                        {(yyval.simple_type)=CTYPE_S_INT;}
#line 1476 "src/parser.c"
    break;

  case 15: /* simple_type: opt_signed "long" opt_int  */
#line 144 "src/parser.bison"
                                                                                        {(yyval.simple_type)=CTYPE_S_LONG;}
#line 1482 "src/parser.c"
    break;

  case 16: /* simple_type: opt_signed "long" "long" opt_int  */
#line 145 "src/parser.bison"
                                                                                        {(yyval.simple_type)=CTYPE_S_LONGER;}
#line 1488 "src/parser.c"
    break;

  case 17: /* simple_type: "unsigned" "short" opt_int  */
#line 146 "src/parser.bison"
                                                                                                {(yyval.simple_type)=CTYPE_U_SHORT;}
#line 1494 "src/parser.c"
    break;

  case 18: /* simple_type: "unsigned" "int"  */
#line 147 "src/parser.bison"
                                                                                                        {(yyval.simple_type)=CTYPE_U_INT;}
#line 1500 "src/parser.c"
    break;

  case 19: /* simple_type: "unsigned" "long" opt_int  */
#line 148 "src/parser.bison"
                                                                                                {(yyval.simple_type)=CTYPE_U_LONG;}
#line 1506 "src/parser.c"
    break;

  case 20: /* simple_type: "unsigned" "long" "long" opt_int  */
#line 149 "src/parser.bison"
                                                                                        {(yyval.simple_type)=CTYPE_U_LONGER;}
#line 1512 "src/parser.c"
    break;

  case 21: /* simple_type: "float"  */
#line 150 "src/parser.bison"
                                                                                                                {(yyval.simple_type)=CTYPE_FLOAT;}
#line 1518 "src/parser.c"
    break;

  case 22: /* simple_type: "double"  */
#line 151 "src/parser.bison"
                                                                                                                {(yyval.simple_type)=CTYPE_DOUBLE;}
#line 1524 "src/parser.c"
    break;

  case 23: /* simple_type: "long" "double"  */
#line 152 "src/parser.bison"
                                                                                        {(yyval.simple_type)=CTYPE_LONG_DOUBLE;}
#line 1530 "src/parser.c"
    break;

  case 24: /* simple_type: "_Bool"  */
#line 153 "src/parser.bison"
                                                                                                                {(yyval.simple_type)=CTYPE_BOOL;}
#line 1536 "src/parser.c"
    break;

  case 25: /* simple_type: "void"  */
#line 154 "src/parser.bison"
                                                                                                                {(yyval.simple_type)=CTYPE_VOID;}
#line 1542 "src/parser.c"
    break;

  case 26: /* funcdef: simple_type TKN_IDENT "(" opt_params ")" "{" stmts "}"  */
#line 157 "src/parser.bison"
                                                                                                        {(yyval.func)=funcdef_decl(&(yyvsp[-6].strval), &(yyvsp[-4].idents), &(yyvsp[-1].stmts));}
#line 1548 "src/parser.c"
    break;

  case 27: /* vardecls: simple_type idents ";"  */
#line 158 "src/parser.bison"
                                                                                        {(yyval.idents)=(yyvsp[-1].idents); idents_settype(&(yyval.idents), &(yyvsp[-2].simple_type));}
#line 1554 "src/parser.c"
    break;

  case 28: /* opt_params: params  */
#line 160 "src/parser.bison"
                                                                                                        {(yyval.idents)=(yyvsp[0].idents);}
#line 1560 "src/parser.c"
    break;

  case 29: /* opt_params: %empty  */
#line 161 "src/parser.bison"
                                                                                                                {(yyval.idents)=idents_empty();}
#line 1566 "src/parser.c"
    break;

  case 30: /* params: params "," simple_type TKN_IDENT  */
#line 162 "src/parser.bison"
                                                                                {(yyval.idents)=idents_cat  (&(yyvsp[-3].idents),  &(yyvsp[-1].simple_type), &(yyvsp[0].strval));}
#line 1572 "src/parser.c"
    break;

  case 31: /* params: simple_type TKN_IDENT  */
#line 163 "src/parser.bison"
                                                                                                {(yyval.idents)=idents_one  (&(yyvsp[-1].simple_type), &(yyvsp[0].strval));}
#line 1578 "src/parser.c"
    break;

  case 32: /* idents: idents "," TKN_IDENT  */
#line 164 "src/parser.bison"
                                                                                        {(yyval.idents)=idents_cat  (&(yyvsp[-2].idents),  NULL, &(yyvsp[0].strval));}
#line 1584 "src/parser.c"
    break;

  case 33: /* idents: TKN_IDENT  */
#line 165 "src/parser.bison"
                                                                                                                {(yyval.idents)=idents_one  (NULL, &(yyvsp[0].strval));}
#line 1590 "src/parser.c"
    break;

  case 34: /* stmts: stmts stmt  */
#line 167 "src/parser.bison"
                                                                                                        {(yyval.stmts)=stmts_cat   (&(yyvsp[-1].stmts), &(yyvsp[0].stmt));}
#line 1596 "src/parser.c"
    break;

  case 35: /* stmts: %empty  */
#line 168 "src/parser.bison"
                                                                                                                {(yyval.stmts)=stmts_empty ();}
#line 1602 "src/parser.c"
    break;

  case 36: /* stmt: "{" stmts "}"  */
#line 169 "src/parser.bison"
                                                                                                {(yyval.stmt)=stmt_multi  (&(yyvsp[-1].stmts));}
#line 1608 "src/parser.c"
    break;

  case 37: /* stmt: "if" "(" expr ")" stmt  */
#line 170 "src/parser.bison"
                                                                                {(yyval.stmt)=stmt_if     (&(yyvsp[-2].expr), &(yyvsp[0].stmt), NULL);}
#line 1614 "src/parser.c"
    break;

  case 38: /* stmt: "if" "(" expr ")" stmt "else" stmt  */
#line 172 "src/parser.bison"
                                                                                                                {(yyval.stmt)=stmt_if     (&(yyvsp[-4].expr), &(yyvsp[-2].stmt), &(yyvsp[0].stmt));}
#line 1620 "src/parser.c"
    break;

  case 39: /* stmt: "while" "(" expr ")" stmt  */
#line 173 "src/parser.bison"
                                                                                                {(yyval.stmt)=stmt_while  (&(yyvsp[-2].expr), &(yyvsp[0].stmt));}
#line 1626 "src/parser.c"
    break;

  case 40: /* stmt: "return" ";"  */
#line 174 "src/parser.bison"
                                                                                                        {(yyval.stmt)=stmt_ret    (NULL);}
#line 1632 "src/parser.c"
    break;

  case 41: /* stmt: "return" expr ";"  */
#line 175 "src/parser.bison"
                                                                                                        {(yyval.stmt)=stmt_ret    (&(yyvsp[-1].expr));}
#line 1638 "src/parser.c"
    break;

  case 42: /* stmt: vardecls  */
#line 176 "src/parser.bison"
                                                                                                                {(yyval.stmt)=stmt_var    (&(yyvsp[0].idents));}
#line 1644 "src/parser.c"
    break;

  case 43: /* stmt: expr ";"  */
#line 177 "src/parser.bison"
                                                                                                                {(yyval.stmt)=stmt_expr   (&(yyvsp[-1].expr));}
#line 1650 "src/parser.c"
    break;

  case 44: /* stmt: inline_asm ";"  */
#line 178 "src/parser.bison"
                                                                                                        {(yyval.stmt)=(yyvsp[-1].stmt);}
#line 1656 "src/parser.c"
    break;

  case 49: /* expr: TKN_IVAL  */
#line 184 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_icnst(&(yyvsp[0].ival));}
#line 1662 "src/parser.c"
    break;

  case 50: /* expr: TKN_STRVAL  */
#line 185 "src/parser.bison"
                                                                                                                {(yyval.expr)=expr_scnst(&(yyvsp[0].strval));}
#line 1668 "src/parser.c"
    break;

  case 51: /* expr: TKN_IDENT  */
#line 186 "src/parser.bison"
                                                                                                                {(yyval.expr)=expr_ident(&(yyvsp[0].strval));}
#line 1674 "src/parser.c"
    break;

  case 52: /* expr: expr "(" opt_exprs ")"  */
#line 187 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_call (&(yyvsp[-3].expr), &(yyvsp[-1].exprs));}
#line 1680 "src/parser.c"
    break;

  case 53: /* expr: expr "[" expr "]"  */
#line 188 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_INDEX,     &(yyvsp[-3].expr), &(yyvsp[-1].expr));}
#line 1686 "src/parser.c"
    break;

  case 54: /* expr: "(" expr ")"  */
#line 189 "src/parser.bison"
                                                                                                        {(yyval.expr)=(yyvsp[-1].expr);}
#line 1692 "src/parser.c"
    break;

  case 55: /* expr: "-" expr  */
#line 191 "src/parser.bison"
                                                                                                                {(yyval.expr)=expr_math1(OP_0_MINUS,   &(yyvsp[0].expr));}
#line 1698 "src/parser.c"
    break;

  case 56: /* expr: "!" expr  */
#line 192 "src/parser.bison"
                                                                                                                {(yyval.expr)=expr_math1(OP_LOGIC_NOT, &(yyvsp[0].expr));}
#line 1704 "src/parser.c"
    break;

  case 57: /* expr: "~" expr  */
#line 193 "src/parser.bison"
                                                                                                                {(yyval.expr)=expr_math1(OP_BIT_NOT,   &(yyvsp[0].expr));}
#line 1710 "src/parser.c"
    break;

  case 58: /* expr: "&" expr  */
#line 194 "src/parser.bison"
                                                                                                                {(yyval.expr)=expr_math1(OP_ADROF,     &(yyvsp[0].expr));}
#line 1716 "src/parser.c"
    break;

  case 59: /* expr: "*" expr  */
#line 195 "src/parser.bison"
                                                                                                                {(yyval.expr)=expr_math1(OP_DEREF,     &(yyvsp[0].expr));}
#line 1722 "src/parser.c"
    break;

  case 60: /* expr: expr "*" expr  */
#line 197 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_MUL,       &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1728 "src/parser.c"
    break;

  case 61: /* expr: expr "/" expr  */
#line 198 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_math2(OP_DIV,       &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1734 "src/parser.c"
    break;

  case 62: /* expr: expr "%" expr  */
#line 199 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_math2(OP_MOD,       &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1740 "src/parser.c"
    break;

  case 63: /* expr: expr "+" expr  */
#line 201 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_ADD,       &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1746 "src/parser.c"
    break;

  case 64: /* expr: expr "-" expr  */
#line 202 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_math2(OP_SUB,       &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1752 "src/parser.c"
    break;

  case 65: /* expr: expr "<" expr  */
#line 204 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_LT,        &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1758 "src/parser.c"
    break;

  case 66: /* expr: expr "<=" expr  */
#line 205 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_math2(OP_LE,        &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1764 "src/parser.c"
    break;

  case 67: /* expr: expr ">" expr  */
#line 206 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_math2(OP_GT,        &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1770 "src/parser.c"
    break;

  case 68: /* expr: expr ">=" expr  */
#line 207 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_math2(OP_GE,        &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1776 "src/parser.c"
    break;

  case 69: /* expr: expr "==" expr  */
#line 209 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_NE,        &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1782 "src/parser.c"
    break;

  case 70: /* expr: expr "!=" expr  */
#line 210 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_math2(OP_EQ,        &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1788 "src/parser.c"
    break;

  case 71: /* expr: expr "&" expr  */
#line 212 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_BIT_AND,   &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1794 "src/parser.c"
    break;

  case 72: /* expr: expr "^" expr  */
#line 213 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_BIT_XOR,   &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1800 "src/parser.c"
    break;

  case 73: /* expr: expr "|" expr  */
#line 214 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_BIT_OR,    &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1806 "src/parser.c"
    break;

  case 74: /* expr: expr "<<" expr  */
#line 215 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_SHIFT_L,   &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1812 "src/parser.c"
    break;

  case 75: /* expr: expr ">>" expr  */
#line 216 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_math2(OP_SHIFT_R,   &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1818 "src/parser.c"
    break;

  case 76: /* expr: expr "&&" expr  */
#line 218 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_LOGIC_AND, &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1824 "src/parser.c"
    break;

  case 77: /* expr: expr "||" expr  */
#line 219 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_LOGIC_OR,  &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1830 "src/parser.c"
    break;

  case 78: /* expr: expr "=" expr  */
#line 221 "src/parser.bison"
                                                                                                        {(yyval.expr)=expr_math2(OP_ASSIGN,    &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1836 "src/parser.c"
    break;

  case 79: /* expr: expr "+=" expr  */
#line 222 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_matha(OP_ADD,       &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1842 "src/parser.c"
    break;

  case 80: /* expr: expr "-=" expr  */
#line 223 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_matha(OP_SUB,       &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1848 "src/parser.c"
    break;

  case 81: /* expr: expr "*=" expr  */
#line 224 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_matha(OP_MUL,       &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1854 "src/parser.c"
    break;

  case 82: /* expr: expr "/=" expr  */
#line 225 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_matha(OP_DIV,       &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1860 "src/parser.c"
    break;

  case 83: /* expr: expr "%=" expr  */
#line 226 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_matha(OP_MOD,       &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1866 "src/parser.c"
    break;

  case 84: /* expr: expr "&=" expr  */
#line 227 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_matha(OP_BIT_AND,   &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1872 "src/parser.c"
    break;

  case 85: /* expr: expr "|=" expr  */
#line 228 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_matha(OP_BIT_OR,    &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1878 "src/parser.c"
    break;

  case 86: /* expr: expr "^=" expr  */
#line 229 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_matha(OP_BIT_XOR,   &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1884 "src/parser.c"
    break;

  case 87: /* expr: expr "<<=" expr  */
#line 230 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_matha(OP_SHIFT_L,   &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1890 "src/parser.c"
    break;

  case 88: /* expr: expr ">>=" expr  */
#line 231 "src/parser.bison"
                                                                                                {(yyval.expr)=expr_matha(OP_SHIFT_R,   &(yyvsp[-2].expr), &(yyvsp[0].expr));}
#line 1896 "src/parser.c"
    break;

  case 89: /* inline_asm: "asm" asm_qual asm_code  */
#line 233 "src/parser.bison"
                                                                                        {(yyval.stmt)=(yyvsp[0].stmt); (yyval.stmt).iasm->qualifiers=(yyvsp[-1].asm_qual); (yyval.stmt).iasm->qualifiers.is_volatile |= !(yyval.stmt).iasm->outputs; }
#line 1902 "src/parser.c"
    break;

  case 90: /* asm_qual: %empty  */
#line 235 "src/parser.bison"
                                                                                                        {(yyval.asm_qual)=(iasm_qual_t) {.is_volatile=0, .is_inline=0, .is_goto=0};}
#line 1908 "src/parser.c"
    break;

  case 91: /* asm_qual: asm_qual "volatile"  */
#line 236 "src/parser.bison"
                                                                                                        {(yyval.asm_qual)=(yyvsp[-1].asm_qual); (yyval.asm_qual).is_volatile=1;}
#line 1914 "src/parser.c"
    break;

  case 92: /* asm_qual: asm_qual "inline"  */
#line 237 "src/parser.bison"
                                                                                                        {(yyval.asm_qual)=(yyvsp[-1].asm_qual); (yyval.asm_qual).is_inline=1;}
#line 1920 "src/parser.c"
    break;

  case 93: /* asm_qual: asm_qual "goto"  */
#line 238 "src/parser.bison"
                                                                                                        {(yyval.asm_qual)=(yyvsp[-1].asm_qual); (yyval.asm_qual).is_goto=1;}
#line 1926 "src/parser.c"
    break;

  case 94: /* asm_code: "(" TKN_STRVAL ":" opt_asm_regs ":" opt_asm_regs ":" opt_asm_strs ")"  */
#line 243 "src/parser.bison"
                                                                                                {(yyval.stmt)=stmt_iasm(&(yyvsp[-7].strval), &(yyvsp[-5].asm_regs),  &(yyvsp[-3].asm_regs),  NULL);}
#line 1932 "src/parser.c"
    break;

  case 95: /* asm_code: "(" TKN_STRVAL ":" opt_asm_regs ":" opt_asm_regs ")"  */
#line 246 "src/parser.bison"
                                                                                                {(yyval.stmt)=stmt_iasm(&(yyvsp[-5].strval), &(yyvsp[-3].asm_regs),  &(yyvsp[-1].asm_regs),  NULL);}
#line 1938 "src/parser.c"
    break;

  case 96: /* asm_code: "(" TKN_STRVAL ":" opt_asm_regs ")"  */
#line 248 "src/parser.bison"
                                                                                                {(yyval.stmt)=stmt_iasm(&(yyvsp[-3].strval), &(yyvsp[-1].asm_regs),  NULL, NULL);}
#line 1944 "src/parser.c"
    break;

  case 97: /* asm_code: "(" TKN_STRVAL ")"  */
#line 249 "src/parser.bison"
                                                                                                        {(yyval.stmt)=stmt_iasm(&(yyvsp[-1].strval), NULL, NULL, NULL);}
#line 1950 "src/parser.c"
    break;

  case 102: /* opt_asm_regs: asm_regs  */
#line 255 "src/parser.bison"
                                                                                                {(yyval.asm_regs)=(yyvsp[0].asm_regs);}
#line 1956 "src/parser.c"
    break;

  case 103: /* opt_asm_regs: %empty  */
#line 256 "src/parser.bison"
                                                                                                                {(yyval.asm_regs)=iasm_regs_empty();}
#line 1962 "src/parser.c"
    break;

  case 104: /* asm_regs: asm_reg "," asm_regs  */
#line 257 "src/parser.bison"
                                                                                        {(yyval.asm_regs)=iasm_regs_cat(&(yyvsp[0].asm_regs), &(yyvsp[-2].asm_reg));}
#line 1968 "src/parser.c"
    break;

  case 105: /* asm_regs: asm_reg  */
#line 258 "src/parser.bison"
                                                                                                                {(yyval.asm_regs)=iasm_regs_one(&(yyvsp[0].asm_reg));}
#line 1974 "src/parser.c"
    break;

  case 106: /* asm_reg: "[" TKN_IDENT "]" TKN_STRVAL "(" expr ")"  */
#line 259 "src/parser.bison"
                                                                        {(yyval.asm_reg)=stmt_iasm_reg(&(yyvsp[-5].strval),  &(yyvsp[-3].strval), &(yyvsp[-1].expr));}
#line 1980 "src/parser.c"
    break;

  case 107: /* asm_reg: TKN_STRVAL "(" expr ")"  */
#line 260 "src/parser.bison"
                                                                                                {(yyval.asm_reg)=stmt_iasm_reg(NULL, &(yyvsp[-3].strval), &(yyvsp[-1].expr));}
#line 1986 "src/parser.c"
    break;


#line 1990 "src/parser.c"

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
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

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
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (ctx, YY_("syntax error"));
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

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, ctx);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

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


#if !defined yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (ctx, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturn;
#endif


/*-------------------------------------------------------.
| yyreturn -- parsing is finished, clean up and return.  |
`-------------------------------------------------------*/
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
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, ctx);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 261 "src/parser.bison"


void *make_copy(void *mem, size_t size) {
	if (!mem) return NULL;
	void *copy = malloc(size);
	memcpy(copy, mem, size);
	return copy;
}
