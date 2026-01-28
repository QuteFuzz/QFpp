#include <gate.h>
#include <resource_definition.h>
#include "assert.h"
#include <coll.h>

Gate::Gate(const std::string& str, const Token_kind& kind, unsigned int qubits, unsigned int bits, unsigned int floats) :
    Node(str, kind),
    num_external_qubits(qubits),
    num_external_bits(bits),
    num_floats(floats)
{}

Gate::Gate(const std::string& str, const Token_kind& kind, const Ptr_coll<Qubit_definition>& _qubit_defs) :
    Node(str, kind),
    qubit_defs(_qubit_defs)
{
    assert(kind == SUBROUTINE);

    // count number of external qubits depending on size of qubit defs
    for(const auto& qubit_def : qubit_defs){
        if(scope_matches(qubit_def->get_scope(), Scope::EXT)){
            num_external_qubits += qubit_def->get_size()->get_num();
        }
    }
}

std::shared_ptr<Qubit_definition> Gate::get_next_qubit_def(){
    last_qubit_def = get_next_from_coll(qubit_defs, Scope::EXT);
    return last_qubit_def;
}

std::shared_ptr<Qubit_definition> Gate::get_last_qubit_def(){
    return last_qubit_def;
}

unsigned int Gate::get_num_external_qubits(){
    return num_external_qubits;
}

unsigned int Gate::get_num_external_qubit_defs() const {
    auto pred = [](const auto& elem){return scope_matches(elem->get_scope(), Scope::EXT);};
    return coll_size<Qubit_definition>(qubit_defs, pred);
}
