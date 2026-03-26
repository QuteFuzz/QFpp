#include <info.h>

Quality::Quality(Slot_type _compilation_unit, const Features& fv):
    Info(_compilation_unit, std::make_shared<Features>(fv))
{
    for(auto& gate : gates){
        Token_kind kind = gate->get_node_kind();
        if (gate_occurances.find(kind) == gate_occurances.end()){
            gate_occurances[kind] = 1;
        } else {
            gate_occurances[kind] += 1;
        }
    }

    components = {
        Component{"gate_arity_variance", gate_arity_variance()},
        Component{"gate_type_entropy", gate_type_entropy()},
        Component{"adj_gate_pair_density", adj_gate_pair_density()},
        Component{"max_control_flow_depth", (float)max_control_flow_depth()},
    };
}

float Quality::gate_arity_variance(){
    if (n_gates == 0) return 0.0;

    std::vector<unsigned int> gate_arities;
    unsigned int sum = 0;

    for (const auto& gate : gates){
        unsigned int n_qubits = gate->get_num_external_qubits();
        gate_arities.push_back(n_qubits);
        sum += n_qubits;
    }

    float mean = (float)sum / n_gates;
    float variance = 0.0;

    for(unsigned int gate_arity : gate_arities){
        variance += std::pow(((float)gate_arity - mean), 2.0);
    }

    assert(variance >= 0.0);

    return variance / n_gates;
}

float Quality::gate_type_entropy(){
    float neg_shannon_entropy = 0.0;

    if (n_gates > 0){
        for(auto&[kind, count] : gate_occurances){
            float frac = count / (float)n_gates;
            neg_shannon_entropy += frac * std::log2(frac);
        }
    }

    return -neg_shannon_entropy;
}

float Quality::adj_gate_pair_density(){
    if (n_gates >= 2){
        unsigned int adj_gate_pairs = 0;

        for(size_t i = 0; i < n_gates - 1; i++){
            if(*gates[i] == *gates[i+1]){
                adj_gate_pairs += 1;
            }
        }

        return (float)adj_gate_pairs / (float)(n_gates - 1);
    
    } else {
        return 0.0;
    }
}

unsigned int Quality::has_mixed_body(){
    for (auto& compound_stmts : Node_gen(**compilation_unit, COMPOUND_STMTS)){
        bool has_gate = compound_stmts->find(QUBIT_OP) != nullptr;
        bool has_cf   = compound_stmts->find(CF_STMT)  != nullptr;

        if (has_gate && has_cf) return 1;
    }

    return 0;
}

unsigned int Quality::max_control_flow_depth(){
    return max_control_flow_depth_rec(*compilation_unit, 0);
}
