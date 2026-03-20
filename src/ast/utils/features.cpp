#include <info.h>

Features::Features(Slot_type _compilation_unit) :
    Info(_compilation_unit)
{
    vec = {
        Feature("has_control_flow", has_control_flow()),
        Feature("has_subroutine_call", has_subroutine_call()),
        Feature("has_barrier", has_barrier()),
        Feature("has_parametrised", has_parametrised()),
        Feature("multi_qubit_gate_ratio", multi_qubit_gate_ratio(), 5),
    };

    for (auto& f : vec){
        archive_size *= f.effective_num_bins(); // extra bin for stuff that doesn't fit into any bin
    }
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
unsigned int Features::has_barrier(){
    return (*compilation_unit)->find(BARRIER_OP) != nullptr ? 1 : 0;
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
        stride *= (vec[j].num_bins + 1); // +1 to account for additional bin
    }
    
    return index;
}
