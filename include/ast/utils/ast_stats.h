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
    unsigned int val;
    unsigned int num_bins;
    unsigned int bin_width = 1;
};

struct Feature_vec {

    public:
        Feature_vec(Node& _ast):
            ast(_ast)
        {
            vec = {
                Feature{"max_control_flow_depth", max_control_flow_depth(), QuteFuzz::NESTED_MAX_DEPTH},
                Feature{"has_subroutines", has_subroutines(), 2},
            };

            for (auto& f : vec){
                archive_size *= (f.num_bins + 1); // extra bin for stuff that doesn't fit into any bin
            }
        }

        unsigned int max_control_flow_depth(){
            unsigned int max_depth = 0;
            return max_depth;
        }

        unsigned int has_subroutines(){
            return ast.find(SUBROUTINE_DEFS) != nullptr;
        }

        unsigned int get_archive_size(){
            return archive_size;
        }

        /// figure out where in the archive this feature vec falls
        unsigned int get_archive_index(){
            // figure out which bin index each feature falls into
            std::vector<unsigned int> bins;

            for (auto& feature : vec){
                unsigned int effective_num_bins = feature.num_bins + 1;
                for (unsigned int i = 0; i < effective_num_bins; i++){
                    unsigned int bin_min = i * feature.bin_width;
                    unsigned int bin_max = bin_min + feature.bin_width;
                    
                    if (feature.val >= bin_min && feature.val < bin_max){
                        bins.push_back(i);
                        break;
                    }
                }
            }

            // second pass to find index while accumulating stride
            unsigned int index = 0;
            unsigned int stride = 1;

            for (int j = (int)vec.size() - 1; j >= 0; j--){
                index += bins[j] * stride;
                stride *= (vec[j].num_bins + 1); // +1 to account for additional bin
            }
            
            return index;
        }

        friend std::ostream& operator<<(std::ostream& stream, Feature_vec& fv){
            for (auto& f : fv.vec){
                stream << f.name << " " << f.val << " n_bins: " << f.num_bins << " bin_width: " << f.bin_width << std::endl;
            }

            return stream;
        }


    private:
        Node& ast;
        std::vector<Feature> vec;
        unsigned int archive_size = 1;

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
            float q = 0.0;
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
