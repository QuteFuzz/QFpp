#include <info.h>
#include <node_gen.h>

Info::Info(Slot_type _compilation_unit) :
    compilation_unit(_compilation_unit)
{
	for(std::shared_ptr<Node>& node : Node_gen(**compilation_unit, GATE_NAME)){
		auto gate = std::dynamic_pointer_cast<Gate>(node->child_at(0));
        if (gate != nullptr){
            gates.push_back(gate);
        } else {
			std::cout << "GATE_NAME child_at(0) kind: " 
			<< (node->child_at(0) ? std::to_string(node->child_at(0)->get_node_kind()) : "null")
			<< std::endl;
		}
	}

	for(std::shared_ptr<Node>& node : Node_gen(**compilation_unit, SUBROUTINE)){
		auto gate = std::dynamic_pointer_cast<Gate>(node);
        if (gate != nullptr) {
            gates.push_back(gate);
        } else {
			std::cout << "GATE_NAME node kind: " 
			<< (node ? std::to_string(node->get_node_kind()) : "null")
			<< std::endl;
		}
	}

    n_gates = gates.size();

    if (n_gates == 0) {
		WARNING("Ciruit has no gates");
		(*compilation_unit)->print_program(std::cout);

	}
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
