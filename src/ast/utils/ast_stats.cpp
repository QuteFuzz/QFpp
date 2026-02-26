#include <ast_stats.h>

std::vector<std::shared_ptr<Gate>> get_gates(const std::shared_ptr<Node> compilation_unit){
    std::vector<std::shared_ptr<Gate>> gates;

	for(std::shared_ptr<Node>& node : Node_gen(*compilation_unit, GATE_NAME)){
		auto gate = std::dynamic_pointer_cast<Gate>(node->child_at(0));
        if (gate != nullptr){
            gates.push_back(gate);
        }
	}

	for(std::shared_ptr<Node>& node : Node_gen(*compilation_unit, SUBROUTINE)){
		auto gate = std::dynamic_pointer_cast<Gate>(node);
        if (gate != nullptr) {
            gates.push_back(gate);
        }
	}

    return gates;
}

unsigned int max_control_flow_depth(const std::shared_ptr<Node> node, unsigned int current_depth){
    Token_kind kind = node->get_node_kind();

    unsigned int depth = current_depth + (kind == CF_STMT);
    unsigned int max_depth = depth;

    for(const std::shared_ptr<Node>& child : node->get_children()){
        unsigned int child_depth = max_control_flow_depth(child, depth);
        max_depth = std::max(max_depth, child_depth);
    }

    return max_depth;
}

unsigned int has_mixed_body(const std::shared_ptr<Node> node){
    for (auto& compound_stmts : Node_gen(*node, COMPOUND_STMTS)){
        bool has_gate = compound_stmts->find(QUBIT_OP) != nullptr;
        bool has_cf   = compound_stmts->find(CF_STMT)  != nullptr;

        if (has_gate && has_cf) return 1;
    }

    return 0;
}