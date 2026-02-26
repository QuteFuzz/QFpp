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

unsigned int max_control_flow_depth(const Node& node, unsigned int current_depth){
    Token_kind kind = node.get_node_kind();

    unsigned int depth = current_depth + (kind == CF_STMT);
    unsigned int max_depth = depth;

    for(const std::shared_ptr<Node>& child : node.get_children()){
        unsigned int child_depth = max_control_flow_depth(*child, depth);
        max_depth = std::max(max_depth, child_depth);
    }

    return max_depth;
}

unsigned int subroutine_depth(const Node& node, unsigned int current_depth, bool inside_subroutine){
    Token_kind kind = node.get_node_kind();

    bool is_call = (kind == SUBROUTINE_OP);

    unsigned int depth = current_depth + (is_call && inside_subroutine ? 1 : 0);
    unsigned int max_depth = depth;

    bool now_inside = inside_subroutine || (kind == SUBROUTINE_DEFS);

    for (const auto& child : node.get_children()){
        unsigned int child_depth = subroutine_depth(*child, depth, now_inside);
        max_depth = std::max(max_depth, child_depth);
    }

    return max_depth;
}