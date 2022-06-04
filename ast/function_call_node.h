#ifndef __L22_AST_FUNCTION_CALL_NODE_H__
#define __L22_AST_FUNCTION_CALL_NODE_H__

#include <string>
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
        std::string _identifier;
        cdk::expression_node *_fptr;
        cdk::sequence_node *_arguments;

    public:
        function_call_node(int lineno, const std::string &identifier)
            : cdk::expression_node(lineno), _identifier(identifier), _fptr(nullptr), _arguments(new cdk::sequence_node(lineno))
        {
        }

        function_call_node(int lineno, const std::string &identifier, cdk::sequence_node *arguments)
            : cdk::expression_node(lineno), _identifier(identifier), _fptr(nullptr), _arguments(arguments)
        {
        }

        function_call_node(int lineno, cdk::expression_node *fptr)
            : cdk::expression_node(lineno), _identifier(""), _fptr(fptr), _arguments(new cdk::sequence_node(lineno))
        {
        }

        function_call_node(int lineno, cdk::expression_node *fptr, cdk::sequence_node *arguments)
            : cdk::expression_node(lineno), _identifier(""), _fptr(fptr), _arguments(arguments)
        {
        }

    public:
        inline std::string &identifier()
        {
            return _identifier;
        }

        cdk::basic_node *fptr()
        {
            return _fptr;
        }

        inline cdk::sequence_node *arguments()
        {
            return _arguments;
        }

        void accept(basic_ast_visitor *sp, int level)
        {
            sp->do_function_call_node(this, level);
        }
    };

} // l22

#endif
