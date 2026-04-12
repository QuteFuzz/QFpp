#ifndef GATE_NAME_H
#define GATE_NAME_H

#include <node.h>
#include <coll.h>
#include <clone_mixin.h>

class Gate_name : public Cloneable<Gate_name> {

    public:
        Gate_name(const std::shared_ptr<Circuit> current_circuit) :
            Cloneable<Gate_name>("gate_name", GATE_NAME)
        {
            unsigned int n_qubits = current_circuit->get_coll<Resource>(Resource_kind::QUBIT).size();
            unsigned int n_bits = current_circuit->get_coll<Resource>(Resource_kind::BIT).size();

            for (auto info : SUPPORTED_GATES){
                if((info.n_qubits > n_qubits) || (info.n_bits > n_bits)){
                    add_branch_constraint(info.gate, 0);
                }
            }
        }

    private:

};

#endif
