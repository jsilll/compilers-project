#ifndef __L22_AST_PRINT_NODE_H__
#define __L22_AST_PRINT_NODE_H__

#include <cdk/ast/expression_node.h>

namespace l22
{

  /**
   * Class for describing writeln nodes.
   */
  class print_node : public cdk::basic_node
  {
    cdk::expression_node *_argument; // TODO: change this to cdk::sequence_node
    bool _newline = false;

  public:
    inline print_node(int lineno, cdk::expression_node *argument, bool newline = false) : cdk::basic_node(lineno), _argument(argument)
    {
    }

  public:
    inline cdk::expression_node *argument()
    {
      return _argument;
    }

    void accept(basic_ast_visitor *sp, int level)
    {
      sp->do_print_node(this, level);
    }
  };

} // l22

#endif
