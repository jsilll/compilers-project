#ifndef __L22_AST_FUNCTION_CALL_NODE_H__
#define __L22_AST_FUNCTION_CALL_NODE_H__

#include <cdk/ast/typed_node.h>
#include <cdk/ast/sequence_node.h>
#include "ast/block_node.h"

namespace l22
{

    /**
     * Class for describing function definition nodes.
     */
    class function_call_node : public cdk::expression_node
    {
        cdk::expression_node *_lambda_ptr;
        cdk::sequence_node *_arguments;

    public:
        function_call_node(int lineno, cdk::expression_node *lambda_ptr, cdk::sequence_node *arguments)
            : cdk::expression_node(lineno), _lambda_ptr(lambda_ptr), _arguments(arguments)
        {
        }

    public:
        cdk::expression_node *lambda_ptr()
        {
            return _lambda_ptr;
        }

        inline cdk::sequence_node *arguments()
        {
            return _arguments;
        }

        inline cdk::expression_node *argument(size_t ix)
        {
            return dynamic_cast<cdk::expression_node *>(_arguments->node(ix));
        }

        void accept(basic_ast_visitor *sp, int level)
        {
            sp->do_function_call_node(this, level);
        }
    };

} // l22

#endif
