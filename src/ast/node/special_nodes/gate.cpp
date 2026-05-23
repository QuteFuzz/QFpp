#include <gate.h>
#include <resource_def.h>
#include "assert.h"
#include <coll.h>
#include <variable.h>

Gate::Gate(const std::string& str, const Token_kind& kind) :
    Cloneable<Gate>(str, kind)
{
    std::shared_ptr<Gate_info> info_ptr = find_gate_info(kind);

    if (info_ptr == nullptr){
        info.gate_source = kind;
        info.resource_counts[Resource_kind::QUBIT] = uniform_uint(QuteFuzz::MAX_REG_SIZE, 1);
        WARNING("Gate " + str + " not supported in QuteFuzz, assigning " + std::to_string(info.resource_counts[Resource_kind::QUBIT]) + " qubits");
    } else {
        info = *info_ptr;
    }
}

Gate::Gate(const std::string& str, const Ptr_coll<Resource_def>& _resource_defs) :
    Cloneable<Gate>(str, SUBROUTINE_OP),
    resource_defs(_resource_defs)
{
    info.gate_source = SUB_CIRCUIT;

    // reset to 0 before counting
    for (auto& pair : info.resource_counts) {
        pair.second = 0;
    }

    for(const auto& def : resource_defs){
        if(scope_matches(def->get_scope(), Scope::EXT)){
            info.resource_counts[def->get_resource_kind()] += def->get_size()->get_num();
        }
    }
}

Gate::Gate(const std::string& str, unsigned int n_matrix_qubits) :
    Cloneable<Gate>(str, SUBROUTINE_OP)
{
    info.gate_source = n_matrix_qubits == 1 ? UNITARY_1Q_DEF : UNITARY_2Q_DEF;
    info.resource_counts[Resource_kind::QUBIT] = n_matrix_qubits;
}

std::shared_ptr<Variable> Gate::get_var_name() const { return std::make_shared<Variable>(str);}

/// Info filters for external scope
unsigned int Gate::get_num_external_resources(Resource_kind rk) const {return info.resource_counts.at(rk);}

unsigned int Gate::get_num_external_resource_defs(Resource_kind kind) const {
    auto pred = [&](const auto& elem){return scope_matches(elem->get_scope(), Scope::EXT) && (elem->get_resource_kind() == kind);};
    return size_pred<Resource_def>(resource_defs, pred);
}

Token_kind Gate::get_gate_source() const {
    return info.gate_source;
}
