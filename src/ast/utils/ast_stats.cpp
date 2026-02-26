#include <ast_stats.h>

std::vector<std::shared_ptr<Gate>> get_gates(Node& ast){
    std::vector<std::shared_ptr<Gate>> gates;

	for(std::shared_ptr<Node>& node : Node_gen(ast, GATE_NAME)){
		auto gate = std::dynamic_pointer_cast<Gate>(node->child_at(0));
        if (gate != nullptr){
            gates.push_back(gate);
        }
	}

	for(std::shared_ptr<Node>& node : Node_gen(ast, SUBROUTINE)){
		auto gate = std::dynamic_pointer_cast<Gate>(node);
        if (gate != nullptr) {
            gates.push_back(gate);
        }
	}

    return gates;
}
