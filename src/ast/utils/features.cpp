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
        Feature("has_control_flow", has_control_flow()),
        Feature("has_subroutine_call", has_subroutine_call()),
        Feature("has_parametrised", has_parametrised()),
        Feature("barrier_op_ratio", barrier_op_ratio(), 3),
        Feature("multi_qubit_gate_ratio", multi_qubit_gate_ratio(), 5),
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

// activates flow unrolling passes
unsigned int Features::has_control_flow(){
    return (*compilation_unit)->find(CF_STMT) != nullptr ? 1 : 0;
}

// activates box decomposition
unsigned int Features::has_subroutine_call(){
    return (*compilation_unit)->find(SUBROUTINE_OP) != nullptr ? 1 : 0;
}

// activates barrier-aware scheduling
float Features::barrier_op_ratio(){
    unsigned int n_barriers = 0;
    unsigned int n_qubit_ops = 0;

    for(const auto& node : Node_gen(**compilation_unit, QUBIT_OP)){
        auto child = node->child_at(0);

        if(child != nullptr && (child->get_node_kind() == BARRIER_OP)){
            n_barriers += 1;
        }

        n_qubit_ops += 1;
    }

    return n_qubit_ops == 0 ? 0.0 : (float)n_barriers / (float)n_qubit_ops;
}

// activates entanglement decomposition
float Features::multi_qubit_gate_ratio(){
	unsigned int n_multi_qubit_gates = 0;

	if (n_gates > 0){
		for (const auto& gate : gates){
			n_multi_qubit_gates += (gate->get_num_external_qubits() > 1);
		}	

		return (float)n_multi_qubit_gates / (float)n_gates;

	} else {
		return 0;
	}
}

// activates symbolic optimization
unsigned int Features::has_parametrised(){
    unsigned int n_parameterised = 0;

    for (const auto& gate : gates){
		if (gate->get_num_floats() > 0){return 1;}
	}

	return 0;
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
