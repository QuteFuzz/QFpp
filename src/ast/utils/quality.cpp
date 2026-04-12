#include <info.h>
#include <ast_utils.h>

Quality::Quality(Slot_type _compilation_unit, const Features& fv):
    Info(_compilation_unit, std::make_shared<Features>(fv))
{
    components = {
        Component{"clifford_interleaving_score", clifford_interleaving_score()},
        Component{"qubit_interaction_density", qubit_interaction_density()},
    };
}

float Quality::qubit_interaction_density() {
    if (qubit_ops.size() == 0) return 0.0;

    std::set<std::pair<std::string, std::string>> interaction_edges;
    std::set<std::string> active_qubits;

    for (size_t i = 0; i < qubit_ops.size(); i++) {
        std::shared_ptr<Gate> gate = gates[i]; 
                
        unsigned int n_ext_qubits = gate->get_num_external_qubits();
        std::vector<std::string> target_qubits = qubit_ops[i]->get_target_qubit_names();

        if (target_qubits.size() != n_ext_qubits) {
            std::cout << "Qubit op: ";
            qubit_ops[i]->print_program(std::cout);
            std::cout << std::endl;

            ERROR("Collected " + std::to_string(target_qubits.size()) + " target qubits from gate, expected " + std::to_string(n_ext_qubits));        
        }
        
        for (const std::string& q : target_qubits) active_qubits.insert(q);

        if (n_ext_qubits >= 2) {
            for (size_t i = 0; i < n_ext_qubits; i++) {
                for (size_t j = i + 1; j < n_ext_qubits; j++) {
                    std::string q1 = std::min(target_qubits[i], target_qubits[j]);
                    std::string q2 = std::max(target_qubits[i], target_qubits[j]);
                    interaction_edges.insert({q1, q2});
                }
            }
        }
    }

    if (active_qubits.size() <= 1) return 0.0;

    float possible_edges = (active_qubits.size() * (active_qubits.size() - 1)) / 2.0;
    return (float)interaction_edges.size() / possible_edges;
}

float Quality::clifford_interleaving_score() {
    unsigned int interleaved_count = 0;

    if (qubit_ops.size() == 0) return 0.0;
    
    // map to track whether the last gate applied to a particular qubit was clifford
    std::unordered_map<std::string, bool> last_gate_was_clifford;

    for (size_t i = 0; i < qubit_ops.size(); i++){
        std::shared_ptr<Gate> gate = gates[i];

        Token_kind kind = gate->get_node_kind();
        bool is_clifford = (kind == H || kind == S || kind == CX);
        
        for (const std::string& q : qubit_ops[i]->get_target_qubit_names()) {
            if (last_gate_was_clifford.find(q) != last_gate_was_clifford.end()) {

                // state flipped on this qubit (last gate was clifford and this gate isn't and vice versa) 
                if (last_gate_was_clifford[q] != is_clifford) {
                    interleaved_count++;
                }
            }

            last_gate_was_clifford[q] = is_clifford;
        }
    }

    return (float)interleaved_count / (float)qubit_ops.size();
}
