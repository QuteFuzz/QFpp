#ifndef GATE_NAME_H
#define GATE_NAME_H

#include <node.h>

class Gate_name : public Node {

    public:
        using Node::Node;

        Gate_name(const std::shared_ptr<Node> parent, const std::shared_ptr<Circuit> current_circuit, const std::optional<Node_constraint>& swarm_testing_gateset) :
            Node("gate_name", GATE_NAME, swarm_testing_gateset)
        {

            if(current_circuit->num_qubits_of(OWNED_SCOPE) == 0){
                add_constraint(MEASURE_AND_RESET, 0);

                if (current_circuit->num_bits_of(ALL_SCOPES) == 0) {
                    add_constraint(MEASURE, 0);
                }
                /*
                    measure_and_reset only needs owned qubits, and guppy doesn't have bit resources
                */
            }

            if(*parent == SUBROUTINE_OP){
                add_constraint(SUBROUTINE, 1);

            } else if (*parent == GATE_OP) {
                add_constraint(SUBROUTINE, 0);

            } else {
                ERROR("Gate name expected parent to be subroutine_op or gate_op!");
            }

        }

    private:

};

#endif
