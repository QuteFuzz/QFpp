#include <info.h>
#include <ast_utils.h>

Features::Features(Slot_type _compilation_unit) :
    Info(_compilation_unit)
{
    /*
        binary features i.e has..... , get 1 not 2 bins, because the effective number always adds 1 for overflows,
        so +1 will get us to 2. 
    */
    vec = {
        Feature("control_flow_ratio", stmt_ratio(COMPOUND_STMT, CF_STMT), 3),
        Feature("subroutine_call_ratio", stmt_ratio(QUBIT_OP, SUBROUTINE_OP), 3),
        Feature("parameterised_gate_ratio", gate_ratio([](std::shared_ptr<Gate> gate){return (gate->get_num_floats() > 0);}), 3),
        Feature("barrier_op_ratio", stmt_ratio(COMPOUND_STMT, BARRIER_OP), 3),
        Feature("multi_qubit_gate_ratio", gate_ratio([](std::shared_ptr<Gate> gate){return (gate->get_num_external_qubits() > 1);}), 3),
    };

    for (auto& f : vec){
        archive_size *= f.effective_num_bins(); // extra bin for stuff that doesn't fit into any bin
    }
}

void Features::dump(std::ofstream& stream){
    stream << "[";

    for (size_t i = 0; i < vec.size(); i++){
        auto f = vec[i];
        stream << "{\"name\": \"" << f.name << "\", \"idx\": " << f.idx() << ", \"n_bins\": " << f.num_bins << ", \"bin_width\": " << f.bin_width << "}";
        
        if (i < vec.size() - 1){
            stream << ",";
        }

        stream << "\n";
    }

    stream << "]";
}

float Features::stmt_ratio(const Token_kind& denominator, const Token_kind& numerator){
    unsigned int denom = 0;
    unsigned int numer = 0;

    for(const auto& node : Node_gen(**compilation_unit, denominator)){
        auto child = node->child_at(0);

        if(child != nullptr && (child->get_node_kind() == numerator)){
            numer += 1;
        }

        denom += 1;
    }

    return denom == 0 ? 0.0 : (float)numer / (float)denom;
}

float Features::gate_ratio(std::function<bool(std::shared_ptr<Gate>)> func){
	unsigned int n_passed = 0;

    for (const auto& gate : gates){
        n_passed += func(gate);
    }	

    return (n_gates == 0) ? 0.0 : (float)n_passed / (float)n_gates;
}

unsigned int Features::get_archive_size() const {
    return archive_size;
}

/// figure out where in the archive this feature vec falls
unsigned int Features::get_archive_index() {
    unsigned int index = 0;
    unsigned int stride = 1;

    for (int j = (int)vec.size() - 1; j >= 0; j--){
        index += vec[j].idx() * stride;
        stride *= vec[j].effective_num_bins();
    }
    
    return index;
}

Features Features::complement() const {
    Features out = *this;
    for (size_t i = 0; i < out.size(); i++) {
        // flip to the other extreme of each dimension
        out[i].raw_idx = out[i].num_bins - out[i].raw_idx;
    }
    return out;
}
