#ifndef FEATURES_H
#define FEATURES_H

#include <utils.h>
#include <gate.h>
#include <node_gen.h>


std::vector<std::shared_ptr<Gate>> get_gates(std::shared_ptr<Node> root){
    std::vector<std::shared_ptr<Gate>> gates;

	for(std::shared_ptr<Node>& node : Node_gen(*root, GATE_NAME)){
		auto gate = std::dynamic_pointer_cast<Gate>(node->child_at(0));
        if (gate != nullptr){
            gates.push_back(gate);
        }
	}

	for(std::shared_ptr<Node>& node : Node_gen(*root, SUBROUTINE)){
		auto gate = std::dynamic_pointer_cast<Gate>(node);
        if (gate != nullptr) {
            gates.push_back(gate);
        }
	}

    return gates;
}

struct Feature_vec {

    public:
        Feature_vec(){}

        Feature_vec(std::shared_ptr<Node> root):
            gates(get_gates(root)),
            n_gates(gates.size())
        {
            for(auto& gate : gates){
                Token_kind kind = gate->get_node_kind();
                if (gate_occurances.find(kind) == gate_occurances.end()){
                    gate_occurances[kind] = 1;
                } else {
                    gate_occurances[kind] += 1;
                }
            }

            features = {
                {"gate_arity_variance", gate_arity_variance()},
                {"gate_type_entropy", gate_type_entropy()},
                {"adj_gate_pair_density", adj_gate_pair_density()},
            };
        }

        float gate_arity_variance(){
            std::vector<unsigned int> gate_arities;
            unsigned int sum = 0;

            for (const auto& gate : gates){
                unsigned int n_qubits = gate->get_num_external_qubits();
                gate_arities.push_back(n_qubits);
                sum += n_qubits;
            }

            float n_arities = (float)gate_arities.size();
            float mean = (float)sum / n_arities;
            float diff_sq = 0.0;

            for(unsigned int gate_arity : gate_arities){
                diff_sq += std::pow(((float)gate_arity - mean), 2.0);
            }

            float variance = diff_sq / n_arities;

            return variance;
        }

        float gate_type_entropy(){
            float neg_shannon_entropy = 0.0;

            for(auto&[kind, count] : gate_occurances){
                float frac = count / (float)n_gates;
                neg_shannon_entropy += frac * std::log2(frac);
            }

            float shannon_entropy = -neg_shannon_entropy;

            return shannon_entropy;
        }

        float adj_gate_pair_density(){
            unsigned int adj_gate_pairs = 0;

            for(size_t i = 0; i < n_gates - 1; i++){
                if(*gates[i] == *gates[i+1]){
                    adj_gate_pairs += 1;
                }
            }

            return (float)adj_gate_pairs / (float)(n_gates - 1);
        }

        std::vector<float> feature_vector(){
            std::vector<float> out;

            for (auto&[name, val] : features){
                out.push_back(val);
            }

            return out;
        }

        friend std::ostream& operator<<(std::ostream& stream, Feature_vec& fv){
            for (auto&[name, val] : fv.features){
                stream << name << " " << val << std::endl;
            }

            return stream;
        }

    private:
        std::vector<std::shared_ptr<Gate>> gates;
        std::unordered_map<Token_kind, unsigned int> gate_occurances;
        unsigned int n_gates;

        std::unordered_map<std::string, float> features;

};



#endif
