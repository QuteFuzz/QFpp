#include <ast_utils.h>

std::shared_ptr<Gate> gate_from_op(Slot_type slot){
    if ((*slot)->get_node_kind() != GATE_OP) {
        ERROR("Slot must be of kind GATE_OP");
    }

    std::shared_ptr<Node> gate_name = (*slot)->find(GATE_NAME);

    if (gate_name == nullptr){
        ERROR("Gate op node must have gate name as a descendant");
    }

    std::shared_ptr<Gate> gate = std::dynamic_pointer_cast<Gate>(gate_name->child_at(0));

    if (gate == nullptr){
        ERROR("Child of gate name must have node kind of GATE");
    }

    return gate;
}
