#ifndef __L22_AST_WHILE_NODE_H__
#define __L22_AST_WHILE_NODE_H__

#include <cdk/ast/expression_node.h>

namespace l22
{

  /**
   * Class for describing while-cycle nodes.
   */
  class while_node : public cdk::basic_node
  {
    cdk::expression_node *_condition;
    l22::block_node *_block; // TODO: change this to l22::block_node (parser needs to recognize blocks)

  public:
    inline while_node(int lineno, cdk::expression_node *condition, l22::block_node *block) : basic_node(lineno), _condition(condition), _block(block)
    {
    }

  public:
    inline cdk::expression_node *condition()
    {
      return _condition;
    }
    inline l22::block_node *block()
    {
      return _block;
    }

    void accept(basic_ast_visitor *sp, int level)
    {
      sp->do_while_node(this, level);
    }
  };

} // l22

#endif
