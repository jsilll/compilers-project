%option c++ prefix="l22_scanner_" outfile="l22_scanner.cpp"
%option stack noyywrap yylineno 8bit debug
%{ 
// make relevant includes before including the parser's tab file
#include <sstream>
#include <string>
#include <vector>

#include <cdk/ast/sequence_node.h>
#include <cdk/ast/expression_node.h>
#include <cdk/ast/lvalue_node.h>
#include "l22_parser.tab.h"

static bool restart_functions = false;

static int lLevelsToPop = 0, lIndex = 0;
static std::vector<int> indents;   // known indentation levels
static std::vector<int> deferred;  // deferred tokens

// output stream for building string literals
static std::ostringstream strlit;

inline bool valid(int indent) {
    lLevelsToPop = lIndex = -1;
    bool indentOk = false;
    
    for (size_t ix = 0; ix < indents.size(); ix++) {
        if (indent == indents[ix]) {
            lLevelsToPop = indents.size() - ix - 1;
            lIndex = ix;
            indentOk = true;
        }
    }

    return indentOk;
}

// don't change this
#define yyerror LexerError
%}

SPACE        [ ]
INT          [[:digit:]]+

LETTER       [A-Za-z_]
IDENTIFIER   {LETTER}({LETTER}|[[:digit:]])*

%x X_COMMENT
%x X_TEXT  
%x X_NEWLINE
%x X_BACKSLASH

%%
  yydebug=1; set_debug(1);
  {
        //std::cerr << "################" << std::endl;
        set_debug(true);
        if (restart_functions) {
          std::cerr << "POPPING FUNCTIONS (" << indents.size() << ")" << std::endl;
          indents.pop_back();
          if (indents.size() == 0) restart_functions = false;
          return '}';
        }
        else {
          if (lLevelsToPop > 0) {
            std::cerr << "POPPING LEVELS (" << lLevelsToPop << ")" << std::endl;
            lLevelsToPop--;
            indents.pop_back();
            return '}';
          }
        }
        
  }

  /* ====================================================================== */
  /* ====[                      3.2.2 - INDENTATION                   ]==== */
  /* ====================================================================== */

\n                   yy_push_state(X_NEWLINE); /* delay decision */

<X_NEWLINE>^{SPACE}+  {
    yy_pop_state();
    if (valid(yyleng)) {
      std::cerr << "CASE 1" << std::endl;
      if (lLevelsToPop == 0) {
        // at the same level: ignore index, do not return tINDENT;
        std::cerr << "CASE 1a (levels to pop = 0)" << std::endl;
        return ';';
      }
      else {
        // nothing is done here: the scanner will ignore the input while levels > 0
        // and send block-end tokens to the parser
        std::cerr << "CASE 1b (levels to pop = " << lLevelsToPop << ")" << std::endl;
        // at least one has to be returned here...
        if (lLevelsToPop > 0) {
          std::cerr << "POPPING LEVELS (" << lLevelsToPop << ")" << std::endl;
          lLevelsToPop--;
          indents.pop_back();
          return '}';
        }
      }
    }
    else if (indents.size() == 0) {
          // first block to open
          std::cerr << "CASE 2" << std::endl;
          indents.push_back(yyleng);
          return '{';
    }
    else if (yyleng > indents.back()) {
          // this is like the one before, but not the first
          std::cerr << "CASE 3" << std::endl;
          indents.push_back(yyleng);
          return '{';
    }
    else {
          // something wrong: bad indent: communicate to user
          std::cerr << "CASE 4" << std::endl;
          std::cerr << "bad indent at line " << yylineno;
    }

}
<X_NEWLINE>\n      ;
<X_NEWLINE>.       {
  // in this case, we have a new function starting
  // first, return whatever we are seeing to the input
  yyless(0);
  yy_pop_state();
  if (indents.size() > 0) {
    std::cerr << "should restart " << indents.size() << " functions at " << yylineno << std::endl;
    indents.pop_back();
    if (indents.size() > 0) { restart_functions = true; }
    return '}';
  }
  else return ';'; // top-level declarations
}

  /* ====================================================================== */
  /* ====[                  3.3 - COMMENTS                            ]==== */
  /* ====================================================================== */

{SPACE}*"..."\n                ;   /* ignore continuation marks */

";".*\n                        ;   /* match the whole line (it is ignored) */

"(*"                           yy_push_state(X_COMMENT);
<X_COMMENT>"*)"                yy_pop_state();
<X_COMMENT>.|\n                ;   /* ignore inside comments */


  /* ====================================================================== */
  /* ====[                  3.4 Palavras-chave                        ]==== */
  /* ====================================================================== */

"begin"                        return tBEGIN;
"end"                          return tEND;

  /* ====================================================================== */
  /* ====[                  3.5 - Types                               ]==== */
  /* ====================================================================== */

"int"                          return tTYPE_INT;
"double"                       return tTYPE_DOUBLE;
"text"                         return tTYPE_TEXT;
"void"                         return tTYPE_VOID;
[\[\]<>]                       return *yytext;

  /* =============================================================================== */
  /* ====[                  3.9.1 - Integers                                   ]==== */      
  /* =============================================================================== */

{INT}                          yylval.i = strtoul(yytext, NULL, 10); return tINTEGER;

  /* =============================================================================== */
  /* ====[                  3.9.2 - Floating Point Reals                       ]==== */                        
  /* =============================================================================== */

([0-9]*\.[0-9]+|[0-9]+\.[0-9]*)([eE][-+]?[0-9]+)? { yylval.d = strtod(yytext, NULL); return tDOUBLE; }

  /* =============================================================================== */
  /* ====[                  3.9.3 - Strings                                    ]==== */                   
  /* =============================================================================== */

"\""                           yy_push_state(X_TEXT);
<X_TEXT>\\                     yy_push_state(X_BACKSLASH);
<X_TEXT>"\""                   {
                                 yylval.s = new std::string(strlit.str());
                                 strlit.str("");
                                 yy_pop_state();
                                 return tTEXT;
                               }
<X_TEXT>.                      strlit << *yytext;

<X_BACKSLASH>n                 strlit << '\n'; yy_pop_state();
<X_BACKSLASH>r                 strlit << '\r'; yy_pop_state();
<X_BACKSLASH>t                 strlit << '\t'; yy_pop_state();
<X_BACKSLASH>\\                strlit << '\\'; yy_pop_state();

<X_BACKSLASH>[0-6]{1,3}        { strlit << (char)(unsigned char)strtoul(yytext, NULL, 7); yy_pop_state(); }
<X_BACKSLASH>.                 strlit << *yytext; yy_pop_state();

  /* =============================================================================== */
  /* ====[                  3.9.4 - Pointers                                   ]==== */       
  /* =============================================================================== */

"null"                         return tNULL_PTR;

  /* ====================================================================== */
  /* ====[                    4.5 - Global Symbols                    ]==== */
  /* ====================================================================== */

"foreign"                      return tFOREIGN;
"public"                       return tPUBLIC;
"use"                          return tUSE;
"var"                          return tVAR;

  /* ========================================================================= */
  /* ====[                 5.2 - Function Calls                          ]==== */
  /* ========================================================================= */

"@"                            return *yytext;

  /* ========================================================================= */
  /* ====[                 6.2 - Conditional Instructions                ]==== */
  /* ========================================================================= */

"if"                           return tIF;
"elif"                         return tELIF;
"then:"                        return tTHEN;
"else:"                        return tELSE;

  /* ===================================================================== */
  /* ====[                 6.3 - Iteration Instructions              ]==== */
  /* ===================================================================== */

"while"                        return tWHILE; 
"do:"                          return tDO;

  /* ===================================================================== */
  /* ====[                 6.4 - Termination Instructions            ]==== */
  /* ===================================================================== */

"stop"                         return tSTOP;

  /* ===================================================================== */
  /* ====[                 6.5 - Instrução de continuação            ]==== */
  /* ===================================================================== */

"again"                        return tAGAIN;

  /* ====================================================================== */
  /* ====[                  6.6 - Return Instruction                  ]==== */
  /* ====================================================================== */

"return"                       return tRETURN;

  /* ====================================================================== */
  /* ====[                  6.6 - Printing Instructions               ]==== */
  /* ====================================================================== */

"write"                        return tWRITE;
"writeln"                      return tWRITELN;

  /* ====================================================================== */
  /* ====[                 7 - Expressions                            ]==== */
  /* ====================================================================== */

[\[\]()+-?*/%<>=]              return *yytext;
"not"                          return tNOT;
"and"                          return tAND;
"or"                           return tOR;
"=="                           return tEQ;
"!="                           return tNE;
"<="                           return tLE;
">="                           return tGE;
"&&"                           return tAND;
"||"                           return tOR;

  /* ====================================================================== */
  /* ====[                 7.1.2 - Reading Expression                 ]==== */
  /* ====================================================================== */

"input"                        return tINPUT;

  /* ====================================================================== */
  /* ====[                 7.2.5 -  Dimension Expression              ]==== */
  /* ====================================================================== */

"sizeof"                       return tSIZEOF;

  /* ====================================================================== */
  /* ====[                 3.8 - Identifiers                          ]==== */
  /* ====================================================================== */

{IDENTIFIER}                   yylval.s = new std::string(yytext); return tID;

  /* ====================================================================== */
  /* ====[                    Other                                   ]==== */
  /* ====================================================================== */

<<EOF>>                {
  std::cout << "AT <<EOF>> OPEN INDENTS ARE " << indents.size() << std::endl;
  if (indents.size() > 0) {
    indents.pop_back();
    return '}';
  }
  return 0;  // nothing left to do: tell parser to proceed
}

[[:space:]]                    /* ignore remaining white space inside the program */
.                              yyerror(yytext);    /* all other chars: error! */

%%
