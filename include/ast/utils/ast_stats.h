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

std::vector<std::shared_ptr<Gate>> get_gates(const std::shared_ptr<Node> compilation_unit);

unsigned int max_control_flow_depth(const std::shared_ptr<Node> node, unsigned int current_depth = 0);

unsigned int has_mixed_body(const std::shared_ptr<Node> node);

struct Feature {
    std::string name;
    unsigned int val;
    unsigned int num_bins;
    unsigned int bin_width = 1;
};

struct Feature_vec {

    public:
        Feature_vec(const std::shared_ptr<Node> compilation_unit){
            vec = {
                Feature{"max_control_flow_depth", max_control_flow_depth(compilation_unit), QuteFuzz::NESTED_MAX_DEPTH},
                Feature{"has_mixed_body", has_mixed_body(compilation_unit), 2},
            };

            for (auto& f : vec){
                archive_size *= (f.num_bins + 1); // extra bin for stuff that doesn't fit into any bin
            }
        }

        unsigned int get_archive_size(){
            return archive_size;
        }

        /// figure out where in the archive this feature vec falls
        unsigned int get_archive_index(){
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

        auto begin(){return vec.begin();}

        auto end(){return vec.end();}

        auto begin() const {return vec.begin();}

        auto end() const {return vec.end();}

        size_t size() const {return vec.size();}

        Feature& operator[](size_t index) {
            return vec[index];
        }

        const Feature& operator[](size_t index) const {
            return vec[index];
        }

        friend std::ostream& operator<<(std::ostream& stream, Feature_vec& fv){
            for (auto& f : fv.vec){
                stream << f.name << " " << f.val << " n_bins: " << f.num_bins << " bin_width: " << f.bin_width << std::endl;
            }

            return stream;
        }


    private:
        std::shared_ptr<Node> compilation_unit = nullptr;
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

        Quality(Slot_type _compilation_unit);

        Slot_type get_compilation_unit() const { return compilation_unit; }

        float gate_arity_variance();

        float gate_type_entropy();

        float adj_gate_pair_density();

        float quality() const {
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
        Slot_type compilation_unit;

};


#endif
