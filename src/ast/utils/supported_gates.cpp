#include <supported_gates.h>

std::shared_ptr<Gate_info> find_gate_info(const Token_kind& gate_kind) {
    for (const auto& _info : SUPPORTED_GATES){
        if(_info.gate == gate_kind){
            return std::make_shared<Gate_info>(_info);
        }
    }

    return nullptr;
}
