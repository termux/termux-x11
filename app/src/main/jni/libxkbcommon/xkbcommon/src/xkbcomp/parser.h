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

#ifndef YY__XKBCOMMON_PARSER_H_INCLUDED
# define YY__XKBCOMMON_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int _xkbcommon_debug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    END_OF_FILE = 0,               /* END_OF_FILE  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    ERROR_TOK = 255,               /* ERROR_TOK  */
    XKB_KEYMAP = 1,                /* XKB_KEYMAP  */
    XKB_KEYCODES = 2,              /* XKB_KEYCODES  */
    XKB_TYPES = 3,                 /* XKB_TYPES  */
    XKB_SYMBOLS = 4,               /* XKB_SYMBOLS  */
    XKB_COMPATMAP = 5,             /* XKB_COMPATMAP  */
    XKB_GEOMETRY = 6,              /* XKB_GEOMETRY  */
    XKB_SEMANTICS = 7,             /* XKB_SEMANTICS  */
    XKB_LAYOUT = 8,                /* XKB_LAYOUT  */
    INCLUDE = 10,                  /* INCLUDE  */
    OVERRIDE = 11,                 /* OVERRIDE  */
    AUGMENT = 12,                  /* AUGMENT  */
    REPLACE = 13,                  /* REPLACE  */
    ALTERNATE = 14,                /* ALTERNATE  */
    VIRTUAL_MODS = 20,             /* VIRTUAL_MODS  */
    TYPE = 21,                     /* TYPE  */
    INTERPRET = 22,                /* INTERPRET  */
    ACTION_TOK = 23,               /* ACTION_TOK  */
    KEY = 24,                      /* KEY  */
    ALIAS = 25,                    /* ALIAS  */
    GROUP = 26,                    /* GROUP  */
    MODIFIER_MAP = 27,             /* MODIFIER_MAP  */
    INDICATOR = 28,                /* INDICATOR  */
    SHAPE = 29,                    /* SHAPE  */
    KEYS = 30,                     /* KEYS  */
    ROW = 31,                      /* ROW  */
    SECTION = 32,                  /* SECTION  */
    OVERLAY = 33,                  /* OVERLAY  */
    TEXT = 34,                     /* TEXT  */
    OUTLINE = 35,                  /* OUTLINE  */
    SOLID = 36,                    /* SOLID  */
    LOGO = 37,                     /* LOGO  */
    VIRTUAL = 38,                  /* VIRTUAL  */
    EQUALS = 40,                   /* EQUALS  */
    PLUS = 41,                     /* PLUS  */
    MINUS = 42,                    /* MINUS  */
    DIVIDE = 43,                   /* DIVIDE  */
    TIMES = 44,                    /* TIMES  */
    OBRACE = 45,                   /* OBRACE  */
    CBRACE = 46,                   /* CBRACE  */
    OPAREN = 47,                   /* OPAREN  */
    CPAREN = 48,                   /* CPAREN  */
    OBRACKET = 49,                 /* OBRACKET  */
    CBRACKET = 50,                 /* CBRACKET  */
    DOT = 51,                      /* DOT  */
    COMMA = 52,                    /* COMMA  */
    SEMI = 53,                     /* SEMI  */
    EXCLAM = 54,                   /* EXCLAM  */
    INVERT = 55,                   /* INVERT  */
    STRING = 60,                   /* STRING  */
    INTEGER = 61,                  /* INTEGER  */
    FLOAT = 62,                    /* FLOAT  */
    IDENT = 63,                    /* IDENT  */
    KEYNAME = 64,                  /* KEYNAME  */
    PARTIAL = 70,                  /* PARTIAL  */
    DEFAULT = 71,                  /* DEFAULT  */
    HIDDEN = 72,                   /* HIDDEN  */
    ALPHANUMERIC_KEYS = 73,        /* ALPHANUMERIC_KEYS  */
    MODIFIER_KEYS = 74,            /* MODIFIER_KEYS  */
    KEYPAD_KEYS = 75,              /* KEYPAD_KEYS  */
    FUNCTION_KEYS = 76,            /* FUNCTION_KEYS  */
    ALTERNATE_GROUP = 77           /* ALTERNATE_GROUP  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 164 "parser.y"

        int64_t          num;
        enum xkb_file_type file_type;
        char            *str;
        xkb_atom_t      atom;
        enum merge_mode merge;
        enum xkb_map_flags mapFlags;
        xkb_keysym_t    keysym;
        ParseCommon     *any;
        struct { ParseCommon *head; ParseCommon *last; } anyList;
        ExprDef         *expr;
        struct { ExprDef *head; ExprDef *last; } exprList;
        VarDef          *var;
        struct { VarDef *head; VarDef *last; } varList;
        VModDef         *vmod;
        struct { VModDef *head; VModDef *last; } vmodList;
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
        struct { XkbFile *head; XkbFile *last; } fileList;

#line 158 "parser.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int _xkbcommon_parse (struct parser_param *param);

#endif /* !YY__XKBCOMMON_PARSER_H_INCLUDED  */
