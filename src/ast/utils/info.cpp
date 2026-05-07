#include <info.h>
#include <node_gen.h>
#include <ast_utils.h>

Info::Info(const Ast_entry& entry) :
    qubit_ops(std::move(entry.get_qubit_ops()))
{
    # if 0
    // programs with all barrier ops
    if (qubit_ops.size() == 0) {
		WARNING("Ciruit has no `QUBIT_OP` nodes");

	}
    #endif

    /*
        ratios are used to ensure that the archive size is constant for any program generated. For example, `stmt_ratio`
        is calculated, then multiplied by the chosen number of bins to get the bin index, instead of using the number of statements 
        in the compilation unit as this is not a constant value. Otherwise, known constants like `NESTED_MAX_DEPTH` can be used to
        denote the number of bins
    */
    feature_vecs = {
        Feature("inverse_pair_density", interesting_pair_density(is_inverse_pair), 5),
        Feature("commuatative_pair_density", interesting_pair_density(is_commutative_pair), 5),
    };

    for (auto& f : feature_vecs){
        archive_size *= f.effective_num_bins(); // extra bin for stuff that doesn't fit into any bin
    }
}


unsigned int Info::get_archive_size() const {
    return archive_size;
}

void Info::dump_feature_vecs(std::ofstream& stream){
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

float Info::interesting_pair_density(std::function<bool(Token_kind, Token_kind)> func){
    int n_sequences = 0;

    if (qubit_ops.size() < 2) return 0.0;

    float n_pairs = (float)(qubit_ops.size() * (qubit_ops.size() - 1)) / 2.0;

    // qubit name -> last qubit op acting on that qubit
    std::unordered_map<std::string, std::shared_ptr<Gate>> last_gate_map;

    for (const auto& qubit_op : qubit_ops){
        if (qubit_op->is_subroutine_op()) continue;
    
        n_sequences += qubit_op_is_interesting(qubit_op, func, last_gate_map);
    }

    return (float)n_sequences / n_pairs;
}

float Info::quality(){
    /// maximise multi qubit gates, higher entaglement => more routing stress for compiler and swap insertions
    int n_multi_qubit_gates = 0;

    for (const auto& qubit_op : qubit_ops){
        n_multi_qubit_gates += qubit_op->get_gate_node()->get_num_external_qubits();
    }

    return (float)n_multi_qubit_gates / (float)qubit_ops.size();
}
