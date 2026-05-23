#include <supported_gates.h>
#include <gate.h>

Gate_info::Gate_info(Token_kind _gate, unsigned int _n_qubits, unsigned int _n_bits, unsigned int _n_params):
    gate_source(_gate),
    resource_counts({
        {Resource_kind::QUBIT, _n_qubits},
        {Resource_kind::BIT, _n_bits},
        {Resource_kind::PARAM, _n_params},
    })
{}

const std::vector<Gate_info> SUPPORTED_GATES = []{
    std::vector<Gate_info> gates;

    // lambda helper to add group of gates to the supported gateset list
    auto add_group = [&](const std::vector<Token_kind>& kinds, unsigned int q, unsigned int b, unsigned int p) {
        for (const auto& kind : kinds) {
            // Emplace automatically picks the matching constructor based on arguments
            gates.emplace_back(kind, q, b, p);
        }
    };

    add_group({
        H, X, Y, Z, T, TDG, S, SDG, PROJECT_Z, V, VDG
    }, 1, 0, 0);

    add_group({
        CX, CY, CZ, CNOT, CH, SWAP, XX, YY, ZZ
    }, 2, 0, 0);

    add_group({
        CRZ, CRX, CRY
    }, 2, 0, 1);

    add_group({
        CCX, CSWAP, TOFFOLI
    }, 3, 0, 0);

    add_group({
        U1, RX, RY, RZ
    }, 1, 0, 1);

    add_group({
        U2, PHASED_X
    }, 1, 0, 2);

    add_group({
        U3, U
    }, 1, 0, 3);

    add_group({
        MEASURE
    }, 1, 1, 0);

    return gates;
}();

std::shared_ptr<Gate_info> find_gate_info(const Token_kind& gate_kind) {
    for (const auto& _info : SUPPORTED_GATES){
        if(_info.gate_source == gate_kind){
            return std::make_shared<Gate_info>(_info);
        }
    }

    return nullptr;
}

Token_kind find_gate_type_with_same_arity(const std::shared_ptr<Gate> gate) {
    for (const auto& _info : SUPPORTED_GATES){
        if ((_info.gate_source != gate->get_node_kind()) && (_info.resource_counts.at(Resource_kind::QUBIT) == gate->get_num_external_resources(Resource_kind::QUBIT))) {
            return _info.gate_source;
        }
    }

    return gate->get_node_kind();
}
