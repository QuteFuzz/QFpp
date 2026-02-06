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

    // count number of external qubits depending on size of qubit defs
    for(const auto& qubit_def : qubit_defs){
        if(scope_matches(qubit_def->get_scope(), Scope::EXT)){
            info.n_qubits += qubit_def->get_size()->get_num();
        }
    }
}

std::shared_ptr<Resource_def> Gate::get_next_qubit_def(){
    Ptr_pred_type<Resource_def> pred = [](const auto& elem){return scope_matches(elem->get_scope(), Scope::EXT) && !elem->is_used();};
    last_qubit_def = get_next_from_coll(qubit_defs, pred);
    last_qubit_def->set_used();

    return last_qubit_def;
}

std::shared_ptr<Resource_def> Gate::get_last_qubit_def() const {
    return last_qubit_def;
}

unsigned int Gate::get_num_external_qubit_defs() const {
    auto pred = [](const auto& elem){return scope_matches(elem->get_scope(), Scope::EXT);};
    return filter<Resource_def>(qubit_defs, pred).size();
}
