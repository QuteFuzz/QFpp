#ifndef GATE_OP_ARGS_H
#define GATE_OP_ARGS_H

#include <node.h>
#include <gate.h>

class Gate_op_args : public Node {

    public:

        Gate_op_args(const std::shared_ptr<Gate> current_gate):
            Node("gate_op_args", GATE_OP_ARGS)
        {

            if(*current_gate == SUBROUTINE){
                ERROR("Gate op args cannot be used on subroutine!");
            
            } else {
                add_constraint(FLOAT_LIST, current_gate->get_num_floats() > 0);
                add_constraint(BIT_LIST, current_gate->get_num_external_bits() > 0);
            }

        }

    private:

};

#endif
