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
        Feature("entanglement_density",   entanglement_density(),  10),
        Feature("non_clifford_density",   non_clifford_density(),  10),
        Feature("inverse_pair_count", interesting_pair_count(is_inverse_pair), 20),
        // Feature("commuatative_pair_density", interesting_pair_count(is_commutative_pair), 20),
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

float Info::non_clifford_density(){
    float non_clifford = 0.0f;
    unsigned int n_qubit_ops = qubit_ops.size();
    if (n_qubit_ops > 0) {
        unsigned int nc = 0;
        for (const auto& op : qubit_ops) {
            if (!op->is_subroutine_op() &&
                !gate_in_set(CLIFFORDS, op->get_gate_node()->get_node_kind()))
                nc++;
        }
        non_clifford = (float)nc / (float)n_qubit_ops;
    }

    return non_clifford;
}

float Info::entanglement_density(){
    float entanglement = 0.0f;
    unsigned int n_qubit_ops = qubit_ops.size();

    if (n_qubit_ops > 0) {
        unsigned int multi_qubit = 0;
        for (const auto& op : qubit_ops) {
            if (!op->is_subroutine_op() &&
                op->get_gate_node()->get_num_external_resources(Resource_kind::QUBIT) > 1)
                multi_qubit++;
        }
        entanglement = (float)multi_qubit / (float)n_qubit_ops;
    }

    return entanglement;
}

float Info::interaction_graph_diversity(){
    std::unordered_set<std::string> seen_pairs;
    unsigned int multi_qubit_count = 0;

    for (const auto& op : qubit_ops) {
        if (op->is_subroutine_op()) continue;
        auto qubits = op->get_target_qubit_names();
        if (qubits.size() < 2) continue;
        multi_qubit_count++;
        // sorted to treat AB == BA
        auto sorted = qubits;
        std::sort(sorted.begin(), sorted.end());
        for (size_t i = 0; i < sorted.size(); i++)
            for (size_t j = i+1; j < sorted.size(); j++)
                seen_pairs.insert(sorted[i] + ":" + sorted[j]);
    }

    return (multi_qubit_count == 0) ? 0.0f :
        (float)seen_pairs.size() / (float)multi_qubit_count;
}

float Info::reducibles_density(){
    unsigned int n_qubit_ops = qubit_ops.size();

    float reducible_density = (float)interesting_pair_count(is_inverse_pair) / (float)n_qubit_ops
        + (float)interesting_pair_count(is_commutative_pair) / (float)n_qubit_ops;
    return std::min(reducible_density, 1.0f);
}

float Info::quality(){
    if (qubit_ops.size() == 0) return 0.0;
    return 0.8f * interaction_graph_diversity() + 0.2f * reducibles_density();
}

