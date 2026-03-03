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

unsigned int max_control_flow_depth_rec(const std::shared_ptr<Node> node, unsigned int current_depth = 0);

struct Feature {
    std::string name;
    unsigned int val;
    unsigned int num_bins;
    unsigned int bin_width = 1;
};

struct Feature_vec {

    public:
        Feature_vec(const Slot_type compilation_unit);

        unsigned int num_immediate_compound_stmts();

        unsigned int has_multi_qubit_gate();

        unsigned int get_archive_size();

        unsigned int get_archive_index();

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
        Slot_type compilation_unit = nullptr;
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

        unsigned int has_mixed_body();

        unsigned int max_control_flow_depth();

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
        Slot_type compilation_unit;
        std::vector<std::shared_ptr<Gate>> gates;
        unsigned int n_gates;
        std::unordered_map<Token_kind, unsigned int> gate_occurances;

        std::vector<Component> components;
};


#endif
