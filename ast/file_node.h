#ifndef __L22_FILE_NODE_H__
#define __L22_FILE_NODE_H__

#include <cdk/ast/basic_node.h>
#include <cdk/ast/sequence_node.h>

#include "program_node.h"

namespace l22
{

    /**
     * Class for describing file nodes.
     */
    class file_node : public cdk::basic_node
    {
        cdk::sequence_node *_declarations;
        l22::program_node *_program;

    public:
        file_node(int lineno, cdk::sequence_node *declarations, l22::program_node *program) : cdk::basic_node(lineno), _declarations(declarations), _program(program)
        {
        }

    public:
        cdk::sequence_node *declarations()
        {
            return _declarations;
        }
        l22::program_node *main_program()
        {
            return _program;
        }

        void accept(basic_ast_visitor *sp, int level)
        {
            sp->do_file_node(this, level);
        }
    };

} // l22

#endif
