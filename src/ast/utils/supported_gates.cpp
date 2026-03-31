#include <supported_gates.h>
#include <gate.h>

std::shared_ptr<Gate_info> find_gate_info(const Token_kind& gate_kind) {
    for (const auto& _info : SUPPORTED_GATES){
        if(_info.gate == gate_kind){
            return std::make_shared<Gate_info>(_info);
        }
    }

    return nullptr;
}

Token_kind find_gate_type_with_same_arity(const std::shared_ptr<Gate> gate) {
    for (const auto& _info : SUPPORTED_GATES){
        if ((_info.gate != gate->get_node_kind()) && (_info.n_qubits == gate->get_num_external_qubits())) {
            return _info.gate;
        }
    }

    return gate->get_node_kind();
}
