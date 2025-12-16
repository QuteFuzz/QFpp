#ifndef SUBROUTINE_DEFS_H
#define SUBROUTINE_DEFS_H

#include <node.h>

class Subroutine_defs : public Node {

    public:

        Subroutine_defs(unsigned int n_blocks):
            Node("subroutine defs", SUBROUTINE_DEFS)
        {
            add_constraint(BLOCK, n_blocks);
        }

    private:

};

#endif
