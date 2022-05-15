#ifndef __L22_MAIN_PROGRAM_NODE_H__
#define __L22_MAIN_PROGRAM_NODE_H__

#include <cdk/ast/basic_node.h>
#include <cdk/ast/sequence_node.h>

#include "block_node.h"

namespace l22
{

    /**
     * Class for describing main program nodes.
     */
    class main_program_node : public cdk::basic_node
    {
        block_node *_block;

    public:
        main_program_node(int lineno, block_node *block) : cdk::basic_node(lineno), _block(block)
        {
        }

    public:
        block_node *block()
        {
            return _block;
        }

        void accept(basic_ast_visitor *sp, int level)
        {
            sp->do_main_program_node(this, level);
        }
    };

} // l22

#endif
