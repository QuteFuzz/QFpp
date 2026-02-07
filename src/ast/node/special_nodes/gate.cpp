#include <gate.h>
#include <resource_def.h>
#include "assert.h"
#include <coll.h>

Gate::Gate(const std::string& str, const Token_kind& kind) :
    Node(str, kind)
{
    bool gate_found = false;

    for (auto _info : SUPPORTED_GATES){
        if(_info.gate == kind){
            info = _info;
            gate_found = true;
            break;
        }
    }

    if (!gate_found){
        ERROR("Gate " + str + " not supported in QuteFuzz");
    }
}

Gate::Gate(const std::string& str, const Token_kind& kind, const Ptr_coll<Resource_def>& _qubit_defs) :
    Node(str, kind),
    qubit_defs(_qubit_defs)
{
    assert(kind == SUBROUTINE);

    info.gate = SUBROUTINE;
    info.n_qubits = 0;  // reset to 0 before counting

    // count number of external qubits depending on size of qubit defs
    for(const auto& qubit_def : qubit_defs){
        if(scope_matches(qubit_def->get_scope(), Scope::EXT)){
            info.n_qubits += qubit_def->get_size()->get_num();
        }
    }

    // std::cout << "Subroutine " << str << " has " << info.n_qubits << " external qubits" << std::endl;
}

unsigned int Gate::get_num_external_qubit_defs() const {
    auto pred = [](const auto& elem){return scope_matches(elem->get_scope(), Scope::EXT);};
    return size_pred<Resource_def>(qubit_defs, pred);
}
