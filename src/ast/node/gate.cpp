#include <gate.h>
#include <resource_definition.h>
#include "assert.h"

Gate::Gate(const std::string& str, const Token::Kind& kind, unsigned int qubits, unsigned int bits, unsigned int floats) :
    Node(str, kind),
    num_external_qubits(qubits),
    num_external_bits(bits),
    num_floats(floats)
{}

Gate::Gate(const std::string& str, const Token::Kind& kind, const Collection<Qubit_definition>& qubit_defs) :
    Node(str, kind)
{
    assert(kind == Token::SUBROUTINE);

    // filter out external qubit defs
    for(const auto& qubit_def : qubit_defs){
        if(qubit_def->get_scope() & EXTERNAL_SCOPE){
            external_qubit_defs.add(*qubit_def);
            num_external_qubits += qubit_def->get_size()->get_num();
        }
    }
}

std::shared_ptr<Qubit_definition> Gate::get_next_qubit_def(){
    current_qubit_def = external_qubit_defs.at(qubit_def_pointer++ % external_qubit_defs.size());
    return current_qubit_def;
}

std::shared_ptr<Qubit_definition> Gate::get_current_qubit_def(){
    return current_qubit_def;
}

unsigned int Gate::get_num_external_qubits(){
    return num_external_qubits;
}