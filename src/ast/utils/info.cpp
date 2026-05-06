#include <info.h>
#include <node_gen.h>
#include <ast_utils.h>
#include <unordered_set>

static bool is_inverse_pair(Token_kind a, Token_kind b) {
    if (a == b) {
        static const std::unordered_set<Token_kind> self_inverse = {
            H, X, Y, Z, CX, CY, CZ, SWAP, CCX, CSWAP, TOFFOLI
        };
        return self_inverse.count(a);
    }
    return (a == S   && b == SDG) || (a == SDG && b == S)   ||
           (a == T   && b == TDG) || (a == TDG && b == T)   ||
           (a == V   && b == VDG) || (a == VDG && b == V);
}

Info::Info(Slot_type _compilation_unit) :
    compilation_unit(_compilation_unit)
{
    for (const auto& node : Node_gen(**compilation_unit, QUBIT_OP)){
        auto qubit_op = std::dynamic_pointer_cast<Qubit_op>(node);

        if (qubit_op == nullptr){
            node->print_program(std::cout);
            std::cout << std::endl;
            
            node->print_ast("");
            std::cout << std::endl;

            ERROR("Nodes of kind `QUBIT_OP` must be of `Qubit_op` type");
        }
    
        qubit_ops.push_back(qubit_op);
    }

    # if 0
    if (qubit_ops.size() == 0) {
		WARNING("Ciruit has no `QUBIT_OP` nodes");
		(*compilation_unit)->print_program(std::cout);

	}
    #endif

    /*
        ratios are used to ensure that the archive size is constant for any program generated. For example, `stmt_ratio`
        is calculated, then multiplied by the chosen number of bins to get the bin index, instead of using the number of statements 
        in the compilation unit as this is not a constant value. Otherwise, known constants like `NESTED_MAX_DEPTH` can be used to
        denote the number of bins
    */
    feature_vecs = {
        Feature("self_inverse_pair_density", self_inverse_pair_density(), 5)
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

float Info::quality(){
    /// TODO:
    return 1.0;
}

float Info::self_inverse_pair_density(){

    int n_sequences = 0;

    if (qubit_ops.size() < 2) return 0.0;

    float n_pairs = (float)(qubit_ops.size() * (qubit_ops.size() - 1)) / 2.0;

    // qubit name -> last qubit op acting on that qubit
    std::unordered_map<std::string, std::shared_ptr<Qubit_op>> last_qubit_op;

    for (const auto& qubit_op : qubit_ops){

        if (qubit_op->is_subroutine_op()) continue;
        
        Token_kind gate_kind = qubit_op->get_node_kind();
        auto qubit_names = qubit_op->get_target_qubit_names();

        bool qubits_satisfied = true;
        std::shared_ptr<Qubit_op> prev_op = nullptr;

        // find prev op for all qubits. it must be the same for all qubits, otherwise, there's no pair
        for (const std::string& name : qubit_names){
            auto it = last_qubit_op.find(name);

            if (it == last_qubit_op.end()){
                qubits_satisfied = false; break;
            }

            if (prev_op == nullptr){
                prev_op = it->second;
            }
        }

        if (qubits_satisfied && (prev_op != nullptr) && is_inverse_pair(gate_kind, prev_op->get_node_kind())){
            n_sequences += 1;
            // remove all qubits from last_qubit_op tracker
            for (const auto& name : qubit_names) last_qubit_op.erase(name);
        
        } else {
            // set last qubit op for all qubits
            for (const auto& name : qubit_names) last_qubit_op[name] = qubit_op;
        }
    }

    return (float)n_sequences / n_pairs;
}
