%{
//-- don't change *any* of these: if you do, you'll break the compiler.
#include <algorithm>
#include <memory>
#include <cstring>
#include <cdk/compiler.h>
#include <cdk/types/types.h>
#include ".auto/all_nodes.h"
#define LINE                         compiler->scanner()->lineno()
#define yylex()                      compiler->scanner()->scan()
#define yyerror(compiler, s)         compiler->scanner()->error(s)
//-- don't change *any* of these --- END!
%}

%parse-param {std::shared_ptr<cdk::compiler> compiler}

%union {
  //--- don't change *any* of these: if you do, you'll break the compiler.
  YYSTYPE() : type(cdk::primitive_type::create(0, cdk::TYPE_VOID)) {}
  ~YYSTYPE() {}
  YYSTYPE(const YYSTYPE &other) { *this = other; }
  YYSTYPE& operator=(const YYSTYPE &other) { type = other.type; return *this; }

  std::shared_ptr<cdk::basic_type> type;        /* expression type */
  //-- don't change *any* of these --- END!

  int                   i;	/* integer value */
  std::string          *s;	/* symbol name or string literal */
  cdk::basic_node      *node;	/* node pointer */
  cdk::sequence_node   *sequence;
  cdk::expression_node *expression; /* expression nodes */
  cdk::lvalue_node     *lvalue;
};

%token <i> tINTEGER
%token <s> tIDENTIFIER tSTRING
%token tWHILE tIF tPRINT tREAD tBEGIN tEND

%nonassoc tIFX
%nonassoc tELSE

%right '='
%left tGE tLE tEQ tNE '>' '<'
%left '+' '-'
%left '*' '/' '%'
%nonassoc tUNARY

%type <node> stmt program
%type <sequence> list
%type <expression> expr
%type <lvalue> lval

%{
//-- The rules below will be included in yyparse, the main parsing function.
%}
%%

// ver como fazer para representar o file node
// file : declarations program {  }

program : tBEGIN block tEND { compiler->ast(new l22::program_node(LINE, $2)); }
        ;

declaration : qualifier tTYPE_VAR tID '=' expr            { $$ = new l22::declaration_node(LINE, $1, nullptr, *$3, $5); }
            |           tTYPE_VAR tID '=' expr            { $$ = new l22::declaration_node(LINE, tPRIVATE, nullptr, *$2, $4); }     
            |                     tID '=' expr            { $$ = new l22::declaration_node(LINE, tPRIVATE, nullptr, *$1, $3); } 
            | qualifier type      tID  opt_initializer    { $$ = new l22::declaration_node(LINE, $1, $2, *$3, $5); }  
            |           type      tID  opt_initializer    { $$ = new l22::declaration_node(LINE, tPRIVATE, $1, *$2, $4); }
            ;

qualifier : tPRIVATE { $$ = $1; }
          | tPUBLIC  { $$ = $1; }
          | tUSE     { $$ = $1; }
          ;

type : tTYPE_INT          { $$ = cdk::primitive_type::create(4, cdk::TYPE_INT); }
     | tTYPE_DOUBLE       { $$ = cdk::primitive_type::create(8, cdk::TYPE_DOUBLE);  }
     | tTYPE_STRING       { $$ = cdk::primitive_type::create(4, cdk::TYPE_STRING);  }
     | '[' data_type ']'  { $$ = cdk::reference_type::create(4, $2); }
     // ver como fazer com function type
     //| func_type          { $$ = cdk::primitive_type::create }
     ;

opt_initializer : /* empty */         { $$ = nullptr; /* must be nullptr, not NIL */ }
                | '=' expr            { $$ = $2; }
                ;

function : '(' argdecs ')' '-''>' type ':' block { $$ = new l22::lambda_node(LINE, $6, $2, $8); }
         ;

argdecs : /* empty */     { $$ = new cdk::sequence_node(LINE); }
     | argdec             { $$ = new cdk::sequence_node(LINE, $1); }
     | argdecs ',' argdec { $$ = new cdk::sequence_node(LINE, $2, $1); }

argdec : type tID         { $$ = new cdk::declaration_node(LINE, tPRIVATE, $1, *$2, nullptr) }

block : '{'            opt_instructions '}' { $$ = new l22::block_node(LINE, nullptr, $2); }
      | '{' innerdecls opt_instructions '}' { $$ = new l22::block_node(LINE, $2, $3); }
      ;

innerdecls :            declaration ';' { $$ = new cdk::sequence_node(LINE, $1);     }
           | innerdecls declaration ';' { $$ = new cdk::sequence_node(LINE, $2, $1); }
           ;

opt_instructions : /* empty */  { $$ = new cdk::sequence_node(LINE); }
                 | instructions { $$ = $1; }
                 ;

instructions : instruction              { $$ = new cdk::sequence_node(LINE, $1); }
             | instructions instruction { $$ = new cdk::sequence_node(LINE, $2, $1); }
             ;

///////////  instructions here   //////////

// ver como fazer com os pontos e virgula
instruction : expr ';'                                 { $$ = new l22::evaluation_node(LINE, $1); }
            //print instructions
            | tWRITE exprs ';'                         { $$ = new l22::print_node(LINE, $2); }
            | tWRITELN exprs ';'                       { $$ = new l22::print_node(LINE, $2, true); }
            //early stop instructions
            | tAGAIN ';'                               { $$ = new l22::again_node(LINE); }
            | tSTOP ';'                                { $$ = new l22::stop_node(LINE); }
            | tRETURN ';'                              { $$ = new l22::return_node(LINE); }
            | tRETURN expr ';'                         { $$ = new l22::return_noed(LINE, $2) }
            //conditional instructions 
            | tIF '(' expr ')' 'then' ':' block        { $$ = new l22::if_node(LINE, $3, $7); }
            | tIF '(' expr ')' 'then' ':' block elsif  { $$ = new l22::if_else_node(LINE, $3, $7, $8); }
            //loop instructions
            | tWHILE '(' expr ')' 'do' ':' block       { $$ = new l22::while_node(LINE, $3, $7); }
            //block
            | block                                    { $$ = $1; }
            ;
            
exprs : expr                            { $$ = new cdk::sequence_node(LINE, $1); }
      | exprs ',' expr                  { $$ = new cdk::sequence_node(LINE, $3, $1); }
      ;

elsif : tELSE ':' block                                       { $$ = $3; }
      | tELIF '(' expression ')' 'then' ':' block             { $$ = new l22::if_node(LINE, $3, $7); }
      | tELIF '(' expression ')' 'then' ':' block elsif       { $$ = new l22::if_else_node(LINE, $3, $7, $8); }

////////  expressions here  ////////

// function vai ter de aparecer nas expressions
expr : tINTEGER                         { $$ = new cdk::integer_node(LINE, $1); }
     | tSTRING                          { $$ = new cdk::string_node(LINE, $1); }
     | '-' expr %prec tUNARY            { $$ = new cdk::neg_node(LINE, $2); }
     | expr '+' expr	               { $$ = new cdk::add_node(LINE, $1, $3); }
     | expr '-' expr	               { $$ = new cdk::sub_node(LINE, $1, $3); }
     | expr '*' expr	               { $$ = new cdk::mul_node(LINE, $1, $3); }
     | expr '/' expr	               { $$ = new cdk::div_node(LINE, $1, $3); }
     | expr '%' expr	               { $$ = new cdk::mod_node(LINE, $1, $3); }
     | expr '<' expr	               { $$ = new cdk::lt_node(LINE, $1, $3); }
     | expr '>' expr	               { $$ = new cdk::gt_node(LINE, $1, $3); }
     | expr tGE expr	               { $$ = new cdk::ge_node(LINE, $1, $3); }
     | expr tLE expr                    { $$ = new cdk::le_node(LINE, $1, $3); }
     | expr tNE expr	               { $$ = new cdk::ne_node(LINE, $1, $3); }
     | expr tEQ expr	               { $$ = new cdk::eq_node(LINE, $1, $3); }
     | '(' expr ')'                     { $$ = $2; }
     | lval                             { $$ = new cdk::rvalue_node(LINE, $1); }  //FIXME
     | lval '=' expr                    { $$ = new cdk::assignment_node(LINE, $1, $3); }
     ;

lval : tID                              { $$ = new cdk::variable_node(LINE, $1); }
     ;

%%
