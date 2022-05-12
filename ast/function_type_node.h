#ifndef __L22_AST_FUNCTION_TYPE_NODE_H_
#define __L22_AST_FUNCTION_TYPE_NODE_H_

#include <cdk/ast/expression_node.h>
#include <cdk/ast/sequence_node.h>

namespace l22
{
    /**
     * Class for describing if-then-else nodes.
     */
    class function_type_node : public cdk::typed_node
    {
        cdk::sequence_node *_arguments;
        cdk::basic_node *_return_type;
        cdk::basic_node *_block;

    public:
        inline function_type_node(int lineno,
                                  cdk::sequence_node *arguments,
                                  cdk::basic_node *return_type,
                                  cdk::basic_node *block)
            : typed_node(lineno), _arguments(arguments), _return_type(return_type), _block(block)
        {
            type();
        }

    public:
        inline cdk::basic_node *return_type()
        {
            return _return_type;
        }
        inline cdk::basic_node *block()
        {
            return _block;
        }

        void accept(basic_ast_visitor *sp, int level)
        {
            sp->do_function_type_node(this, level);
        }
    };
}

#endif