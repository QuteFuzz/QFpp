#include <info.h>
#include <node_gen.h>
#include <ast_utils.h>

Info::Info(const Ast_entry& entry) :
    qubit_ops(std::move(entry.get_qubit_ops(false)))
{
    # if 0
    // programs with all barrier ops
    if (qubit_ops.size() == 0) {
		WARNING("Ciruit has no `QUBIT_OP` nodes");

	}
    #endif

    feature_vecs = {
        Feature("inverse_pair_density", interesting_pair_count(is_inverse_pair), 20),
        Feature("commuatative_pair_density", interesting_pair_count(is_commutative_pair), 20),
    };

    for (auto& f : feature_vecs){
        archive_size *= f.effective_num_bins(); // extra bin for stuff that doesn't fit into any bin
    }
}


unsigned int Info::get_archive_size() const {
    return archive_size;
}

void Info::dump_feature_vecs(std::ostream& stream){
    stream << "[";

    for (size_t i = 0; i < feature_vecs.size(); i++){
        auto f = feature_vecs[i];
        stream << "{\"name\": \"" << f.name << "\", \"idx\": " << f.idx() << ", \"n_bins\": " << f.num_bins << ", \"bin_width\": " << f.bin_width << "}";
        
        if (i < feature_vecs.size() - 1){
            stream << ",";
        }

        stream << "\n";
    }

    stream << "]";
}

unsigned int Info::get_archive_index() {
    unsigned int index = 0;
    unsigned int stride = 1;

    for (int j = (int)feature_vecs.size() - 1; j >= 0; j--){
        index += feature_vecs[j].idx() * stride;
        stride *= feature_vecs[j].effective_num_bins();
    }
    
    return index;
}

unsigned int Info::interesting_pair_count(std::function<bool(Token_kind, Token_kind)> func){
    int n_sequences = 0;

    if (qubit_ops.size() < 2) return 0;

    // qubit name -> last qubit op acting on that qubit
    std::unordered_map<std::string, std::shared_ptr<Qubit_op>> last_qubit_op_map;

    for (const auto& qubit_op : qubit_ops){
        if (qubit_op->is_subroutine_op()) continue;
    
        n_sequences += qubit_op_is_interesting(qubit_op, func, last_qubit_op_map).first;
    }

    return n_sequences;
}

float Info::quality(){
    /// maximise multi qubit gates, higher entaglement => more routing stress for compiler and swap insertions
    int n_multi_qubit_gates = 0;

    if (qubit_ops.size() == 0) return 0.0;

    for (const auto& qubit_op : qubit_ops){
        n_multi_qubit_gates += qubit_op->get_gate_node()->get_num_external_resources(Resource_kind::QUBIT);
    }

    return (float)n_multi_qubit_gates / (float)qubit_ops.size();
}
