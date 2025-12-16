#ifndef SUBROUTINE_OP_ARGS_H
#define SUBROUTINE_OP_ARGS_H

#include <node.h>

class Subroutine_op_args : public Node {

    public:

        Subroutine_op_args(unsigned int num_arguments):
            Node("subroutine_op_args", SUBROUTINE_OP_ARGS)
        {
            add_constraint(SUBROUTINE_OP_ARG, num_arguments);
        }

    private:

};

#endif
