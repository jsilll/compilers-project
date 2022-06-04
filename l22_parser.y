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

  std::shared_ptr<cdk::basic_type> type; /* expression type */
  //-- don't change *any* of these --- END!

  int                   i;	      /* integer value */
  double                d;	      /* double value */
  std::string          *s;	      /* symbol name or string literal */
  
  cdk::basic_node      *node;	      /* node pointer */
  cdk::sequence_node   *sequence;
  cdk::expression_node *expression; /* expression nodes */
  cdk::lvalue_node     *lvalue;

  l22::block_node                               *block;
  l22::declaration_node                         *declaration;
  std::vector<std::shared_ptr<cdk::basic_type>> vtypes;
};

%token tINDENT
%token tBEGIN tEND
%token tIF tELIF tTHEN tELSE
%token tWHILE  tDO tSTOP tAGAIN
%token tVAR
%token tWRITE tWRITELN
%token tNOT tAND tOR tEQ tNE tLE tGE
%token tINPUT
%token tSIZEOF
%token tNULL_PTR
%token tTYPE_DOUBLE tTYPE_INT tTYPE_TEXT tTYPE_VOID
%token tRETURN
%token tUNARY

%token<i> tINTEGER 
%token<d> tDOUBLE
%token<s> tTEXT
%token<s> tID
%token<i> tFOREIGN tPUBLIC tUSE tPRIVATE

/* TODO: ver se é preciso acrescentar alguma cena a este bloco */
%nonassoc tIF
%nonassoc tELIF

%right '='
%left tOR
%left tAND
%right '~'
%left tNE tEQ
%left '<' tLE tGE '>'
%left '+' '-'
%left '*' '/' '%'
%right tUMINUS

%type<node> elif_block
%type<block> block 
%type<declaration> argdec declaration
%type<expression> expr lambda opt_initializer
%type<i> qualifier
%type<lvalue> lval
%type<node> instruction program
%type<sequence> exprs file opt_arg_decs opt_declarations opt_exprs opt_instructions
%type<s> text
%type<type> function_type type
%type<vtypes> arg_types

%%

/* TODO: ver como fazer com virgulas */
/* TODO: ver a ordem das regras (porque isso importa) e ver a as precedencias?? (%left e tBATATA) */

/* TODO: ver como é que o resto do pessoal esta a fazer esta parte */
file : opt_declarations         { compiler->ast($$ = $1); }
     | opt_declarations program { compiler->ast($$ = new cdk::sequence_node(LINE, $2, $1)); }
     ;

program : tEND block tBEGIN     { compiler->ast(new l22::program_node(LINE, $2)); }
        ;

block : '{' opt_declarations opt_instructions '}' { $$ = new l22::block_node(LINE, $2, $3); }
      ;

opt_declarations : /* empty */                  { $$ = new cdk::sequence_node(LINE); }
                 |                  declaration { $$ = new cdk::sequence_node(LINE, $1); }
                 | opt_declarations declaration { $$ = new cdk::sequence_node(LINE, $2, $1);  }

opt_instructions : /* empty */                  { $$ = new cdk::sequence_node(LINE); }
                 | instruction                  { $$ = new cdk::sequence_node(LINE, $1); }
                 | opt_instructions instruction { $$ = new cdk::sequence_node(LINE, $2, $1); }
                 ;

declaration : qualifier tVAR tID '=' expr         { $$ = new l22::declaration_node(LINE, $1, nullptr, *$3, $5); }
            |           tVAR tID '=' expr         { $$ = new l22::declaration_node(LINE, tPRIVATE, nullptr, *$2, $4); }     
            |                tID '=' expr         { $$ = new l22::declaration_node(LINE, tPRIVATE, nullptr, *$1, $3); } 
            | qualifier type tID  opt_initializer { $$ = new l22::declaration_node(LINE, $1, $2, *$3, $4); }  
            |           type tID  opt_initializer { $$ = new l22::declaration_node(LINE, tPRIVATE, $1, *$2, $3); }
            ;

opt_initializer : /* empty */ { $$ = nullptr; }
                | '=' expr    { $$ = $2; }
                ;

qualifier : tFOREIGN { $$ = $1; }
          | tPUBLIC  { $$ = $1; }
          | tUSE     { $$ = $1; }
          ;

type : tTYPE_INT     { $$ = cdk::primitive_type::create(4, cdk::TYPE_INT); }
     | tTYPE_DOUBLE  { $$ = cdk::primitive_type::create(8, cdk::TYPE_DOUBLE);  }
     | tTYPE_TEXT    { $$ = cdk::primitive_type::create(4, cdk::TYPE_STRING);  }
     | '[' type ']'  { $$ = cdk::reference_type::create(4, $2); }
     | function_type { $$ = $1; }
     ;

function_type : type '<' arg_types '>' { 
                                         auto v = std::vector<std::shared_ptr<cdk::basic_type>>();
                                         v.push_back($1);
                                         $$ = cdk::functional_type::create(v, $3);
                                       }
              | type '<'           '>' { 
                                         auto v = std::vector<std::shared_ptr<cdk::basic_type>>();
                                         v.push_back($1);
                                         $$ = cdk::functional_type::create(v);
                                       }
              ;

arg_types : type               { $$ = std::vector<std::shared_ptr<cdk::basic_type>>(); $$.push_back($1); }
          | arg_types ',' type { $1.push_back($3); $$ = $1; }
          ;

instruction : expr                              { $$ = new l22::evaluation_node(LINE, $1); }
            
            /* Print Instructions */
            | tWRITE   exprs                    { $$ = new l22::print_node(LINE, $2); }
            | tWRITELN exprs                    { $$ = new l22::print_node(LINE, $2, true); }
            
            /* Early Stop Instructions */
            | tAGAIN                            { $$ = new l22::again_node(LINE); }
            | tSTOP                             { $$ = new l22::stop_node(LINE); }
            | tRETURN                           { $$ = new l22::return_node(LINE); }
            | tRETURN expr                      { $$ = new l22::return_node(LINE, $2); }
            
            /* Conditional Instructions  */
            | tIF '(' expr ')' tTHEN block            { $$ = new l22::if_node(LINE, $3, $6); }
            | tIF '(' expr ')' tTHEN block elif_block { $$ = new l22::if_else_node(LINE, $3, $6, $7); }
            
            /* Loop Instructions */
            | tWHILE '(' expr ')' tDO block     { $$ = new l22::while_node(LINE, $3, $6); }
             
            /* Block */
            | block                             { $$ = $1; }
            ;

lambda : '(' opt_arg_decs ')' '-''>' type ':' block { $$ = new l22::lambda_node(LINE, $6, $2, $8); }
       ;

opt_arg_decs : /* empty */            { $$ = new cdk::sequence_node(LINE); }
            | argdec                  { $$ = new cdk::sequence_node(LINE, $1); }
            | opt_arg_decs ',' argdec { $$ = new cdk::sequence_node(LINE, $3, $1); }

argdec : type tID { $$ = new l22::declaration_node(LINE, tPRIVATE, $1, *$2, nullptr); }
       ;

exprs : expr       { $$ = new cdk::sequence_node(LINE, $1); }
      | exprs expr { $$ = new cdk::sequence_node(LINE, $2, $1); }
      ;

opt_exprs : /* empty */ { $$ = new cdk::sequence_node(LINE); }
          | exprs       { $$ = $1; }
          ;

elif_block : tELSE  block                              { $$ = $2; }
           | tELIF '(' expr ')' tTHEN block            { $$ = new l22::if_node(LINE, $3, $6); }
           | tELIF '(' expr ')' tTHEN block elif_block { $$ = new l22::if_else_node(LINE, $3, $6, $7); }

expr : tINTEGER                  { $$ = new cdk::integer_node(LINE, $1); }
     | tDOUBLE                   { $$ = new cdk::double_node(LINE, $1); }
     | text                      { $$ = new cdk::string_node(LINE, $1); }
     | tNULL_PTR                 { $$ = new l22::nullptr_node(LINE); }
     
     /* Left Values */
     | lval                      { $$ = new cdk::rvalue_node(LINE, $1); }
     
     /* Assignments */
     | lval '=' expr     { $$ = new cdk::assignment_node(LINE, $1, $3); }
     
     /* Arithmetic Expressions */
     | expr '+' expr	        { $$ = new cdk::add_node(LINE, $1, $3); }
     | expr '-' expr	        { $$ = new cdk::sub_node(LINE, $1, $3); }
     | expr '*' expr	        { $$ = new cdk::mul_node(LINE, $1, $3); }
     | expr '/' expr	        { $$ = new cdk::div_node(LINE, $1, $3); }
     | expr '%' expr	        { $$ = new cdk::mod_node(LINE, $1, $3); }
     
     /* Logical Expressions */
     | expr '<' expr	        { $$ = new cdk::lt_node(LINE, $1, $3); }
     | expr '>' expr	        { $$ = new cdk::gt_node(LINE, $1, $3); }
     | expr tGE expr	        { $$ = new cdk::ge_node(LINE, $1, $3); }
     | expr tLE expr             { $$ = new cdk::le_node(LINE, $1, $3); }
     | expr tNE expr	        { $$ = new cdk::ne_node(LINE, $1, $3); }
     | expr tEQ expr	        { $$ = new cdk::eq_node(LINE, $1, $3); }
     
     /* Logical Expressions */
     | expr tAND expr            { $$ = new cdk::and_node(LINE, $1, $3); }
     | expr tOR  expr            { $$ = new cdk::or_node (LINE, $1, $3); }
     
     /* Unary Expressions */
     | '-' expr %prec tUNARY     { $$ = new cdk::neg_node(LINE, $2); }
     | '+' expr %prec tUNARY     { $$ = new l22::identity_node(LINE, $2); }
     | '~' expr                  { $$ = new cdk::not_node(LINE, $2); }
     
     /* Input Expression */
     | tINPUT                    { $$ = new l22::input_node(LINE); }
     
     /* Lambdas */
     | lambda                    { $$ = $1; }  
     
     /* Function Calls */
     | lambda  '(' opt_exprs ')' { /* TODO ver como fazer a nova function call */ }
     | tID     '(' opt_exprs ')' { $$ = new l22::function_call_node(LINE, *$1, $3); delete $1; }
     | '@'     '(' opt_exprs ')' { $$ = new l22::function_call_node(LINE, nullptr, $3);  }
     | tSIZEOF '(' expr     ')'  { $$ = new l22::sizeof_node(LINE, $3); }
     
     /* Memory Expressions */
     | '('     expr       ')'    { $$ = $2; }
     | '['     expr ']'          { $$ = new l22::stack_alloc_node(LINE, $2); }
     | lval         '?'          { $$ = new l22::address_of_node(LINE, $1); }
     ;

text : tTEXT      { $$ = $1; }
     | text tTEXT { $$->append(*$2); delete $2; }

lval : tID { $$ = new cdk::variable_node(LINE, $1); }
     ;

%%