#ifndef __L22_AST_VARIABLE_DECLARATION_NODE_H_
#define __L22_AST_VARIABLE_DECLARATION_NODE_H_

#include <cdk/ast/expression_node.h>

namespace l22
{
    /**
     * Class for describing if-then-else nodes.
     */
    class variable_declaration_node : public cdk::typed_node
    {
        int _qualifier;
        std::string _identifier;
        cdk::expression_node *_expression;

    public:
        variable_declaration_node(int lineno,
                                  int qualifier,
                                  std::string &indentifier,
                                  cdk::expression_node *expression,
                                  std::shared_ptr<cdk::basic_type> varType = cdk::primitive_type::create(0, cdk::TYPE_VOID))
            : typed_node(lineno), _identifier(indentifier), _expression(expression)
        {
            type(varType);
        }

    public:
        inline int qualifier()
        {
            return _qualifier;
        }

        inline std::string &identifier()
        {
            return _identifier;
        }

        inline cdk::expression_node *expression()
        {
            return _expression;
        }

        void accept(basic_ast_visitor *sp, int level)
        {
            sp->do_variable_declaration_node(this, level);
        }
    };
}

#endif