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
%token tSELF

%token<d> tDOUBLE
%token<i> tFOREIGN tPUBLIC tUSE tPRIVATE
%token<i> tINTEGER 
%token<s> tID
%token<s> tTEXT

%nonassoc tIF
%nonassoc tTHEN
%nonassoc tELIF tELSE

%nonassoc tWHILE
%nonassoc tDO

%right '='
%left tOR
%left tAND
%right tNOT
%left tNE tEQ
%left '<' tLE tGE '>'
%left '+' '-'
%left '*' '/' '%'
%right tUMINUS

%type<block> block 
%type<declaration> declaration arg_declaration   
%type<expression> expression block_expression lambda initializer opt_initializer 
%type<lvalue> lvalue
%type<node> program instruction block_instruction elif 
%type<s> text
%type<sequence> expressions file arg_declarations opt_arg_declarations declarations opt_declarations opt_exprs instructions opt_instructions
%type<type> function_type type
%type<vtypes> arg_types

%%

file : opt_declarations         { compiler->ast($$ = $1); }
     | opt_declarations program { compiler->ast($$ = new cdk::sequence_node(LINE, $2, $1)); }
     ;

program : tBEGIN block tEND { $$ = new l22::program_node(LINE, $2); }
        ;

block : '{' opt_declarations opt_instructions '}' { $$ = new l22::block_node(LINE, $2, $3); }
      ;

opt_declarations : /* empty */  { $$ = nullptr; }
                 | declarations { $$ = $1; }
                 ;

declarations : declaration              { $$ = new cdk::sequence_node(LINE, $1); }
             | declaration declarations { std::reverse($2->nodes().begin(), $2->nodes().end()); $$ = new cdk::sequence_node(LINE, $1, $2); std::reverse($$->nodes().begin(), $$->nodes().end());  }
             ;

opt_instructions : /* empty */  { $$ = nullptr; }
                 | instructions { $$ = $1; }
                 ;

instructions : instruction                    { $$ = new cdk::sequence_node(LINE, $1); }   
             | block_instruction              { $$ = new cdk::sequence_node(LINE, $1); }
             | instruction ';' instructions   {  std::reverse($3->nodes().begin(), $3->nodes().end()); $$ = new cdk::sequence_node(LINE, $1, $3); std::reverse($$->nodes().begin(), $$->nodes().end());  }
             | block_instruction instructions {  std::reverse($2->nodes().begin(), $2->nodes().end()); $$ = new cdk::sequence_node(LINE, $1, $2); std::reverse($$->nodes().begin(), $$->nodes().end()); }
             ;

declaration :           type tID opt_initializer { $$ = new l22::declaration_node(LINE, tPRIVATE, $1, *$2, $3); delete $2; }
            | tPUBLIC   type tID opt_initializer { $$ = new l22::declaration_node(LINE, tPUBLIC, $2, *$3, $4); delete $3; }  
            |           tVAR tID initializer     { $$ = new l22::declaration_node(LINE, tPRIVATE, nullptr, *$2, $3); delete $2; }  
            | tPUBLIC        tID initializer     { $$ = new l22::declaration_node(LINE, tPUBLIC, nullptr, *$2, $3); delete $2; }
            | tPUBLIC   tVAR tID initializer     { $$ = new l22::declaration_node(LINE, tPUBLIC, nullptr, *$3, $4); delete $3; }
            | tFOREIGN  type tID ';'             { $$ = new l22::declaration_node(LINE, tFOREIGN, $2, *$3, nullptr); delete $3; }
            | tUSE      type tID ';'             { $$ = new l22::declaration_node(LINE, tUSE, $2, *$3, nullptr); delete $3; }
            ;

opt_initializer : ';'         { $$ = nullptr; }
                | initializer { $$ = $1; }
                ;

initializer : '=' expression ';'   { $$ = $2; }
            | '=' block_expression { $$ = $2; }
            ;

type : tTYPE_INT        { $$ = cdk::primitive_type::create(4, cdk::TYPE_INT); }
     | tTYPE_DOUBLE     { $$ = cdk::primitive_type::create(8, cdk::TYPE_DOUBLE); }
     | tTYPE_TEXT       { $$ = cdk::primitive_type::create(4, cdk::TYPE_STRING); }
     | tTYPE_VOID       { $$ = cdk::primitive_type::create(0, cdk::TYPE_VOID); }
     | '[' type ']'     { $$ = cdk::reference_type::create(4, $2); }
     | function_type    { $$ = $1; }
     ;

function_type : type '<' arg_types '>' { 
                                         auto v = std::vector<std::shared_ptr<cdk::basic_type>>();
                                         v.push_back($1);
                                         $$ = cdk::functional_type::create(v, *$3);
                                         delete $3;
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

instruction : expression         { $$ = new l22::evaluation_node(LINE, $1); }
            | tWRITE   expressions     { $$ = new l22::print_node(LINE, $2); }
            | tWRITELN expressions     { $$ = new l22::print_node(LINE, $2, true); }
            | tAGAIN             { $$ = new l22::again_node(LINE); }
            | tSTOP              { $$ = new l22::stop_node(LINE); }
            | tRETURN            { $$ = new l22::return_node(LINE); }
            | tRETURN expression { $$ = new l22::return_node(LINE, $2); }
            ;

block_instruction : tIF     '(' expression ')' tTHEN block      { $$ = new l22::if_node(LINE, $3, $6); }
                  | tIF     '(' expression ')' tTHEN block elif { $$ = new l22::if_else_node(LINE, $3, $6, $7); }
                  | tWHILE  '(' expression ')' tDO block        { $$ = new l22::while_node(LINE, $3, $6); }
                  | tRETURN lambda                              { $$ = new l22::return_node(LINE, $2); }
                  | lvalue '=' block_expression                 { $$ = new cdk::assignment_node(LINE, $1, $3); }
                  | block                                       { $$ = $1; }
                  ;

lambda : '(' opt_arg_declarations ')' '-' '>' type ':' block { $$ = new l22::lambda_node(LINE, $6, $2, $8); }
       ;

opt_arg_declarations : /* empty */      { $$ = nullptr; }
                     | arg_declarations { $$ = $1; }
                     ;

arg_declarations : arg_declaration                      { $$ = new cdk::sequence_node(LINE, $1); }
                 | arg_declarations ',' arg_declaration { $$ = new cdk::sequence_node(LINE, $3, $1); }
                 ; 

arg_declaration : type tID { $$ = new l22::declaration_node(LINE, tPRIVATE, $1, *$2, nullptr); delete $2; }
                ;

expressions : expression                       { $$ = new cdk::sequence_node(LINE, $1); }
            | expressions ',' expression       { $$ = new cdk::sequence_node(LINE, $3, $1); }
            | block_expression                 { $$ = new cdk::sequence_node(LINE, $1);   }
            | expressions ',' block_expression { $$ = new cdk::sequence_node(LINE, $3, $1); }
            ;

opt_exprs : /* empty */ { $$ = nullptr; }
          | expressions { $$ = $1; }
          ;

elif : tELSE  block                              { $$ = $2; }
     | tELIF '(' expression ')' tTHEN block      { $$ = new l22::if_node(LINE, $3, $6); }
     | tELIF '(' expression ')' tTHEN block elif { $$ = new l22::if_else_node(LINE, $3, $6, $7); }
     ;

expression : tINTEGER                         { $$ = new cdk::integer_node(LINE, $1); }
           | tDOUBLE                          { $$ = new cdk::double_node(LINE, $1); }
           | text                             { $$ = new cdk::string_node(LINE, $1); }
           | tNULL_PTR                        { $$ = new l22::nullptr_node(LINE); }
           
           /* Left Values */
           | lvalue                           { $$ = new cdk::rvalue_node(LINE, $1); }
           
           /* Assignments */
           | lvalue '='     expression        { $$ = new cdk::assignment_node(LINE, $1, $3); }
           | lvalue '=' '[' expression ']'    { $$ = new cdk::assignment_node(LINE, $1, new l22::stack_alloc_node(LINE, $4)); }
           
           /* Arithmetic Expressions */
           | expression '+' expression	      { $$ = new cdk::add_node(LINE, $1, $3); }
           | expression '-' expression	      { $$ = new cdk::sub_node(LINE, $1, $3); }
           | expression '*' expression	      { $$ = new cdk::mul_node(LINE, $1, $3); }
           | expression '/' expression	      { $$ = new cdk::div_node(LINE, $1, $3); }
           | expression '%' expression	      { $$ = new cdk::mod_node(LINE, $1, $3); }
           
           /* Logical Expressions */
           | expression '<' expression	      { $$ = new cdk::lt_node(LINE, $1, $3); }
           | expression '>' expression	      { $$ = new cdk::gt_node(LINE, $1, $3); }
           | expression tGE expression	      { $$ = new cdk::ge_node(LINE, $1, $3); }
           | expression tLE expression        { $$ = new cdk::le_node(LINE, $1, $3); }
           | expression tNE expression	      { $$ = new cdk::ne_node(LINE, $1, $3); }
           | expression tEQ expression	      { $$ = new cdk::eq_node(LINE, $1, $3); }
           
           /* Logical Expressions */
           | expression tAND expression       { $$ = new cdk::and_node(LINE, $1, $3); }
           | expression tOR  expression       { $$ = new cdk::or_node (LINE, $1, $3); }
           
           /* Unary Expressions */
           | '-'  expression %prec tUMINUS    { $$ = new cdk::neg_node(LINE, $2); }
           | '+'  expression %prec tUMINUS    { $$ = new l22::identity_node(LINE, $2); }
           | tNOT expression                  { $$ = new cdk::not_node(LINE, $2); }
           
           /* Input Expression */
           | tINPUT                           { $$ = new l22::input_node(LINE); }
           
           /* Function Calls */
           | '(' lambda ')' '(' opt_exprs ')' { $$ = new l22::function_call_node(LINE, $2, $5); }
           | lvalue       '(' opt_exprs   ')' { $$ = new l22::function_call_node(LINE, new cdk::rvalue_node(LINE, $1), $3); }
           | tSIZEOF      '(' expression  ')' { $$ = new l22::sizeof_node(LINE, $3); }
           
           /* Wrap */
           | '('     expression       ')'     { $$ = $2; }
           
           /* Memory Expressions */
           | lvalue '?'                       { $$ = new l22::address_of_node(LINE, $1); }
           ;

block_expression : lambda                      { $$ = $1; }
                 | lvalue '=' block_expression { $$ = new cdk::assignment_node(LINE, $1, $3); }

text : tTEXT      { $$ = $1; }
     | text tTEXT { $$->append(*$2); delete $2; }
     ;

lvalue : tID                                        { $$ = new cdk::variable_node(LINE, *$1); delete $1; }
       | tSELF                                      { $$ = new cdk::variable_node(LINE, "@"); }
       | lvalue             '['    expression   ']' { $$ = new l22::index_node(LINE, new cdk::rvalue_node(LINE, $1), $3); }
       | '(' expression ')' '['    expression   ']' { $$ = new l22::index_node(LINE, $2, $5); }
       ;

%%