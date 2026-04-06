#include <gate.h>
#include <resource_def.h>
#include "assert.h"
#include <coll.h>

// Gate::Gate(const std::string& str, std::shared_ptr<Gate_info> _info) :
//     Cloneable<Gate>(str, _info->gate),
//     info(*_info)
// {}


Gate::Gate(const std::string& str, const Token_kind& kind) :
    Cloneable<Gate>(str, kind)
{
    std::shared_ptr<Gate_info> info_ptr = find_gate_info(kind);

    if (info_ptr == nullptr){
        info.gate = kind;
        info.n_qubits = random_uint(QuteFuzz::MAX_REG_SIZE, 1);
        WARNING("Gate " + str + " not supported in QuteFuzz, assigning " + std::to_string(info.n_qubits) + " qubits");
    } else {
        info = *info_ptr;
    }
}

Gate::Gate(const std::string& str, const Token_kind& kind, const Ptr_coll<Resource_def>& _resource_defs) :
    Cloneable<Gate>(str, kind),
    resource_defs(_resource_defs)
{
    assert(kind == SUBROUTINE);

    info.gate = SUBROUTINE;
    info.n_qubits = 0;  // reset to 0 before counting
    info.n_bits = 0;  // reset to 0 before counting

    for(const auto& def : resource_defs){
        if(scope_matches(def->get_scope(), Scope::EXT)){
            if(def->get_resource_kind() == Resource_kind::QUBIT){
                info.n_qubits += def->get_size()->get_num();
            } else {
                info.n_bits += def->get_size()->get_num();
            }
        }
    }

    #if 0
    std::cout << "Subroutine " << str << " has " << info.n_qubits << " external qubits" << std::endl;
    std::cout << "Subroutine " << str << " has " << info.n_bits << " external bits" << std::endl;
    #endif
}

unsigned int Gate::get_num_external_resource_defs(Resource_kind kind) const {
    auto pred = [&](const auto& elem){return scope_matches(elem->get_scope(), Scope::EXT) && (elem->get_resource_kind() == kind);};
    return size_pred<Resource_def>(resource_defs, pred);
}
