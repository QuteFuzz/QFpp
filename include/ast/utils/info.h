#ifndef INFO_H
#define INFO_H

#include <utils.h>
#include <gate.h>
#include <qubit_op.h>
#include <node_gen.h>
#include <unordered_set>
#include <ast.h>


/*
    quality measures relate to how well the circuit would stress test the compiler. All are static measures, using 
    components that interact with the compiler

    features are structural descriptors of the AST which effectively describe different *classes* of programs
*/

struct Feature {

    Feature(std::string _name, unsigned int _raw_idx, unsigned int _num_bins = 2):
        name(_name),
        raw_idx(_raw_idx),
        num_bins(_num_bins),
        is_binary(_num_bins == 2)
    {}
        std::vector<std::shared_ptr<Gate>> gates;

    Feature(std::string _name, float _val, unsigned int _num_bins):
        name(_name),
        raw_idx((unsigned int)(_val * _num_bins)),
        num_bins(_num_bins),
        is_binary(false)
    {
        if((0.0 > _val) || (_val > 1.0)){
            ERROR("Non-binary feature " + name + " is expected to have value as a ratio between 0.0 and 1.0");
        }
    }

    std::string name;
    unsigned int raw_idx;
    unsigned int num_bins;
    bool is_binary;
    unsigned int bin_width = 1;

    inline unsigned int effective_num_bins() const {
        // +1 additional bin for overflows for non-binary features
        /*
            this is done because for binary features, it makes no sense to have an overflow bin,
            because the 3rd bin is always unreachable, so we can never fill those cells in the archive
        */
        return is_binary ? num_bins : num_bins + 1;
    }

    inline unsigned int idx() const {
        return std::min(raw_idx, effective_num_bins() - 1) / bin_width;
    }
};

class Info {

    public:
        Info(){}

        Info(const Ast_entry& entry);

        auto begin(){return feature_vecs.begin();}

        auto end(){return feature_vecs.end();}

        auto begin() const {return feature_vecs.begin();}

        auto end() const {return feature_vecs.end();}

        Feature& operator[](size_t index) { return feature_vecs[index];}

        const Feature& operator[](size_t index) const { return feature_vecs[index];}

        size_t size() const {return feature_vecs.size();}

        unsigned int get_archive_size() const;

        unsigned int get_archive_index();
        
        std::vector<std::shared_ptr<Qubit_op>> get_qubit_ops() const {return qubit_ops;}

        void dump_feature_vecs(std::ostream& stream);

        float quality();

        unsigned int interesting_pair_count(std::function<bool(Token_kind, Token_kind)> func);

        float non_clifford_density();

        float entanglement_density();

        float interaction_graph_diversity();
        
        float reducibles_density();

    private:
        std::vector<std::shared_ptr<Qubit_op>> qubit_ops;

        std::vector<Feature> feature_vecs;
        unsigned int archive_size = 1;

};

unsigned int max_control_flow_depth_rec(const std::shared_ptr<Node> node, unsigned int current_depth = 0);

#endif
