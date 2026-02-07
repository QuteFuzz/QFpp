#ifndef SUPPORTED_GATES_H
#define SUPPORTED_GATES_H

#include <lex.h>
#include <coll.h>

struct Gate_info {
    Token_kind gate;
    unsigned int n_qubits = 0;
    unsigned int n_bits = 0;
    unsigned int n_floats = 0;

    Gate_info(){}

    Gate_info(Token_kind _gate, unsigned int _n_qubits, unsigned int _n_bits = 0, unsigned int _n_floats = 0):
        gate(_gate),
        n_qubits(_n_qubits),
        n_bits(_n_bits),
        n_floats(_n_floats)
    {}
};

const std::vector<Gate_info> SUPPORTED_GATES = []{
    std::vector<Gate_info> gates;

    // lambda helper to add group of gates to the supported gateset list
    auto add_group = [&](const std::vector<Token_kind>& kinds, unsigned int q, unsigned int b, unsigned int f) {
        for (const auto& kind : kinds) {
            // Emplace automatically picks the matching constructor based on arguments
            gates.emplace_back(kind, q, b, f);
        }
    };

    // 1 Qubit, 0 Bits, 0 Floats
    add_group({
        H, X, Y, Z, T, TDG, S, SDG, PROJECT_Z, V, VDG
    }, 1, 0, 0);

    // 2 Qubits, 0 Bits, 0 Floats
    add_group({
        CX, CY, CZ, CNOT, CH, SWAP
    }, 2, 0, 0);

    // 2 Qubits, 0 Bits, 1 Float
    add_group({
        CRZ, CRX, CRY
    }, 2, 0, 1);

    // 3 Qubits, 0 Bits, 0 Floats
    add_group({
        CCX, CSWAP, TOFFOLI
    }, 3, 0, 0);

    // 1 Qubit, 0 Bits, 1 Float
    add_group({
        U1, RX, RY, RZ
    }, 1, 0, 1);

    // 1 Qubit, 0 Bits, 2 Floats
    add_group({
        U2, PHASED_X
    }, 1, 0, 2);

    // 1 Qubit, 0 Bits, 3 Floats
    add_group({
        U3, U
    }, 1, 0, 3);

    // 1 Qubit, 1 Bit, 0 Floats
    add_group({
        MEASURE
    }, 1, 1, 0);

    return gates;
}();

#endif
