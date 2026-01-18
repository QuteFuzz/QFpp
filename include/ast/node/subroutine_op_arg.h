#ifndef SUBROUTINE_OP_ARG_H
#define SUBROUTINE_OP_ARG_H

#include <node.h>

class Qubit_definition;

class Subroutine_op_arg : public Node {
    public:
        Subroutine_op_arg();

        Subroutine_op_arg(const std::shared_ptr<Qubit_definition>& qubit_def);

    private:
};

#endif
