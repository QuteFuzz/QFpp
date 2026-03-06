#include <ast_stats.h>

/*
    helpers
*/

std::vector<std::shared_ptr<Gate>> get_gates(const std::shared_ptr<Node> compilation_unit){
    std::vector<std::shared_ptr<Gate>> gates;

	for(std::shared_ptr<Node>& node : Node_gen(*compilation_unit, GATE_NAME)){
		auto gate = std::dynamic_pointer_cast<Gate>(node->child_at(0));
        if (gate != nullptr){
            gates.push_back(gate);
        } else {
			std::cout << "GATE_NAME child_at(0) kind: " 
			<< (node->child_at(0) ? std::to_string(node->child_at(0)->get_node_kind()) : "null")
			<< std::endl;
		}
	}

	for(std::shared_ptr<Node>& node : Node_gen(*compilation_unit, SUBROUTINE)){
		auto gate = std::dynamic_pointer_cast<Gate>(node);
        if (gate != nullptr) {
            gates.push_back(gate);
        } else {
			std::cout << "GATE_NAME node kind: " 
			<< (node ? std::to_string(node->get_node_kind()) : "null")
			<< std::endl;
		}
	}

    return gates;
}

unsigned int max_control_flow_depth_rec(const std::shared_ptr<Node> node, unsigned int current_depth){
    Token_kind kind = node->get_node_kind();

    unsigned int depth = current_depth + (kind == CF_STMT);
    unsigned int max_depth = depth;

    for(const std::shared_ptr<Node>& child : node->get_children()){
        unsigned int child_depth = max_control_flow_depth_rec(child, depth);
        max_depth = std::max(max_depth, child_depth);
    }

    return max_depth;
}


/* 
    AST features
*/

Feature_vec::Feature_vec(const Slot_type _compilation_unit):
    compilation_unit(_compilation_unit),
	gates(get_gates(*_compilation_unit)),
    n_gates(gates.size())
{
	unsigned int multi_qubit_gate_n_bins = 5;

	if (n_gates == 0) {
		WARNING("Ciruit has no gates");
		(*compilation_unit)->print_program(std::cout);

	}

    vec = {
        Feature{"has_control_flow", has_control_flow()},
        Feature{"has_subroutine_call", has_subroutine_call()},
        Feature{"has_barrier", has_barrier()},
		// done this way such that archive size is constant for all genomes, otherwise could use max number of gates as num of bins
        Feature{"multi_qubit_gate_ratio", (unsigned int)(multi_qubit_gate_ratio() * multi_qubit_gate_n_bins), multi_qubit_gate_n_bins},
        Feature{"has_parametrised", has_parametrised()},
    };

    for (auto& f : vec){
        archive_size *= (f.num_bins + 1); // extra bin for stuff that doesn't fit into any bin
    }
}


// activates flow unrolling passes
unsigned int Feature_vec::has_control_flow(){
    return (*compilation_unit)->find(CF_STMT) != nullptr ? 1 : 0;
}

// activates box decomposition
unsigned int Feature_vec::has_subroutine_call(){
    return (*compilation_unit)->find(SUBROUTINE_OP) != nullptr ? 1 : 0;
}

// activates barrier-aware scheduling
unsigned int Feature_vec::has_barrier(){
    return (*compilation_unit)->find(BARRIER_OP) != nullptr ? 1 : 0;
}

// activates entanglement decomposition
float Feature_vec::multi_qubit_gate_ratio(){
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
unsigned int Feature_vec::has_parametrised(){
    for (const auto& gate : gates){
		if (gate->get_num_floats() > 0){return 1;}
	}

	return 0;
}

unsigned int Feature_vec::get_archive_size() const {
    return archive_size;
}

/// figure out where in the archive this feature vec falls
unsigned int Feature_vec::get_archive_index() const {
    std::vector<unsigned int> bins;

    for (auto& feature : vec){
        unsigned int effective_num_bins = feature.num_bins + 1;

        // clamp to last bin if value exceeds range
        unsigned int bin = (feature.bin_width > 0)
            ? feature.val / feature.bin_width
            : 0;

        bin = std::min(bin, effective_num_bins - 1);
        bins.push_back(bin);
    }

    assert(bins.size() == vec.size());
    
    // second pass to find index while accumulating stride
    unsigned int index = 0;
    unsigned int stride = 1;

    for (int j = (int)vec.size() - 1; j >= 0; j--){
        index += bins[j] * stride;
        stride *= (vec[j].num_bins + 1); // +1 to account for additional bin
    }
    
    return index;
}


/*
    AST quality heuristics
*/

Quality::Quality(Slot_type _compilation_unit):
    compilation_unit(_compilation_unit),
    gates(get_gates(*_compilation_unit)),
    n_gates(gates.size())
{
    for(auto& gate : gates){
        Token_kind kind = gate->get_node_kind();
        if (gate_occurances.find(kind) == gate_occurances.end()){
            gate_occurances[kind] = 1;
        } else {
            gate_occurances[kind] += 1;
        }
    }

    components = {
        Component{"gate_arity_variance", gate_arity_variance()},
        Component{"gate_type_entropy", gate_type_entropy()},
        Component{"adj_gate_pair_density", adj_gate_pair_density()},
        Component{"has_mixed_body", (float)has_mixed_body()},
        Component{"max_control_flow_depth", (float)max_control_flow_depth()},
    };
}

float Quality::gate_arity_variance(){
    if (n_gates == 0) return 0.0;

    std::vector<unsigned int> gate_arities;
    unsigned int sum = 0;

    for (const auto& gate : gates){
        unsigned int n_qubits = gate->get_num_external_qubits();
        gate_arities.push_back(n_qubits);
        sum += n_qubits;
    }

    float mean = (float)sum / n_gates;
    float variance = 0.0;

    for(unsigned int gate_arity : gate_arities){
        variance += std::pow(((float)gate_arity - mean), 2.0);
    }

    assert(variance >= 0.0);

    return variance / n_gates;
}

float Quality::gate_type_entropy(){
    float neg_shannon_entropy = 0.0;

    if (n_gates > 0){
        for(auto&[kind, count] : gate_occurances){
            float frac = count / (float)n_gates;
            neg_shannon_entropy += frac * std::log2(frac);
        }
    }

    return -neg_shannon_entropy;
}

float Quality::adj_gate_pair_density(){
    if (n_gates >= 2){
        unsigned int adj_gate_pairs = 0;

        for(size_t i = 0; i < n_gates - 1; i++){
            if(*gates[i] == *gates[i+1]){
                adj_gate_pairs += 1;
            }
        }

        return (float)adj_gate_pairs / (float)(n_gates - 1);
    
    } else {
        return 0.0;
    }
}

unsigned int Quality::has_mixed_body(){
    for (auto& compound_stmts : Node_gen(**compilation_unit, COMPOUND_STMTS)){
        bool has_gate = compound_stmts->find(QUBIT_OP) != nullptr;
        bool has_cf   = compound_stmts->find(CF_STMT)  != nullptr;

        if (has_gate && has_cf) return 1;
    }

    return 0;
}

unsigned int Quality::max_control_flow_depth(){
    return max_control_flow_depth_rec(*compilation_unit, 0);
}
