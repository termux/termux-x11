/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

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

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         _xkbcommon_parse
#define yylex           _xkbcommon_lex
#define yyerror         _xkbcommon_error
#define yydebug         _xkbcommon_debug
#define yynerrs         _xkbcommon_nerrs


/* Copy the first part of user declarations.  */
#line 33 "src/xkbcomp/parser.y" /* yacc.c:339  */

#include "xkbcomp-priv.h"
#include "ast-build.h"
#include "parser-priv.h"
#include "scanner-utils.h"

struct parser_param {
    struct xkb_context *ctx;
    struct scanner *scanner;
    XkbFile *rtrn;
    bool more_maps;
};

#define parser_err(param, fmt, ...) \
    scanner_err((param)->scanner, fmt, ##__VA_ARGS__)

#define parser_warn(param, fmt, ...) \
    scanner_warn((param)->scanner, fmt, ##__VA_ARGS__)

static void
_xkbcommon_error(struct parser_param *param, const char *msg)
{
    parser_err(param, "%s", msg);
}

static bool
resolve_keysym(const char *name, xkb_keysym_t *sym_rtrn)
{
    xkb_keysym_t sym;

    if (!name || istreq(name, "any") || istreq(name, "nosymbol")) {
        *sym_rtrn = XKB_KEY_NoSymbol;
        return true;
    }

    if (istreq(name, "none") || istreq(name, "voidsymbol")) {
        *sym_rtrn = XKB_KEY_VoidSymbol;
        return true;
    }

    sym = xkb_keysym_from_name(name, XKB_KEYSYM_NO_FLAGS);
    if (sym != XKB_KEY_NoSymbol) {
        *sym_rtrn = sym;
        return true;
    }

    return false;
}

#define param_scanner param->scanner

#line 124 "src/xkbcomp/parser.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
#ifndef YY__XKBCOMMON_SRC_XKBCOMP_PARSER_H_INCLUDED
# define YY__XKBCOMMON_SRC_XKBCOMP_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int _xkbcommon_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    END_OF_FILE = 0,
    ERROR_TOK = 255,
    XKB_KEYMAP = 1,
    XKB_KEYCODES = 2,
    XKB_TYPES = 3,
    XKB_SYMBOLS = 4,
    XKB_COMPATMAP = 5,
    XKB_GEOMETRY = 6,
    XKB_SEMANTICS = 7,
    XKB_LAYOUT = 8,
    INCLUDE = 10,
    OVERRIDE = 11,
    AUGMENT = 12,
    REPLACE = 13,
    ALTERNATE = 14,
    VIRTUAL_MODS = 20,
    TYPE = 21,
    INTERPRET = 22,
    ACTION_TOK = 23,
    KEY = 24,
    ALIAS = 25,
    GROUP = 26,
    MODIFIER_MAP = 27,
    INDICATOR = 28,
    SHAPE = 29,
    KEYS = 30,
    ROW = 31,
    SECTION = 32,
    OVERLAY = 33,
    TEXT = 34,
    OUTLINE = 35,
    SOLID = 36,
    LOGO = 37,
    VIRTUAL = 38,
    EQUALS = 40,
    PLUS = 41,
    MINUS = 42,
    DIVIDE = 43,
    TIMES = 44,
    OBRACE = 45,
    CBRACE = 46,
    OPAREN = 47,
    CPAREN = 48,
    OBRACKET = 49,
    CBRACKET = 50,
    DOT = 51,
    COMMA = 52,
    SEMI = 53,
    EXCLAM = 54,
    INVERT = 55,
    STRING = 60,
    INTEGER = 61,
    FLOAT = 62,
    IDENT = 63,
    KEYNAME = 64,
    PARTIAL = 70,
    DEFAULT = 71,
    HIDDEN = 72,
    ALPHANUMERIC_KEYS = 73,
    MODIFIER_KEYS = 74,
    KEYPAD_KEYS = 75,
    FUNCTION_KEYS = 76,
    ALTERNATE_GROUP = 77
  };
#endif
/* Tokens.  */
#define END_OF_FILE 0
#define ERROR_TOK 255
#define XKB_KEYMAP 1
#define XKB_KEYCODES 2
#define XKB_TYPES 3
#define XKB_SYMBOLS 4
#define XKB_COMPATMAP 5
#define XKB_GEOMETRY 6
#define XKB_SEMANTICS 7
#define XKB_LAYOUT 8
#define INCLUDE 10
#define OVERRIDE 11
#define AUGMENT 12
#define REPLACE 13
#define ALTERNATE 14
#define VIRTUAL_MODS 20
#define TYPE 21
#define INTERPRET 22
#define ACTION_TOK 23
#define KEY 24
#define ALIAS 25
#define GROUP 26
#define MODIFIER_MAP 27
#define INDICATOR 28
#define SHAPE 29
#define KEYS 30
#define ROW 31
#define SECTION 32
#define OVERLAY 33
#define TEXT 34
#define OUTLINE 35
#define SOLID 36
#define LOGO 37
#define VIRTUAL 38
#define EQUALS 40
#define PLUS 41
#define MINUS 42
#define DIVIDE 43
#define TIMES 44
#define OBRACE 45
#define CBRACE 46
#define OPAREN 47
#define CPAREN 48
#define OBRACKET 49
#define CBRACKET 50
#define DOT 51
#define COMMA 52
#define SEMI 53
#define EXCLAM 54
#define INVERT 55
#define STRING 60
#define INTEGER 61
#define FLOAT 62
#define IDENT 63
#define KEYNAME 64
#define PARTIAL 70
#define DEFAULT 71
#define HIDDEN 72
#define ALPHANUMERIC_KEYS 73
#define MODIFIER_KEYS 74
#define KEYPAD_KEYS 75
#define FUNCTION_KEYS 76
#define ALTERNATE_GROUP 77

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 162 "src/xkbcomp/parser.y" /* yacc.c:355  */

        int              ival;
        int64_t          num;
        enum xkb_file_type file_type;
        char            *str;
        xkb_atom_t      atom;
        enum merge_mode merge;
        enum xkb_map_flags mapFlags;
        xkb_keysym_t    keysym;
        ParseCommon     *any;
        ExprDef         *expr;
        VarDef          *var;
        VModDef         *vmod;
        InterpDef       *interp;
        KeyTypeDef      *keyType;
        SymbolsDef      *syms;
        ModMapDef       *modMask;
        GroupCompatDef  *groupCompat;
        LedMapDef       *ledMap;
        LedNameDef      *ledName;
        KeycodeDef      *keyCode;
        KeyAliasDef     *keyAlias;
        void            *geom;
        XkbFile         *file;

#line 320 "src/xkbcomp/parser.c" /* yacc.c:355  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int _xkbcommon_parse (struct parser_param *param);

#endif /* !YY__XKBCOMMON_SRC_XKBCOMP_PARSER_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 334 "src/xkbcomp/parser.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

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

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
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
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
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
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  16
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   735

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  65
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  72
/* YYNRULES -- Number of rules.  */
#define YYNRULES  184
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  334

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   257

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     4,     5,     6,     7,     8,     9,    10,    11,     2,
      12,    13,    14,    15,    16,     2,     2,     2,     2,     2,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,     2,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,     2,     2,     2,     2,
      52,    53,    54,    55,    56,     2,     2,     2,     2,     2,
      57,    58,    59,    60,    61,    62,    63,    64,     2,     2,
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
       2,     2,     2,     2,     2,     3,     1,     2
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   241,   241,   243,   245,   249,   255,   256,   257,   260,
     268,   272,   287,   288,   289,   290,   291,   294,   295,   298,
     299,   302,   303,   304,   305,   306,   307,   308,   309,   312,
     314,   317,   322,   327,   332,   337,   342,   347,   352,   357,
     362,   367,   372,   373,   374,   375,   382,   384,   386,   390,
     394,   398,   402,   405,   409,   411,   415,   421,   423,   427,
     430,   434,   440,   446,   449,   451,   454,   455,   456,   457,
     458,   461,   463,   467,   471,   475,   479,   481,   485,   487,
     491,   495,   496,   499,   501,   503,   505,   507,   511,   512,
     515,   516,   520,   521,   524,   526,   530,   534,   535,   538,
     541,   543,   547,   549,   551,   555,   557,   561,   565,   569,
     570,   571,   572,   575,   576,   579,   581,   583,   585,   587,
     589,   591,   593,   595,   597,   599,   603,   604,   607,   608,
     609,   610,   611,   621,   622,   625,   628,   632,   634,   636,
     638,   640,   642,   646,   648,   650,   652,   654,   656,   658,
     660,   664,   667,   671,   675,   677,   679,   681,   685,   687,
     689,   691,   695,   696,   699,   701,   703,   705,   709,   713,
     719,   720,   740,   741,   744,   745,   748,   751,   754,   757,
     758,   761,   764,   765,   768
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "END_OF_FILE", "error", "$undefined", "ERROR_TOK", "XKB_KEYMAP",
  "XKB_KEYCODES", "XKB_TYPES", "XKB_SYMBOLS", "XKB_COMPATMAP",
  "XKB_GEOMETRY", "XKB_SEMANTICS", "XKB_LAYOUT", "INCLUDE", "OVERRIDE",
  "AUGMENT", "REPLACE", "ALTERNATE", "VIRTUAL_MODS", "TYPE", "INTERPRET",
  "ACTION_TOK", "KEY", "ALIAS", "GROUP", "MODIFIER_MAP", "INDICATOR",
  "SHAPE", "KEYS", "ROW", "SECTION", "OVERLAY", "TEXT", "OUTLINE", "SOLID",
  "LOGO", "VIRTUAL", "EQUALS", "PLUS", "MINUS", "DIVIDE", "TIMES",
  "OBRACE", "CBRACE", "OPAREN", "CPAREN", "OBRACKET", "CBRACKET", "DOT",
  "COMMA", "SEMI", "EXCLAM", "INVERT", "STRING", "INTEGER", "FLOAT",
  "IDENT", "KEYNAME", "PARTIAL", "DEFAULT", "HIDDEN", "ALPHANUMERIC_KEYS",
  "MODIFIER_KEYS", "KEYPAD_KEYS", "FUNCTION_KEYS", "ALTERNATE_GROUP",
  "$accept", "XkbFile", "XkbCompositeMap", "XkbCompositeType",
  "XkbMapConfigList", "XkbMapConfig", "FileType", "OptFlags", "Flags",
  "Flag", "DeclList", "Decl", "VarDecl", "KeyNameDecl", "KeyAliasDecl",
  "VModDecl", "VModDefList", "VModDef", "InterpretDecl", "InterpretMatch",
  "VarDeclList", "KeyTypeDecl", "SymbolsDecl", "SymbolsBody",
  "SymbolsVarDecl", "ArrayInit", "GroupCompatDecl", "ModMapDecl",
  "LedMapDecl", "LedNameDecl", "ShapeDecl", "SectionDecl", "SectionBody",
  "SectionBodyItem", "RowBody", "RowBodyItem", "Keys", "Key",
  "OverlayDecl", "OverlayKeyList", "OverlayKey", "OutlineList",
  "OutlineInList", "CoordList", "Coord", "DoodadDecl", "DoodadType",
  "FieldSpec", "Element", "OptMergeMode", "MergeMode", "OptExprList",
  "ExprList", "Expr", "Term", "ActionList", "Action", "Lhs", "Terminal",
  "OptKeySymList", "KeySymList", "KeySyms", "KeySym", "SignedNumber",
  "Number", "Float", "Integer", "KeyCode", "Ident", "String", "OptMapName",
  "MapName", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   255,     1,     2,     3,     4,     5,     6,
       7,     8,    10,    11,    12,    13,    14,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    60,    61,    62,    63,    64,    70,    71,    72,
      73,    74,    75,    76,    77
};
# endif

#define YYPACT_NINF -182

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-182)))

#define YYTABLE_NINF -180

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     176,  -182,  -182,  -182,  -182,  -182,  -182,  -182,  -182,  -182,
       6,  -182,  -182,   271,   227,  -182,  -182,  -182,  -182,  -182,
    -182,  -182,  -182,  -182,  -182,   -38,   -38,  -182,  -182,   -24,
    -182,    33,   227,  -182,   210,  -182,   353,    44,     5,  -182,
    -182,  -182,  -182,  -182,  -182,    32,  -182,    13,    41,  -182,
    -182,   -48,    55,    11,  -182,    79,    87,    58,   -48,    -2,
      55,  -182,    55,    72,  -182,  -182,  -182,   107,   -48,  -182,
     110,  -182,  -182,  -182,  -182,  -182,  -182,  -182,  -182,  -182,
    -182,  -182,  -182,  -182,  -182,  -182,    55,   -18,  -182,   127,
     121,  -182,    66,  -182,   138,  -182,   136,  -182,  -182,  -182,
     144,   147,  -182,   152,   180,   182,   178,   184,   187,   188,
     190,    58,   198,   201,   214,   367,   677,   367,  -182,   -48,
    -182,   367,   663,   663,   367,   494,   200,   367,   367,   367,
     663,    68,   449,   223,  -182,  -182,   212,   663,  -182,  -182,
    -182,  -182,  -182,  -182,  -182,  -182,  -182,   367,   367,   367,
     367,   367,  -182,  -182,    57,   157,  -182,   224,  -182,  -182,
    -182,  -182,  -182,   218,    91,  -182,   333,  -182,   509,   537,
     333,   552,   -48,     1,  -182,  -182,   228,    40,   216,   143,
      70,   333,   150,   593,   247,   -30,    97,  -182,   105,  -182,
     261,    55,   259,    55,  -182,  -182,   408,  -182,  -182,  -182,
     367,  -182,   608,  -182,  -182,  -182,   287,  -182,  -182,   367,
     367,   367,   367,   367,  -182,   367,   367,  -182,   252,  -182,
     253,   264,    24,   269,   272,   163,  -182,   273,   270,  -182,
    -182,  -182,   280,   494,   285,  -182,  -182,   283,   367,  -182,
     284,   112,     8,  -182,  -182,   294,  -182,   299,   -36,   304,
     247,   326,   649,   279,   307,  -182,   204,   316,  -182,   322,
     320,   111,   111,  -182,  -182,   333,   211,  -182,  -182,   116,
     367,  -182,   677,  -182,    24,  -182,  -182,  -182,   333,  -182,
     333,  -182,  -182,  -182,   -30,  -182,  -182,  -182,  -182,   247,
     333,   334,  -182,   466,  -182,   318,  -182,  -182,  -182,  -182,
    -182,  -182,   339,  -182,  -182,  -182,   343,   120,    14,   345,
    -182,   361,   124,  -182,  -182,  -182,  -182,   367,  -182,   131,
    -182,  -182,   344,   350,   318,   166,   352,    14,  -182,  -182,
    -182,  -182,  -182,  -182
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
      18,     4,    21,    22,    23,    24,    25,    26,    27,    28,
       0,     2,     3,     0,    17,    20,     1,     6,    12,    13,
      15,    14,    16,     7,     8,   183,   183,    19,   184,     0,
     182,     0,    18,    30,    18,    10,     0,   127,     0,     9,
     128,   130,   129,   131,   132,     0,    29,     0,   126,     5,
      11,     0,   117,   116,   115,   118,     0,   119,   120,   121,
     122,   123,   124,   125,   110,   111,   112,     0,     0,   179,
       0,   180,    31,    34,    35,    32,    33,    36,    37,    39,
      38,    40,    41,    42,    43,    44,     0,   154,   114,     0,
     113,    45,     0,    53,    54,   181,     0,   170,   177,   169,
       0,    58,   171,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    47,     0,
      51,     0,     0,     0,     0,    65,     0,     0,     0,     0,
       0,     0,     0,     0,    48,   178,     0,     0,   117,   116,
     118,   119,   120,   121,   122,   124,   125,     0,     0,     0,
       0,     0,   176,   161,   154,     0,   142,   147,   149,   160,
     159,   113,   158,   155,     0,    52,    55,    60,     0,     0,
      57,   163,     0,     0,    64,    70,     0,   113,     0,     0,
       0,   136,     0,     0,     0,     0,     0,   101,     0,   106,
       0,   121,   123,     0,    84,    86,     0,    82,    87,    85,
       0,    49,     0,   144,   147,   143,     0,   145,   146,   134,
       0,     0,     0,     0,   156,     0,     0,    46,     0,    59,
       0,   170,     0,   169,     0,     0,   152,     0,   162,   167,
     166,    69,     0,     0,     0,    50,    73,     0,     0,    76,
       0,     0,     0,   175,   174,     0,   173,     0,     0,     0,
       0,     0,     0,     0,     0,    81,     0,     0,   150,     0,
     133,   138,   139,   137,   140,   141,     0,    61,    56,     0,
     134,    72,     0,    71,     0,    62,    63,    67,    66,    74,
     135,    75,   102,   172,     0,    78,   100,    79,   105,     0,
     104,     0,    91,     0,    89,     0,    80,    77,   108,   148,
     157,   168,     0,   151,   165,   164,     0,     0,     0,     0,
      88,     0,     0,    98,   153,   107,   103,     0,    94,     0,
      93,    83,     0,     0,     0,     0,     0,     0,    99,    96,
      97,    95,    90,    92
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -182,  -182,  -182,  -182,  -182,   181,  -182,   402,  -182,   389,
    -182,  -182,   -35,  -182,  -182,  -182,  -182,   288,  -182,  -182,
     -50,  -182,  -182,  -182,   173,   174,  -182,  -182,   362,  -182,
    -182,  -182,  -182,   215,  -182,   119,  -182,    86,  -182,  -182,
      90,  -182,   167,  -181,   185,   369,  -182,   -27,  -182,  -182,
    -182,   154,  -126,    83,    76,  -182,   158,   -31,  -182,  -182,
     221,   170,   -52,   161,   205,  -182,   -44,  -182,   -47,   -34,
     420,  -182
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    10,    11,    25,    34,    12,    26,    36,    14,    15,
      37,    46,   167,    73,    74,    75,    92,    93,    76,   100,
     168,    77,    78,   173,   174,   175,    79,    80,   195,    82,
      83,    84,   196,   197,   293,   294,   319,   320,   198,   312,
     313,   186,   187,   188,   189,   199,    86,   154,    88,    47,
      48,   259,   260,   181,   156,   225,   226,   157,   158,   227,
     228,   229,   230,   245,   246,   159,   160,   136,   161,   162,
      29,    30
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      90,   101,   180,   241,    94,   184,    16,    69,   242,   102,
      71,   106,    72,   105,    28,   107,    89,    32,    96,    69,
      87,   112,    71,   243,   244,   108,   109,   115,   110,   116,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      97,    61,    62,   232,    63,    64,    65,    66,    67,   233,
      95,    98,   114,    97,    49,   317,    40,    41,    42,    43,
      44,   243,   244,    68,    98,   222,    99,   133,    69,    70,
     318,    71,    94,   169,    33,    90,    90,    98,   177,    99,
     183,    50,   -68,    90,   190,    90,    45,   202,   -68,   163,
      90,    89,    89,    91,   176,    87,    87,   194,    87,    89,
     209,    89,   115,    87,   116,    87,    89,    95,   307,   184,
      87,    98,   237,   185,   119,   120,   204,   204,   238,   204,
     204,    90,    90,    69,  -109,   231,    71,   102,   210,   211,
     212,   213,   111,   219,   219,   103,    90,    89,    89,   247,
     217,    87,    87,   104,   224,   248,   113,   249,   219,    90,
     212,   213,    89,   250,   282,    90,    87,   108,   301,   253,
     250,   194,   316,   117,   274,    89,   323,   219,   250,    87,
     118,    89,   324,   326,   121,    87,     1,   122,   102,   327,
     210,   211,   212,   213,   124,   123,   177,   210,   211,   212,
     213,   325,   236,   125,   210,   211,   212,   213,   155,   239,
     164,   190,   176,   214,   166,    90,    87,   170,   331,   271,
     179,   272,   182,    35,   238,    39,   126,   292,   127,   128,
     129,    89,   305,   203,   205,    87,   207,   208,   130,   131,
     102,   132,   206,     2,     3,     4,     5,     6,     7,     8,
       9,   210,   211,   212,   213,   224,    90,   134,   210,   211,
     212,   213,    38,   297,   135,   137,   178,   300,   292,   200,
     215,   201,    89,   216,   234,   235,    87,     2,     3,     4,
       5,     6,     7,     8,     9,    17,    18,    19,    20,    21,
      22,    23,    24,   256,     2,     3,     4,     5,     6,     7,
       8,     9,   185,   261,   262,   263,   264,   251,   265,   266,
     252,   267,   268,   138,   139,    54,   140,  -124,   141,   142,
     143,   144,  -179,    61,   145,   270,   146,   278,   274,   273,
     295,   280,   147,   148,   210,   211,   212,   213,   149,   275,
     171,   258,   279,   281,   290,   150,   151,    95,    98,   152,
      69,   153,   284,    71,   138,   139,    54,   140,   285,   141,
     142,   143,   144,   287,    61,   145,   296,   146,    18,    19,
      20,    21,    22,   147,   148,   298,   299,   289,   238,   149,
     210,   211,   212,   213,   311,   308,   150,   151,    95,    98,
     152,    69,   153,   314,    71,   138,   139,    54,   140,   315,
     141,   142,   143,   144,   321,    61,   145,   322,   146,   329,
     328,   332,    13,    27,   147,   148,   276,   165,   277,    81,
     149,   255,   310,   333,   330,   286,    85,   150,   151,    95,
      98,   152,    69,   153,   302,    71,   138,   139,    54,   140,
     303,   141,   142,   191,   144,   288,   192,   145,   193,    63,
      64,    65,    66,   269,   304,   306,    31,   283,     0,     0,
     254,     0,     0,     0,     0,     0,     0,     0,    68,     0,
       0,     0,     0,    69,     0,     0,    71,   138,   139,    54,
     140,     0,   141,   142,   191,   144,     0,   192,   145,   193,
      63,    64,    65,    66,   138,   139,    54,   140,     0,   141,
     142,   143,   144,   291,    61,   145,     0,   146,     0,    68,
       0,     0,     0,     0,    69,     0,     0,    71,   309,     0,
       0,     0,   138,   139,    54,   140,    68,   141,   142,   143,
     144,    69,    61,   145,    71,   146,     0,   138,   139,    54,
     140,     0,   141,   142,   143,   144,     0,    61,   145,   171,
     146,     0,     0,     0,   172,     0,     0,     0,     0,    69,
       0,   218,    71,     0,     0,   138,   139,    54,   140,    68,
     141,   142,   143,   144,    69,    61,   145,    71,   146,     0,
     138,   139,    54,   140,     0,   141,   142,   143,   144,   220,
      61,   221,     0,   146,     0,     0,     0,    68,     0,     0,
       0,     0,    69,   222,     0,    71,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    98,     0,   223,     0,     0,
      71,   138,   139,    54,   140,     0,   141,   142,   143,   144,
       0,    61,   145,     0,   146,     0,   138,   139,    54,   140,
       0,   141,   142,   143,   144,   240,    61,   145,     0,   146,
       0,     0,     0,    68,     0,     0,     0,     0,    69,     0,
     257,    71,     0,     0,     0,     0,     0,     0,    68,     0,
       0,     0,     0,    69,     0,     0,    71,   138,   139,    54,
     140,     0,   141,   142,   143,   144,   291,    61,   145,     0,
     146,   138,   139,    54,   140,     0,   141,   142,   143,   144,
       0,    61,   145,     0,   146,   138,   139,    54,   140,    68,
     141,   142,   143,   144,    69,    61,   145,    71,   146,     0,
       0,     0,     0,    68,     0,     0,     0,     0,    69,     0,
       0,    71,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    69,     0,     0,    71
};

static const yytype_int16 yycheck[] =
{
      47,    53,   128,   184,    51,    41,     0,    55,    38,    53,
      58,    58,    47,    57,    52,    59,    47,    41,    52,    55,
      47,    68,    58,    53,    54,    59,    60,    45,    62,    47,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      29,    28,    29,    42,    31,    32,    33,    34,    35,    48,
      52,    53,    86,    29,    49,    41,    12,    13,    14,    15,
      16,    53,    54,    50,    53,    41,    55,   111,    55,    56,
      56,    58,   119,   123,    41,   122,   123,    53,   125,    55,
     130,    49,    42,   130,   131,   132,    42,   137,    48,   116,
     137,   122,   123,    52,   125,   122,   123,   132,   125,   130,
      43,   132,    45,   130,    47,   132,   137,    52,   289,    41,
     137,    53,    42,    45,    48,    49,   147,   148,    48,   150,
     151,   168,   169,    55,    52,   172,    58,   171,    37,    38,
      39,    40,    25,   168,   169,    56,   183,   168,   169,    42,
      49,   168,   169,    56,   171,    48,    36,    42,   183,   196,
      39,    40,   183,    48,    42,   202,   183,   191,    42,   193,
      48,   196,    42,    36,    48,   196,    42,   202,    48,   196,
      49,   202,    48,    42,    36,   202,     0,    41,   222,    48,
      37,    38,    39,    40,    37,    41,   233,    37,    38,    39,
      40,   317,    49,    41,    37,    38,    39,    40,   115,    49,
     117,   248,   233,    46,   121,   252,   233,   124,    42,    46,
     127,    48,   129,    32,    48,    34,    36,   252,    36,    41,
      36,   252,   274,   147,   148,   252,   150,   151,    41,    41,
     274,    41,   149,    57,    58,    59,    60,    61,    62,    63,
      64,    37,    38,    39,    40,   272,   293,    49,    37,    38,
      39,    40,    42,    49,    53,    41,    56,    46,   293,    36,
      36,    49,   293,    45,    36,    49,   293,    57,    58,    59,
      60,    61,    62,    63,    64,     4,     5,     6,     7,     8,
       9,    10,    11,   200,    57,    58,    59,    60,    61,    62,
      63,    64,    45,   210,   211,   212,   213,    36,   215,   216,
      41,    49,    49,    18,    19,    20,    21,    43,    23,    24,
      25,    26,    43,    28,    29,    43,    31,   234,    48,    46,
      41,   238,    37,    38,    37,    38,    39,    40,    43,    49,
      45,    44,    49,    49,   251,    50,    51,    52,    53,    54,
      55,    56,    48,    58,    18,    19,    20,    21,    49,    23,
      24,    25,    26,    49,    28,    29,    49,    31,     5,     6,
       7,     8,     9,    37,    38,    49,    44,    41,    48,    43,
      37,    38,    39,    40,    56,    41,    50,    51,    52,    53,
      54,    55,    56,    44,    58,    18,    19,    20,    21,    46,
      23,    24,    25,    26,    49,    28,    29,    36,    31,    49,
      56,    49,     0,    14,    37,    38,   233,   119,   234,    47,
      43,   196,   293,   327,   324,   248,    47,    50,    51,    52,
      53,    54,    55,    56,   270,    58,    18,    19,    20,    21,
     272,    23,    24,    25,    26,   250,    28,    29,    30,    31,
      32,    33,    34,   222,   274,   284,    26,   242,    -1,    -1,
      42,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    50,    -1,
      -1,    -1,    -1,    55,    -1,    -1,    58,    18,    19,    20,
      21,    -1,    23,    24,    25,    26,    -1,    28,    29,    30,
      31,    32,    33,    34,    18,    19,    20,    21,    -1,    23,
      24,    25,    26,    27,    28,    29,    -1,    31,    -1,    50,
      -1,    -1,    -1,    -1,    55,    -1,    -1,    58,    42,    -1,
      -1,    -1,    18,    19,    20,    21,    50,    23,    24,    25,
      26,    55,    28,    29,    58,    31,    -1,    18,    19,    20,
      21,    -1,    23,    24,    25,    26,    -1,    28,    29,    45,
      31,    -1,    -1,    -1,    50,    -1,    -1,    -1,    -1,    55,
      -1,    42,    58,    -1,    -1,    18,    19,    20,    21,    50,
      23,    24,    25,    26,    55,    28,    29,    58,    31,    -1,
      18,    19,    20,    21,    -1,    23,    24,    25,    26,    42,
      28,    29,    -1,    31,    -1,    -1,    -1,    50,    -1,    -1,
      -1,    -1,    55,    41,    -1,    58,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    53,    -1,    55,    -1,    -1,
      58,    18,    19,    20,    21,    -1,    23,    24,    25,    26,
      -1,    28,    29,    -1,    31,    -1,    18,    19,    20,    21,
      -1,    23,    24,    25,    26,    42,    28,    29,    -1,    31,
      -1,    -1,    -1,    50,    -1,    -1,    -1,    -1,    55,    -1,
      42,    58,    -1,    -1,    -1,    -1,    -1,    -1,    50,    -1,
      -1,    -1,    -1,    55,    -1,    -1,    58,    18,    19,    20,
      21,    -1,    23,    24,    25,    26,    27,    28,    29,    -1,
      31,    18,    19,    20,    21,    -1,    23,    24,    25,    26,
      -1,    28,    29,    -1,    31,    18,    19,    20,    21,    50,
      23,    24,    25,    26,    55,    28,    29,    58,    31,    -1,
      -1,    -1,    -1,    50,    -1,    -1,    -1,    -1,    55,    -1,
      -1,    58,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    55,    -1,    -1,    58
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     0,    57,    58,    59,    60,    61,    62,    63,    64,
      66,    67,    70,    72,    73,    74,     0,     4,     5,     6,
       7,     8,     9,    10,    11,    68,    71,    74,    52,   135,
     136,   135,    41,    41,    69,    70,    72,    75,    42,    70,
      12,    13,    14,    15,    16,    42,    76,   114,   115,    49,
      49,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    28,    29,    31,    32,    33,    34,    35,    50,    55,
      56,    58,    77,    78,    79,    80,    83,    86,    87,    91,
      92,    93,    94,    95,    96,   110,   111,   112,   113,   122,
     133,    52,    81,    82,   133,    52,   134,    29,    53,    55,
      84,   127,   131,    56,    56,   131,   133,   131,   134,   134,
     134,    25,   133,    36,   134,    45,    47,    36,    49,    48,
      49,    36,    41,    41,    37,    41,    36,    36,    41,    36,
      41,    41,    41,   131,    49,    53,   132,    41,    18,    19,
      21,    23,    24,    25,    26,    29,    31,    37,    38,    43,
      50,    51,    54,    56,   112,   118,   119,   122,   123,   130,
     131,   133,   134,   112,   118,    82,   118,    77,    85,    85,
     118,    45,    50,    88,    89,    90,   122,   133,    56,   118,
     117,   118,   118,    85,    41,    45,   106,   107,   108,   109,
     133,    25,    28,    30,    77,    93,    97,    98,   103,   110,
      36,    49,    85,   119,   122,   119,   118,   119,   119,    43,
      37,    38,    39,    40,    46,    36,    45,    49,    42,    77,
      42,    29,    41,    55,   112,   120,   121,   124,   125,   126,
     127,   133,    42,    48,    36,    49,    49,    42,    48,    49,
      42,   108,    38,    53,    54,   128,   129,    42,    48,    42,
      48,    36,    41,   134,    42,    98,   118,    42,    44,   116,
     117,   118,   118,   118,   118,   118,   118,    49,    49,   125,
      43,    46,    48,    46,    48,    49,    89,    90,   118,    49,
     118,    49,    42,   129,    48,    49,   107,    49,   109,    41,
     118,    27,    77,    99,   100,    41,    49,    49,    49,    44,
      46,    42,   116,   121,   126,   127,   128,   108,    41,    42,
     100,    56,   104,   105,    44,    46,    42,    41,    56,   101,
     102,    49,    36,    42,    48,   117,    42,    48,    56,    49,
     105,    42,    49,   102
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    65,    66,    66,    66,    67,    68,    68,    68,    69,
      69,    70,    71,    71,    71,    71,    71,    72,    72,    73,
      73,    74,    74,    74,    74,    74,    74,    74,    74,    75,
      75,    76,    76,    76,    76,    76,    76,    76,    76,    76,
      76,    76,    76,    76,    76,    76,    77,    77,    77,    78,
      79,    80,    81,    81,    82,    82,    83,    84,    84,    85,
      85,    86,    87,    88,    88,    88,    89,    89,    89,    89,
      89,    90,    90,    91,    92,    93,    94,    94,    95,    95,
      96,    97,    97,    98,    98,    98,    98,    98,    99,    99,
     100,   100,   101,   101,   102,   102,   103,   104,   104,   105,
     106,   106,   107,   107,   107,   108,   108,   109,   110,   111,
     111,   111,   111,   112,   112,   113,   113,   113,   113,   113,
     113,   113,   113,   113,   113,   113,   114,   114,   115,   115,
     115,   115,   115,   116,   116,   117,   117,   118,   118,   118,
     118,   118,   118,   119,   119,   119,   119,   119,   119,   119,
     119,   120,   120,   121,   122,   122,   122,   122,   123,   123,
     123,   123,   124,   124,   125,   125,   125,   125,   126,   127,
     127,   127,   128,   128,   129,   129,   130,   131,   132,   133,
     133,   134,   135,   135,   136
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     7,     1,     1,     1,     2,
       1,     7,     1,     1,     1,     1,     1,     1,     0,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     4,     2,     3,     4,
       5,     3,     3,     1,     1,     3,     6,     3,     1,     2,
       1,     6,     6,     3,     1,     0,     3,     3,     1,     2,
       1,     3,     3,     5,     6,     6,     5,     6,     6,     6,
       6,     2,     1,     5,     1,     1,     1,     1,     2,     1,
       5,     1,     3,     1,     1,     3,     6,     3,     1,     3,
       3,     1,     3,     5,     3,     3,     1,     5,     6,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     0,     1,     1,
       1,     1,     1,     1,     0,     3,     1,     3,     3,     3,
       3,     3,     1,     2,     2,     2,     2,     1,     4,     1,
       3,     3,     1,     4,     1,     3,     4,     6,     1,     1,
       1,     1,     1,     0,     3,     3,     1,     1,     3,     1,
       1,     1,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     0,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
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
      yyerror (param, YY_("syntax error: cannot back up")); \
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
                  Type, Value, param); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, struct parser_param *param)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (param);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, struct parser_param *param)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, param);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
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
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule, struct parser_param *param)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              , param);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, param); \
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
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
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
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
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
            /* Fall through.  */
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

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
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
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

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
      int yyn = yypact[*yyssp];
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
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
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
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
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
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, struct parser_param *param)
{
  YYUSE (yyvaluep);
  YYUSE (param);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yytype)
    {
          case 52: /* STRING  */
#line 225 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { free(((*yyvaluep).str)); }
#line 1491 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 55: /* IDENT  */
#line 225 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { free(((*yyvaluep).str)); }
#line 1497 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 66: /* XkbFile  */
#line 224 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { if (!param->rtrn) FreeXkbFile(((*yyvaluep).file)); }
#line 1503 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 67: /* XkbCompositeMap  */
#line 224 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { if (!param->rtrn) FreeXkbFile(((*yyvaluep).file)); }
#line 1509 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 69: /* XkbMapConfigList  */
#line 224 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { if (!param->rtrn) FreeXkbFile(((*yyvaluep).file)); }
#line 1515 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 70: /* XkbMapConfig  */
#line 224 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { if (!param->rtrn) FreeXkbFile(((*yyvaluep).file)); }
#line 1521 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 75: /* DeclList  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).any)); }
#line 1527 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 76: /* Decl  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).any)); }
#line 1533 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 77: /* VarDecl  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).var)); }
#line 1539 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 78: /* KeyNameDecl  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).keyCode)); }
#line 1545 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 79: /* KeyAliasDecl  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).keyAlias)); }
#line 1551 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 80: /* VModDecl  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).vmod)); }
#line 1557 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 81: /* VModDefList  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).vmod)); }
#line 1563 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 82: /* VModDef  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).vmod)); }
#line 1569 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 83: /* InterpretDecl  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).interp)); }
#line 1575 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 84: /* InterpretMatch  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).interp)); }
#line 1581 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 85: /* VarDeclList  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).var)); }
#line 1587 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 86: /* KeyTypeDecl  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).keyType)); }
#line 1593 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 87: /* SymbolsDecl  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).syms)); }
#line 1599 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 88: /* SymbolsBody  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).var)); }
#line 1605 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 89: /* SymbolsVarDecl  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).var)); }
#line 1611 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 90: /* ArrayInit  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1617 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 91: /* GroupCompatDecl  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).groupCompat)); }
#line 1623 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 92: /* ModMapDecl  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).modMask)); }
#line 1629 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 93: /* LedMapDecl  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).ledMap)); }
#line 1635 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 94: /* LedNameDecl  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).ledName)); }
#line 1641 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 108: /* CoordList  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1647 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 109: /* Coord  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1653 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 116: /* OptExprList  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1659 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 117: /* ExprList  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1665 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 118: /* Expr  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1671 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 119: /* Term  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1677 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 120: /* ActionList  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1683 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 121: /* Action  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1689 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 122: /* Lhs  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1695 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 123: /* Terminal  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1701 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 124: /* OptKeySymList  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1707 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 125: /* KeySymList  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1713 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 126: /* KeySyms  */
#line 219 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1719 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 135: /* OptMapName  */
#line 225 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { free(((*yyvaluep).str)); }
#line 1725 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;

    case 136: /* MapName  */
#line 225 "src/xkbcomp/parser.y" /* yacc.c:1257  */
      { free(((*yyvaluep).str)); }
#line 1731 "src/xkbcomp/parser.c" /* yacc.c:1257  */
        break;


      default:
        break;
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/*----------.
| yyparse.  |
`----------*/

int
yyparse (struct parser_param *param)
{
/* The lookahead symbol.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

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
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
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
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);

        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

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
      yychar = yylex (&yylval, param_scanner);
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

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
| yyreduce -- Do a reduction.  |
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
#line 242 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.file) = param->rtrn = (yyvsp[0].file); param->more_maps = true; }
#line 1999 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 3:
#line 244 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.file) = param->rtrn = (yyvsp[0].file); param->more_maps = true; YYACCEPT; }
#line 2005 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 4:
#line 246 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.file) = param->rtrn = NULL; param->more_maps = false; }
#line 2011 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 5:
#line 252 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.file) = XkbFileCreate((yyvsp[-5].file_type), (yyvsp[-4].str), (ParseCommon *) (yyvsp[-2].file), (yyvsp[-6].mapFlags)); }
#line 2017 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 6:
#line 255 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.file_type) = FILE_TYPE_KEYMAP; }
#line 2023 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 7:
#line 256 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.file_type) = FILE_TYPE_KEYMAP; }
#line 2029 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 8:
#line 257 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.file_type) = FILE_TYPE_KEYMAP; }
#line 2035 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 9:
#line 261 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    {
                            if (!(yyvsp[0].file))
                                (yyval.file) = (yyvsp[-1].file);
                            else
                                (yyval.file) = (XkbFile *) AppendStmt((ParseCommon *) (yyvsp[-1].file),
                                                            (ParseCommon *) (yyvsp[0].file));
                        }
#line 2047 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 10:
#line 269 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.file) = (yyvsp[0].file); }
#line 2053 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 11:
#line 275 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    {
                            if ((yyvsp[-5].file_type) == FILE_TYPE_GEOMETRY) {
                                free((yyvsp[-4].str));
                                FreeStmt((yyvsp[-2].any));
                                (yyval.file) = NULL;
                            }
                            else {
                                (yyval.file) = XkbFileCreate((yyvsp[-5].file_type), (yyvsp[-4].str), (yyvsp[-2].any), (yyvsp[-6].mapFlags));
                            }
                        }
#line 2068 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 12:
#line 287 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.file_type) = FILE_TYPE_KEYCODES; }
#line 2074 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 13:
#line 288 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.file_type) = FILE_TYPE_TYPES; }
#line 2080 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 14:
#line 289 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.file_type) = FILE_TYPE_COMPAT; }
#line 2086 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 15:
#line 290 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.file_type) = FILE_TYPE_SYMBOLS; }
#line 2092 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 16:
#line 291 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.file_type) = FILE_TYPE_GEOMETRY; }
#line 2098 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 17:
#line 294 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.mapFlags) = (yyvsp[0].mapFlags); }
#line 2104 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 18:
#line 295 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.mapFlags) = 0; }
#line 2110 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 19:
#line 298 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.mapFlags) = ((yyvsp[-1].mapFlags) | (yyvsp[0].mapFlags)); }
#line 2116 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 20:
#line 299 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.mapFlags) = (yyvsp[0].mapFlags); }
#line 2122 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 21:
#line 302 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.mapFlags) = MAP_IS_PARTIAL; }
#line 2128 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 22:
#line 303 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.mapFlags) = MAP_IS_DEFAULT; }
#line 2134 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 23:
#line 304 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.mapFlags) = MAP_IS_HIDDEN; }
#line 2140 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 24:
#line 305 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.mapFlags) = MAP_HAS_ALPHANUMERIC; }
#line 2146 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 25:
#line 306 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.mapFlags) = MAP_HAS_MODIFIER; }
#line 2152 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 26:
#line 307 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.mapFlags) = MAP_HAS_KEYPAD; }
#line 2158 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 27:
#line 308 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.mapFlags) = MAP_HAS_FN; }
#line 2164 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 28:
#line 309 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.mapFlags) = MAP_IS_ALTGR; }
#line 2170 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 29:
#line 313 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.any) = AppendStmt((yyvsp[-1].any), (yyvsp[0].any)); }
#line 2176 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 30:
#line 314 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.any) = NULL; }
#line 2182 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 31:
#line 318 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    {
                            (yyvsp[0].var)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].var);
                        }
#line 2191 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 32:
#line 323 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    {
                            (yyvsp[0].vmod)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].vmod);
                        }
#line 2200 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 33:
#line 328 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    {
                            (yyvsp[0].interp)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].interp);
                        }
#line 2209 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 34:
#line 333 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    {
                            (yyvsp[0].keyCode)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].keyCode);
                        }
#line 2218 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 35:
#line 338 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    {
                            (yyvsp[0].keyAlias)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].keyAlias);
                        }
#line 2227 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 36:
#line 343 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    {
                            (yyvsp[0].keyType)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].keyType);
                        }
#line 2236 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 37:
#line 348 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    {
                            (yyvsp[0].syms)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].syms);
                        }
#line 2245 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 38:
#line 353 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    {
                            (yyvsp[0].modMask)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].modMask);
                        }
#line 2254 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 39:
#line 358 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    {
                            (yyvsp[0].groupCompat)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].groupCompat);
                        }
#line 2263 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 40:
#line 363 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    {
                            (yyvsp[0].ledMap)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].ledMap);
                        }
#line 2272 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 41:
#line 368 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    {
                            (yyvsp[0].ledName)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].ledName);
                        }
#line 2281 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 42:
#line 372 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.any) = NULL; }
#line 2287 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 43:
#line 373 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.any) = NULL; }
#line 2293 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 44:
#line 374 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.any) = NULL; }
#line 2299 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 45:
#line 376 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    {
                            (yyval.any) = (ParseCommon *) IncludeCreate(param->ctx, (yyvsp[0].str), (yyvsp[-1].merge));
                            free((yyvsp[0].str));
                        }
#line 2308 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 46:
#line 383 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.var) = VarCreate((yyvsp[-3].expr), (yyvsp[-1].expr)); }
#line 2314 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 47:
#line 385 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.var) = BoolVarCreate((yyvsp[-1].atom), true); }
#line 2320 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 48:
#line 387 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.var) = BoolVarCreate((yyvsp[-1].atom), false); }
#line 2326 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 49:
#line 391 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.keyCode) = KeycodeCreate((yyvsp[-3].atom), (yyvsp[-1].num)); }
#line 2332 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 50:
#line 395 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.keyAlias) = KeyAliasCreate((yyvsp[-3].atom), (yyvsp[-1].atom)); }
#line 2338 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 51:
#line 399 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.vmod) = (yyvsp[-1].vmod); }
#line 2344 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 52:
#line 403 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.vmod) = (VModDef *) AppendStmt((ParseCommon *) (yyvsp[-2].vmod),
                                                      (ParseCommon *) (yyvsp[0].vmod)); }
#line 2351 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 53:
#line 406 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.vmod) = (yyvsp[0].vmod); }
#line 2357 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 54:
#line 410 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.vmod) = VModCreate((yyvsp[0].atom), NULL); }
#line 2363 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 55:
#line 412 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.vmod) = VModCreate((yyvsp[-2].atom), (yyvsp[0].expr)); }
#line 2369 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 56:
#line 418 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyvsp[-4].interp)->def = (yyvsp[-2].var); (yyval.interp) = (yyvsp[-4].interp); }
#line 2375 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 57:
#line 422 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.interp) = InterpCreate((yyvsp[-2].keysym), (yyvsp[0].expr)); }
#line 2381 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 58:
#line 424 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.interp) = InterpCreate((yyvsp[0].keysym), NULL); }
#line 2387 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 59:
#line 428 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.var) = (VarDef *) AppendStmt((ParseCommon *) (yyvsp[-1].var),
                                                     (ParseCommon *) (yyvsp[0].var)); }
#line 2394 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 60:
#line 431 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.var) = (yyvsp[0].var); }
#line 2400 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 61:
#line 437 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.keyType) = KeyTypeCreate((yyvsp[-4].atom), (yyvsp[-2].var)); }
#line 2406 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 62:
#line 443 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.syms) = SymbolsCreate((yyvsp[-4].atom), (yyvsp[-2].var)); }
#line 2412 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 63:
#line 447 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.var) = (VarDef *) AppendStmt((ParseCommon *) (yyvsp[-2].var),
                                                     (ParseCommon *) (yyvsp[0].var)); }
#line 2419 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 64:
#line 450 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.var) = (yyvsp[0].var); }
#line 2425 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 65:
#line 451 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.var) = NULL; }
#line 2431 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 66:
#line 454 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.var) = VarCreate((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2437 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 67:
#line 455 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.var) = VarCreate((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2443 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 68:
#line 456 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.var) = BoolVarCreate((yyvsp[0].atom), true); }
#line 2449 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 69:
#line 457 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.var) = BoolVarCreate((yyvsp[0].atom), false); }
#line 2455 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 70:
#line 458 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.var) = VarCreate(NULL, (yyvsp[0].expr)); }
#line 2461 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 71:
#line 462 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2467 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 72:
#line 464 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateUnary(EXPR_ACTION_LIST, EXPR_TYPE_ACTION, (yyvsp[-1].expr)); }
#line 2473 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 73:
#line 468 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.groupCompat) = GroupCompatCreate((yyvsp[-3].ival), (yyvsp[-1].expr)); }
#line 2479 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 74:
#line 472 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.modMask) = ModMapCreate((yyvsp[-4].atom), (yyvsp[-2].expr)); }
#line 2485 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 75:
#line 476 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.ledMap) = LedMapCreate((yyvsp[-4].atom), (yyvsp[-2].var)); }
#line 2491 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 76:
#line 480 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.ledName) = LedNameCreate((yyvsp[-3].ival), (yyvsp[-1].expr), false); }
#line 2497 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 77:
#line 482 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.ledName) = LedNameCreate((yyvsp[-3].ival), (yyvsp[-1].expr), true); }
#line 2503 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 78:
#line 486 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.geom) = NULL; }
#line 2509 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 79:
#line 488 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (void) (yyvsp[-2].expr); (yyval.geom) = NULL; }
#line 2515 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 80:
#line 492 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.geom) = NULL; }
#line 2521 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 81:
#line 495 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.geom) = NULL;}
#line 2527 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 82:
#line 496 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.geom) = NULL; }
#line 2533 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 83:
#line 500 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.geom) = NULL; }
#line 2539 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 84:
#line 502 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { FreeStmt((ParseCommon *) (yyvsp[0].var)); (yyval.geom) = NULL; }
#line 2545 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 85:
#line 504 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.geom) = NULL; }
#line 2551 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 86:
#line 506 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { FreeStmt((ParseCommon *) (yyvsp[0].ledMap)); (yyval.geom) = NULL; }
#line 2557 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 87:
#line 508 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.geom) = NULL; }
#line 2563 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 88:
#line 511 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.geom) = NULL;}
#line 2569 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 89:
#line 512 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.geom) = NULL; }
#line 2575 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 90:
#line 515 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.geom) = NULL; }
#line 2581 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 91:
#line 517 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { FreeStmt((ParseCommon *) (yyvsp[0].var)); (yyval.geom) = NULL; }
#line 2587 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 92:
#line 520 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.geom) = NULL; }
#line 2593 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 93:
#line 521 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.geom) = NULL; }
#line 2599 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 94:
#line 525 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.geom) = NULL; }
#line 2605 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 95:
#line 527 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { FreeStmt((ParseCommon *) (yyvsp[-1].expr)); (yyval.geom) = NULL; }
#line 2611 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 96:
#line 531 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.geom) = NULL; }
#line 2617 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 97:
#line 534 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.geom) = NULL; }
#line 2623 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 98:
#line 535 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.geom) = NULL; }
#line 2629 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 99:
#line 538 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.geom) = NULL; }
#line 2635 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 100:
#line 542 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.geom) = NULL;}
#line 2641 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 101:
#line 544 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.geom) = NULL; }
#line 2647 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 102:
#line 548 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (void) (yyvsp[-1].expr); (yyval.geom) = NULL; }
#line 2653 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 103:
#line 550 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (void) (yyvsp[-1].expr); (yyval.geom) = NULL; }
#line 2659 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 104:
#line 552 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { FreeStmt((ParseCommon *) (yyvsp[0].expr)); (yyval.geom) = NULL; }
#line 2665 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 105:
#line 556 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (void) (yyvsp[-2].expr); (void) (yyvsp[0].expr); (yyval.expr) = NULL; }
#line 2671 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 106:
#line 558 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (void) (yyvsp[0].expr); (yyval.expr) = NULL; }
#line 2677 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 107:
#line 562 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = NULL; }
#line 2683 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 108:
#line 566 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { FreeStmt((ParseCommon *) (yyvsp[-2].var)); (yyval.geom) = NULL; }
#line 2689 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 109:
#line 569 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.ival) = 0; }
#line 2695 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 110:
#line 570 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.ival) = 0; }
#line 2701 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 111:
#line 571 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.ival) = 0; }
#line 2707 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 112:
#line 572 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.ival) = 0; }
#line 2713 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 113:
#line 575 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.atom) = (yyvsp[0].atom); }
#line 2719 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 114:
#line 576 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.atom) = (yyvsp[0].atom); }
#line 2725 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 115:
#line 580 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "action"); }
#line 2731 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 116:
#line 582 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "interpret"); }
#line 2737 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 117:
#line 584 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "type"); }
#line 2743 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 118:
#line 586 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "key"); }
#line 2749 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 119:
#line 588 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "group"); }
#line 2755 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 120:
#line 590 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    {(yyval.atom) = xkb_atom_intern_literal(param->ctx, "modifier_map");}
#line 2761 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 121:
#line 592 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "indicator"); }
#line 2767 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 122:
#line 594 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.atom) = XKB_ATOM_NONE; }
#line 2773 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 123:
#line 596 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.atom) = XKB_ATOM_NONE; }
#line 2779 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 124:
#line 598 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.atom) = XKB_ATOM_NONE; }
#line 2785 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 125:
#line 600 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.atom) = XKB_ATOM_NONE; }
#line 2791 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 126:
#line 603 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.merge) = (yyvsp[0].merge); }
#line 2797 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 127:
#line 604 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.merge) = MERGE_DEFAULT; }
#line 2803 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 128:
#line 607 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.merge) = MERGE_DEFAULT; }
#line 2809 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 129:
#line 608 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.merge) = MERGE_AUGMENT; }
#line 2815 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 130:
#line 609 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.merge) = MERGE_OVERRIDE; }
#line 2821 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 131:
#line 610 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.merge) = MERGE_REPLACE; }
#line 2827 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 132:
#line 612 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    {
                    /*
                     * This used to be MERGE_ALT_FORM. This functionality was
                     * unused and has been removed.
                     */
                    (yyval.merge) = MERGE_DEFAULT;
                }
#line 2839 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 133:
#line 621 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2845 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 134:
#line 622 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = NULL; }
#line 2851 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 135:
#line 626 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (ExprDef *) AppendStmt((ParseCommon *) (yyvsp[-2].expr),
                                                      (ParseCommon *) (yyvsp[0].expr)); }
#line 2858 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 136:
#line 629 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2864 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 137:
#line 633 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateBinary(EXPR_DIVIDE, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2870 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 138:
#line 635 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateBinary(EXPR_ADD, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2876 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 139:
#line 637 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateBinary(EXPR_SUBTRACT, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2882 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 140:
#line 639 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateBinary(EXPR_MULTIPLY, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2888 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 141:
#line 641 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateBinary(EXPR_ASSIGN, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2894 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 142:
#line 643 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2900 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 143:
#line 647 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateUnary(EXPR_NEGATE, (yyvsp[0].expr)->expr.value_type, (yyvsp[0].expr)); }
#line 2906 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 144:
#line 649 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateUnary(EXPR_UNARY_PLUS, (yyvsp[0].expr)->expr.value_type, (yyvsp[0].expr)); }
#line 2912 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 145:
#line 651 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateUnary(EXPR_NOT, EXPR_TYPE_BOOLEAN, (yyvsp[0].expr)); }
#line 2918 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 146:
#line 653 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateUnary(EXPR_INVERT, (yyvsp[0].expr)->expr.value_type, (yyvsp[0].expr)); }
#line 2924 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 147:
#line 655 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr);  }
#line 2930 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 148:
#line 657 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateAction((yyvsp[-3].atom), (yyvsp[-1].expr)); }
#line 2936 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 149:
#line 659 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr);  }
#line 2942 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 150:
#line 661 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr);  }
#line 2948 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 151:
#line 665 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (ExprDef *) AppendStmt((ParseCommon *) (yyvsp[-2].expr),
                                                      (ParseCommon *) (yyvsp[0].expr)); }
#line 2955 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 152:
#line 668 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2961 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 153:
#line 672 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateAction((yyvsp[-3].atom), (yyvsp[-1].expr)); }
#line 2967 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 154:
#line 676 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateIdent((yyvsp[0].atom)); }
#line 2973 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 155:
#line 678 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateFieldRef((yyvsp[-2].atom), (yyvsp[0].atom)); }
#line 2979 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 156:
#line 680 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateArrayRef(XKB_ATOM_NONE, (yyvsp[-3].atom), (yyvsp[-1].expr)); }
#line 2985 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 157:
#line 682 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateArrayRef((yyvsp[-5].atom), (yyvsp[-3].atom), (yyvsp[-1].expr)); }
#line 2991 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 158:
#line 686 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateString((yyvsp[0].atom)); }
#line 2997 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 159:
#line 688 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateInteger((yyvsp[0].ival)); }
#line 3003 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 160:
#line 690 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = NULL; }
#line 3009 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 161:
#line 692 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateKeyName((yyvsp[0].atom)); }
#line 3015 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 162:
#line 695 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 3021 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 163:
#line 696 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = NULL; }
#line 3027 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 164:
#line 700 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprAppendKeysymList((yyvsp[-2].expr), (yyvsp[0].keysym)); }
#line 3033 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 165:
#line 702 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprAppendMultiKeysymList((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3039 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 166:
#line 704 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateKeysymList((yyvsp[0].keysym)); }
#line 3045 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 167:
#line 706 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = ExprCreateMultiKeysymList((yyvsp[0].expr)); }
#line 3051 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 168:
#line 710 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 3057 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 169:
#line 714 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    {
                            if (!resolve_keysym((yyvsp[0].str), &(yyval.keysym)))
                                parser_warn(param, "unrecognized keysym \"%s\"", (yyvsp[0].str));
                            free((yyvsp[0].str));
                        }
#line 3067 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 170:
#line 719 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.keysym) = XKB_KEY_section; }
#line 3073 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 171:
#line 721 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    {
                            if ((yyvsp[0].ival) < 0) {
                                parser_warn(param, "unrecognized keysym \"%d\"", (yyvsp[0].ival));
                                (yyval.keysym) = XKB_KEY_NoSymbol;
                            }
                            else if ((yyvsp[0].ival) < 10) {      /* XKB_KEY_0 .. XKB_KEY_9 */
                                (yyval.keysym) = XKB_KEY_0 + (xkb_keysym_t) (yyvsp[0].ival);
                            }
                            else {
                                char buf[17];
                                snprintf(buf, sizeof(buf), "0x%x", (yyvsp[0].ival));
                                if (!resolve_keysym(buf, &(yyval.keysym))) {
                                    parser_warn(param, "unrecognized keysym \"%s\"", buf);
                                    (yyval.keysym) = XKB_KEY_NoSymbol;
                                }
                            }
                        }
#line 3095 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 172:
#line 740 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.ival) = -(yyvsp[0].ival); }
#line 3101 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 173:
#line 741 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.ival) = (yyvsp[0].ival); }
#line 3107 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 174:
#line 744 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.ival) = (yyvsp[0].num); }
#line 3113 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 175:
#line 745 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.ival) = (yyvsp[0].num); }
#line 3119 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 176:
#line 748 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.ival) = 0; }
#line 3125 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 177:
#line 751 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.ival) = (yyvsp[0].num); }
#line 3131 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 178:
#line 754 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.num) = (yyvsp[0].num); }
#line 3137 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 179:
#line 757 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.atom) = xkb_atom_steal(param->ctx, (yyvsp[0].str)); }
#line 3143 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 180:
#line 758 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "default"); }
#line 3149 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 181:
#line 761 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.atom) = xkb_atom_steal(param->ctx, (yyvsp[0].str)); }
#line 3155 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 182:
#line 764 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 3161 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 183:
#line 765 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.str) = NULL; }
#line 3167 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;

  case 184:
#line 768 "src/xkbcomp/parser.y" /* yacc.c:1646  */
    { (yyval.str) = (yyvsp[0].str); }
#line 3173 "src/xkbcomp/parser.c" /* yacc.c:1646  */
    break;


#line 3177 "src/xkbcomp/parser.c" /* yacc.c:1646  */
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

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

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
      yyerror (param, YY_("syntax error"));
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
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
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
        yyerror (param, yymsgp);
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
                      yytoken, &yylval, param);
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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

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
                  yystos[yystate], yyvsp, param);
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
  yyerror (param, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, param);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, param);
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
#line 771 "src/xkbcomp/parser.y" /* yacc.c:1906  */


XkbFile *
parse(struct xkb_context *ctx, struct scanner *scanner, const char *map)
{
    int ret;
    XkbFile *first = NULL;
    struct parser_param param = {
        .scanner = scanner,
        .ctx = ctx,
        .rtrn = NULL,
    };

    /*
     * If we got a specific map, we look for it exclusively and return
     * immediately upon finding it. Otherwise, we need to get the
     * default map. If we find a map marked as default, we return it
     * immediately. If there are no maps marked as default, we return
     * the first map in the file.
     */

    while ((ret = yyparse(&param)) == 0 && param.more_maps) {
        if (map) {
            if (streq_not_null(map, param.rtrn->name))
                return param.rtrn;
            else
                FreeXkbFile(param.rtrn);
        }
        else {
            if (param.rtrn->flags & MAP_IS_DEFAULT) {
                FreeXkbFile(first);
                return param.rtrn;
            }
            else if (!first) {
                first = param.rtrn;
            }
            else {
                FreeXkbFile(param.rtrn);
            }
        }
        param.rtrn = NULL;
    }

    if (ret != 0) {
        FreeXkbFile(first);
        return NULL;
    }

    if (first)
        log_vrb(ctx, 5,
                "No map in include statement, but \"%s\" contains several; "
                "Using first defined map, \"%s\"\n",
                scanner->file_name, first->name);

    return first;
}
