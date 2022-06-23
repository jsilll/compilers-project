#ifndef __L22_AST_LAMBDA_NODE_H__
#define __L22_AST_LAMBDA_NODE_H__

#include <string>
#include <cdk/ast/typed_node.h>
#include <cdk/ast/sequence_node.h>
#include "ast/block_node.h"

namespace l22
{

    /**
     * Class for describing function definition nodes.
     */
    class lambda_node : public cdk::expression_node
    {
        std::shared_ptr<cdk::basic_type> _return_type;
        cdk::sequence_node *_arguments;
        l22::block_node *_block;

    public:
        lambda_node(int lineno, std::shared_ptr<cdk::basic_type> return_type,
                    cdk::sequence_node *arguments,
                    l22::block_node *block)
            : cdk::expression_node(lineno), _return_type(return_type), _arguments(arguments), _block(block)
        {
        }

    public:
        cdk::sequence_node *arguments()
        {
            return _arguments;
        }
        cdk::typed_node *argument(size_t ax)
        {
            return dynamic_cast<cdk::typed_node *>(_arguments->node(ax));
        }
        l22::block_node *block()
        {
            return _block;
        }

        std::shared_ptr<cdk::basic_type> return_type()
        {
            return _return_type;
        }

        void accept(basic_ast_visitor *sp, int level)
        {
            sp->do_lambda_node(this, level);
        }
    };

} // l22

#endif
