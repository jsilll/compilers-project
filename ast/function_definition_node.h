#ifndef __L22_AST_FUNCTION_DEFINITION_NODE_H_
#define __L22_AST_FUNCTION_DEFINITION_NODE_H_

#include <cdk/ast/expression_node.h>
#include <cdk/ast/sequence_node.h>

namespace l22
{
    /**
     * Class for describing if-then-else nodes.
     */
    class function_definition_node : public cdk::typed_node
    {
        cdk::sequence_node *_arguments;
        cdk::basic_node *_block;

    public:
        function_definition_node(int lineno,
                                 cdk::sequence_node *arguments,
                                 cdk::basic_node *block,
                                 std::shared_ptr<cdk::basic_type> funType = cdk::primitive_type::create(0, cdk::TYPE_VOID))
            : typed_node(lineno), _arguments(arguments), _block(block)
        {
            type(funType);
        }

    public:
        inline cdk::sequence_node *arguments()
        {
            return _arguments;
        }

        inline cdk::basic_node *block()
        {
            return _block;
        }

        void accept(basic_ast_visitor *sp, int level)
        {
            sp->do_function_definition_node(this, level);
        }
    };
}

#endif