/* A Bison parser, made by GNU Bison 3.8.2.  */

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

#ifndef YY_CFGP_USERS_CAITLIN_ROSS_DEV_DIGITAL_TWIN_SOFTWARE_DIGITAL_TWIN_SRC_MODEL_NET_CONFIGPARSER_H_INCLUDED
# define YY_CFGP_USERS_CAITLIN_ROSS_DEV_DIGITAL_TWIN_SOFTWARE_DIGITAL_TWIN_SRC_MODEL_NET_CONFIGPARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int cfgp_debug;
#endif
/* "%code requires" blocks.  */
#line 34 "/Users/caitlin.ross/dev/digital-twin/software/digital-twin/src/model-net/configparser.y"

  
  #ifndef YY_TYPEDEF_YY_SCANNER_T
  #define YY_TYPEDEF_YY_SCANNER_T
  typedef void* yyscan_t;
  #endif
   

#line 58 "/Users/caitlin.ross/dev/digital-twin/software/digital-twin/src/model-net/configparser.h"

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    LITERAL_STRING = 258,          /* LITERAL_STRING  */
    OPENSECTION = 259,             /* OPENSECTION  */
    CLOSESECTION = 260,            /* CLOSESECTION  */
    IDENTIFIER = 261,              /* IDENTIFIER  */
    EQUAL_TOKEN = 262,             /* EQUAL_TOKEN  */
    SEMICOLUMN = 263,              /* SEMICOLUMN  */
    KOMMA = 264,                   /* KOMMA  */
    LOPEN = 265,                   /* LOPEN  */
    LCLOSE = 266                   /* LCLOSE  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 44 "/Users/caitlin.ross/dev/digital-twin/software/digital-twin/src/model-net/configparser.y"

        struct
        {
            char string_buf [512];
            unsigned int curstringpos;
        };


#line 95 "/Users/caitlin.ross/dev/digital-twin/software/digital-twin/src/model-net/configparser.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif




int cfgp_parse (yyscan_t * scanner, ParserParams * param);


#endif /* !YY_CFGP_USERS_CAITLIN_ROSS_DEV_DIGITAL_TWIN_SOFTWARE_DIGITAL_TWIN_SRC_MODEL_NET_CONFIGPARSER_H_INCLUDED  */
