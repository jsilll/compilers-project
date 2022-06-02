%option c++ prefix="l22_scanner_" outfile="l22_scanner.cpp"
%option stack noyywrap yylineno 8bit debug
%{ 
// make relevant includes before including the parser's tab file
#include <string>
#include <cdk/ast/sequence_node.h>
#include <cdk/ast/expression_node.h>
#include <cdk/ast/lvalue_node.h>
#include "l22_parser.tab.h"

// don't change this
#define yyerror LexerError
%}
SPACE     ([ \t\r]|\n)  /* check if space is like this */
INT       [[:digit:]]+

LETTER  [A-Za-z_]
IDENTIFIER   [A-Za-z][A-Za-z0-9_]*

%x X_COMMENT
%x X_STRING  X_BACKSLASH   X_CHAR    X_CHAR_END
%x X_OCTAL   X_HEXADECIMAL X_HEX_INT X_BIN_INT

%%
yydebug=1; set_debug(1);

  /* ====================================================================== */
  /* ====[                      2.2 - COMMENTS                        ]==== */
  /* ====================================================================== */

";".*$                /* match the whole line (it is ignored) */

"(*"                   yy_push_state(X_COMMENT);
<X_COMMENT>"*)"        yy_pop_state();
<X_COMMENT>.|"\n"      ;  /* ignore inside comments */

  /* ====================================================================== */
  /* ====[                  1.1   - Tipos de dados                    ]==== */
  /* ====[                  1.1   - Null pointers                     ]==== */
  /* ====[                  1.1   - Procedures                        ]==== */
  /* ====================================================================== */

"int"                     return tTYPE_INT;
"double"                  return tTYPE_REAL;
"text"                    return tTYPE_STRING;
"void"                    return tVOID;
"null"                    return tNULL_PTR;


"var"                     return tTYPE_VAR;
[\[\]]                    return tTYPE_POINTER; /* ver melhor tipo de ponteiro */
"sizeof"                  return tSIZEOF;

  /* ====================================================================== */
  /* ====[                  3.4.2 - Símbolos globais                  ]==== */
  /* ====================================================================== */

"public"                 return tPUBLIC;
"foreign"                return tFOREIGN;
"use"                    return tUSE;

 /* ====================================================================== */
  /* ====[                 5.5 - Instrução condicional                ]==== */
  /* ====================================================================== */

"if"                   return tIF;
"then"                 return tTHEN;
"else"                 return tELSE;

  /* ====================================================================== */
  /* ====[                 5.5 - Instrução de iteração                ]==== */
  /* ====[                 5.5 - Instrução de terminação              ]==== */
  /* ====[                 5.4 - Instrução de continuação             ]==== */
  /* ====================================================================== */

"while"                return tWHILE;
"do"                   return tDO;
"stop"                 return tSTOP;
"again"                return tAGAIN;

  /* ====================================================================== */
  /* ====[                 6 - Operadores de expressões               ]==== */
  /* ====================================================================== */

  /* [\[\](),?]             return *yytext; */
[-+*/%~<>@?=]          return *yytext;
"=="                   return tEQ;
"!="                   return tNE;
"<="                   return tLE;
">="                   return tGE;
"and"                  return tAND;
"or"                   return tOR;
"input"                return tINPUT;

 /* ====================================================================== */
  /* ====[              3   - Delimitadores e separadores             ]==== */
  /* ====[              4.3 - Corpo (de função)                       ]==== */
  /* ====[                 5.3 - Instrução de retorno                 ]==== */
  /* ====================================================================== */

[#\[\](){}:!,]          return *yytext;
"return"                return tRETURN;
"write"                 return tWRITE;
"writeln"               return tWRITELN;
"print"                 return tPRINT; 
"begin"                 return tBEGIN; 
"end"                   return tEND; 

  /* ====================================================================== */
  /* ====[                   2.7.1 - inteiros                         ]==== */
  /* ====================================================================== */

"0"                   yy_push_state(X_HEPT_INT);

  /* ver heptadecimal melhor */
<X_HEPT_INT>[[:xdigit:]]+      { yylval.i = strtoul(yytext, NULL, 7); yy_pop_state(); return tINTEGER; }
<X_HEPT_INT>.|\n               std::cerr << "WARNING: line " << lineno() << ": bad hexadecimal data!" << std::endl; exit(1);

{INT}                  yylval.i = strtoul(yytext, NULL, 10); return tINTEGER;

  /* ====================================================================== */
  /* ====[                   2.7.2 - reais                            ]==== */
  /* ====================================================================== */

([0-9]*\.[0-9]+|[0-9]+\.[0-9]*)([eE][-+]?[0-9]+)? { yylval.d = strtod(yytext, NULL); return tREAL; }

  /* ====================================================================== */
  /* ====[              2.7.3 - Cadeias de caracteres                 ]==== */
  /* ====================================================================== */

\"                     yy_push_state(X_STRING); yylval.s = new std::string("");
<X_STRING>\"           yy_pop_state(); return tSTRING;
<X_STRING>\\\"         *yylval.s += yytext + 1;
<X_STRING>.            *yylval.s += yytext;
<X_STRING>\n           yyerror("newline in string");

  /* ====================================================================== */
  /* ====[                2.6 - Identificadores (nomes)               ]==== */
  /* ====================================================================== */

{IDENTIFIER}  yylval.s = new std::string(yytext); return tID;

  /* ====================================================================== */
  /* ====[                    (other characters)                      ]==== */
  /* ====================================================================== */

[ \t\n]+               ; /* ignore whitespace */
.                      yyerror("Unknown character");

  /* ====================================================================== */
  /* ====[                           THE END                          ]==== */
  /* ====================================================================== */

%%
