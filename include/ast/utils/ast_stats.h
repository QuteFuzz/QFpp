#ifndef STATS_H
#define STATS_H

#include <utils.h>
#include <gate.h>
#include <node_gen.h>

/*
    lil timmy tim's favorite subject

    quality measures relate to how well the circuit would stress test the compiler. All are static measures, using 
    components that interact with the compiler

    features are structural descriptors of the AST which effectively describe different *classes* of programs
*/

std::vector<std::shared_ptr<Gate>> get_gates(Node& ast);

struct Feature {
    std::string name;
    unsigned int num_bins;
    unsigned int val = 0;
};

struct Feature_vec {

    public:
        Feature_vec(Node& _ast):
            ast(_ast)
        {
            vec = {
                Feature{"max_control_flow_depth", QuteFuzz::NESTED_MAX_DEPTH, max_control_flow_depth()},
                Feature{"has_subroutines", 2, has_subroutines()},
            };
        }

        unsigned int max_control_flow_depth(){
            unsigned int max_depth = 0;
            return max_depth;
        }

        unsigned int has_subroutines(){
            return ast.find(SUBROUTINE_DEFS) != nullptr;
        }

        unsigned int archive_size(){
            unsigned int s = 1;

            for (auto& f : vec){
                s *= f.num_bins;
            }

            return s;
        }

    private:
        Node& ast;
        std::vector<Feature> vec;

};

struct Quality {

    public:
        struct Component {
            std::string name;
            float val;
            float weight = 1.0;
        };

        Quality(){}

        Quality(Node& root):
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

            components = {
                Component{"gate_arity_variance", gate_arity_variance()},
                Component{"gate_type_entropy", gate_type_entropy()},
                Component{"adj_gate_pair_density", adj_gate_pair_density()},
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

        float quality(){
            float q;

            for (auto& c : components){
                q += (c.val * c.weight);
            }

            return q;
        }

        friend std::ostream& operator<<(std::ostream& stream, Quality& q){
            for (auto& c : q.components){
                stream << c.name << " " << c.val << " w: " << c.weight << std::endl;
            }

            return stream;
        }

    private:
        std::vector<std::shared_ptr<Gate>> gates;
        std::unordered_map<Token_kind, unsigned int> gate_occurances;
        unsigned int n_gates;

        std::vector<Component> components;

};


#endif
