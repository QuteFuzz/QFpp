#ifndef SUPPORTED_GATES_H
#define SUPPORTED_GATES_H

#include <lex.h>
#include <coll.h>
#include <resource_kind.h>

class Gate;

struct Gate_info {
    Token_kind gate;
    std::unordered_map<Resource_kind, unsigned int> resource_counts {
        {Resource_kind::QUBIT, 0},
        {Resource_kind::BIT, 0},
        {Resource_kind::PARAM, 0},
    };

    Gate_info(){}

    Gate_info(Token_kind _gate, unsigned int _n_qubits, unsigned int _n_bits = 0, unsigned int _n_params = 0);
};

extern const std::vector<Gate_info> SUPPORTED_GATES;

std::shared_ptr<Gate_info> find_gate_info(const Token_kind& gate_kind);

Token_kind find_gate_type_with_same_arity(const std::shared_ptr<Gate> gate);

#endif
