#ifndef PRIMITIVE_GATE_H
#define PRIMITIVE_GATE_H

#include <node.h>
#include <coll.h>
#include <clone_mixin.h>
#include <supported_gates.h>

class Primitive_gate : public Cloneable<Primitive_gate> {

    public:
        Primitive_gate(const std::shared_ptr<Circuit> current_circuit) :
            Cloneable<Primitive_gate>("primitive_gate", PRIMITIVE_GATE)
        {
            unsigned int n_qubits = current_circuit->get_coll<Resource>(Resource_kind::QUBIT).size();
            unsigned int n_bits = current_circuit->get_coll<Resource>(Resource_kind::BIT).size();

            for (auto info : SUPPORTED_GATES){
                if((info.resource_counts.at(Resource_kind::QUBIT) > n_qubits) || (info.resource_counts.at(Resource_kind::BIT) > n_bits)){
                    add_branch_constraint(info.gate_source, 0);
                }
            }
        }

    private:

};

#endif
