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

%parse-param { std::shared_ptr<cdk::compiler> compiler }

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
  std::vector<std::shared_ptr<cdk::basic_type>> *vtypes;
};

%token tBEGIN tEND
%token tIF tELIF tTHEN tELSE
%token tINPUT
%token tNOT tAND tOR tEQ tNE tLE tGE
%token tNULL_PTR
%token tRETURN
%token tSIZEOF
%token tTYPE_DOUBLE tTYPE_INT tTYPE_TEXT tTYPE_VOID
%token tVAR
%token tWHILE tDO tSTOP tAGAIN
%token tWRITE tWRITELN

%token<d> tDOUBLE
%token<i> tFOREIGN tPUBLIC tUSE tPRIVATE
%token<i> tINTEGER 
%token<s> tID
%token<s> tTEXT

%nonassoc tIF
%nonassoc tELIF

%left '*' '/' '%'
%left '+' '-'
%left '<' tLE tGE '>'
%left tAND
%left tNE tEQ
%left tOR
%right tNOT
%right '='
%right tUMINUS

%type<block> block 
%type<declaration> arg_desc declaration  
%type<expression> expr lambda opt_initializer 
%type<lvalue> lvalue
%type<node> program instruction elif 
%type<s> text
%type<sequence> exprs file opt_arg_decs declarations opt_declarations opt_exprs instructions opt_instructions
%type<type> function_type type
%type<vtypes> arg_types

%%

/* TODO: Regras do lvalue: será que isto contempla tudo e / ou nao contempla coisas a mais? */
/* TODO: Conflitos: ver a ordem das regras (porque isso importa) e ver a as precedencias?? (%left e tBATATA) */

file : opt_declarations         { compiler->ast($$ = $1); }
     | opt_declarations program { compiler->ast($$ = new cdk::sequence_node(LINE, $2, $1)); }
     ;

program : tBEGIN block tEND { $$ = new l22::program_node(LINE, $2); }
        ;

block : '{' opt_declarations opt_instructions '}' { $$ = new l22::block_node(LINE, $2, $3); }
      ;

opt_declarations : /* empty */  { $$ = new cdk::sequence_node(LINE); }
                 | declarations { $$ = $1; }
                 ;

declarations : declaration                  { $$ = new cdk::sequence_node(LINE, $1); }
             | declaration ';'              { $$ = new cdk::sequence_node(LINE, $1); }
             | declarations declaration     { $$ = new cdk::sequence_node(LINE, $2, $1); }
             | declarations declaration ';' { $$ = new cdk::sequence_node(LINE, $2, $1); }
             ;

opt_instructions : /* empty */  { $$ = new cdk::sequence_node(LINE); }
                 | instructions { $$ = $1; }
                 ;

instructions : instruction                  { $$ = new cdk::sequence_node(LINE, $1); }   
             | instruction ';'              { $$ = new cdk::sequence_node(LINE, $1); }
             | instructions instruction     { $$ = new cdk::sequence_node(LINE, $2, $1); }
             | instructions instruction ';' { $$ = new cdk::sequence_node(LINE, $2, $1); }    

declaration :           type tID opt_initializer { $$ = new l22::declaration_node(LINE, tPRIVATE, $1, *$2, $3); delete $2; }
            | tPUBLIC   type tID opt_initializer { $$ = new l22::declaration_node(LINE, tPUBLIC, $2, *$3, $4); delete $3; }  
            |           tVAR tID '=' expr        { $$ = new l22::declaration_node(LINE, tPRIVATE, nullptr, *$2, $4); delete $2; }  
            | tPUBLIC        tID '=' expr        { $$ = new l22::declaration_node(LINE, tPUBLIC, nullptr, *$2, $4); delete $2; }
            | tPUBLIC   tVAR tID '=' expr        { $$ = new l22::declaration_node(LINE, tPUBLIC, nullptr, *$3, $5); delete $3; }
            | tFOREIGN  tVAR tID '=' expr        { $$ = new l22::declaration_node(LINE, tFOREIGN, nullptr, *$3, $5); delete $3; }
            | tFOREIGN  type tID                 { $$ = new l22::declaration_node(LINE, $1, $2, *$3, nullptr); delete $3; }
            | tUSE      type tID                 { $$ = new l22::declaration_node(LINE, $1, $2, *$3, nullptr); delete $3; }
            ;

opt_initializer : /* empty */ { $$ = nullptr; }
                | '=' expr    { $$ = $2; }
                ;

type : tTYPE_INT     { $$ = cdk::primitive_type::create(4, cdk::TYPE_INT); }
     | tTYPE_DOUBLE  { $$ = cdk::primitive_type::create(8, cdk::TYPE_DOUBLE); }
     | tTYPE_TEXT    { $$ = cdk::primitive_type::create(4, cdk::TYPE_STRING); }
     | tTYPE_VOID    { $$ = cdk::primitive_type::create(0, cdk::TYPE_VOID); }
     | '[' type ']'  { $$ = cdk::reference_type::create(4, $2); }
     | function_type { $$ = $1; }
     ;

function_type : type '<' arg_types '>' { 
                                         auto v = std::vector<std::shared_ptr<cdk::basic_type>>();
                                         v.push_back($1);
                                         $$ = cdk::functional_type::create(v, *$3);
                                       }
              | type '<'           '>' { 
                                         auto v = std::vector<std::shared_ptr<cdk::basic_type>>();
                                         v.push_back($1);
                                         $$ = cdk::functional_type::create(v);
                                       }
              ;

arg_types : type               { $$ = new std::vector<std::shared_ptr<cdk::basic_type>>(); $$->push_back($1); }
          | arg_types ',' type { $1->push_back($3); $$ = $1; }
          ;

instruction : expr           { $$ = new l22::evaluation_node(LINE, $1); }
            
            /* Print Instructions */
            | tWRITE   exprs { $$ = new l22::print_node(LINE, $2); }
            | tWRITELN exprs { $$ = new l22::print_node(LINE, $2, true); }
            
            /* Early Stop Instructions */
            | tAGAIN         { $$ = new l22::again_node(LINE); }
            | tSTOP          { $$ = new l22::stop_node(LINE); }
            | tRETURN        { $$ = new l22::return_node(LINE); }
            | tRETURN expr   { $$ = new l22::return_node(LINE, $2); }

            /* Conditional Instructions */
            | tIF     '(' expr ')' tTHEN block            { $$ = new l22::if_node(LINE, $3, $6); }
            | tIF     '(' expr ')' tTHEN block elif { $$ = new l22::if_else_node(LINE, $3, $6, $7); }
            
            /* Iteration Instructions */
            | tWHILE  '(' expr ')' tDO block              { $$ = new l22::while_node(LINE, $3, $6); }
  
            /* Block Instrcution */
            | block                                       { $$ = $1; }
            ;

lambda : '(' opt_arg_decs ')' '-' '>' type ':' block { $$ = new l22::lambda_node(LINE, $6, $2, $8); }
       ;

opt_arg_decs : /* empty */              { $$ = new cdk::sequence_node(LINE); }
             | arg_desc                  { $$ = new cdk::sequence_node(LINE, $1); }
             | opt_arg_decs ',' arg_desc { $$ = new cdk::sequence_node(LINE, $3, $1); }
             ;

arg_desc : type tID { $$ = new l22::declaration_node(LINE, tPRIVATE, $1, *$2, nullptr); delete $2; }
         ;

exprs : expr             { $$ = new cdk::sequence_node(LINE, $1); }
      | exprs ',' expr   { $$ = new cdk::sequence_node(LINE, $3, $1); }
      ;

opt_exprs : /* empty */ { $$ = new cdk::sequence_node(LINE); }
          | exprs       { $$ = $1; }
          ;

elif : tELSE  block                              { $$ = $2; }
     | tELIF '(' expr ')' tTHEN block            { $$ = new l22::if_node(LINE, $3, $6); }
     | tELIF '(' expr ')' tTHEN block elif { $$ = new l22::if_else_node(LINE, $3, $6, $7); }
     ;

expr : tINTEGER                  { $$ = new cdk::integer_node(LINE, $1); }
     | tDOUBLE                   { $$ = new cdk::double_node(LINE, $1); }
     | text                      { $$ = new cdk::string_node(LINE, $1); }
     | tNULL_PTR                 { $$ = new l22::nullptr_node(LINE); }
     
     /* Left Values */
     | lvalue                    { $$ = new cdk::rvalue_node(LINE, $1); }
     
     /* Assignments */
     | lvalue '=' expr           { $$ = new cdk::assignment_node(LINE, $1, $3); }
     
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
     | '-'  expr %prec tUMINUS   { $$ = new cdk::neg_node(LINE, $2); }
     | '+'  expr %prec tUMINUS   { $$ = new l22::identity_node(LINE, $2); }
     | tNOT expr                 { $$ = new cdk::not_node(LINE, $2); }
     
     /* Input Expression */
     | tINPUT                    { $$ = new l22::input_node(LINE); }
     
     /* Lambdas */
     | lambda                    { $$ = $1; }
     
     /* Function Calls */

     /* TODO generalize for lvalues function calling */
     | tID     '(' opt_exprs ')' { $$ = new l22::function_call_node(LINE, *$1, $3); delete $1; }
     | lambda  '(' opt_exprs ')' { $$ = new l22::function_call_node(LINE, $1, $3); }
     | '@'     '(' opt_exprs ')' { $$ = new l22::function_call_node(LINE, "@", $3);  }
     | tSIZEOF '(' expr      ')' { $$ = new l22::sizeof_node(LINE, $3); }
     
     /* Memory Expressions */
     | '('     expr       ')'    { $$ = $2; }
     | '['     expr       ']'    { $$ = new l22::stack_alloc_node(LINE, $2); }
     | lvalue '?'                { $$ = new l22::address_of_node(LINE, $1); }
     ;

text : tTEXT      { $$ = $1; }
     | text tTEXT { $$->append(*$2); delete $2; }
     ;

lvalue : tID                 { $$ = new cdk::variable_node(LINE, *$1); delete $1; }
       | expr   '[' expr ']' { $$ = new l22::index_node(LINE, $1, $3); }
       | lvalue '[' expr ']' { $$ = new l22::index_node(LINE, new cdk::rvalue_node(LINE, $1), $3); }
       ;

%%