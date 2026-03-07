#ifndef INFO_H
#define INFO_H

#include <utils.h>
#include <gate.h>
#include <node_gen.h>

/*
    quality measures relate to how well the circuit would stress test the compiler. All are static measures, using 
    components that interact with the compiler

    features are structural descriptors of the AST which effectively describe different *classes* of programs
*/

class Info {

    public:
        Info(Slot_type _compilation_unit);

    protected:
        Slot_type compilation_unit = nullptr;
        std::vector<std::shared_ptr<Gate>> gates;
        unsigned int n_gates;
};

unsigned int max_control_flow_depth_rec(const std::shared_ptr<Node> node, unsigned int current_depth = 0);


class Features : public Info {

    public:
        struct Feature {
            std::string name;
            unsigned int val;
            unsigned int num_bins = 2;  // binary features by default
            unsigned int bin_width = 1;
        };

        Features(Slot_type compilation_unit);

        unsigned int has_control_flow();

        unsigned int has_subroutine_call();

        unsigned int has_barrier();

        float multi_qubit_gate_ratio();

        unsigned int has_parametrised();

        unsigned int get_archive_size() const;

        unsigned int get_archive_index() const;

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

        friend std::ostream& operator<<(std::ostream& stream, Features& fv){
            for (auto& f : fv.vec){
                stream << f.name << " " << f.val << " n_bins: " << f.num_bins << " bin_width: " << f.bin_width << std::endl;
            }

            return stream;
        }

    private:
        std::vector<Feature> vec;
        unsigned int archive_size = 1;
};


class Quality : public Info {

    public:
        struct Component {
            std::string name;
            float val;
            float weight = 1.0;
        };

        Quality(Slot_type _compilation_unit);

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
        std::unordered_map<Token_kind, unsigned int> gate_occurances;
        std::vector<Component> components;
};


#endif
