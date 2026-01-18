#include <qubit_op.h>
#include <circuit.h>

Qubit_op::Qubit_op(std::shared_ptr<Circuit> current_circuit):
    Node("qubit_op", QUBIT_OP)
{
    bool can_use_subroutine = current_circuit->get_can_apply_subroutines();

    if(!can_use_subroutine){
        add_constraint(SUBROUTINE_OP, 0);
    }
}

bool Qubit_op::is_subroutine_op() const{
    return gate_node.has_value() && *gate_node.value() == SUBROUTINE;
}

void Qubit_op::add_gate_if_subroutine(std::vector<std::shared_ptr<Node>>& subroutine_gates){

    if(is_subroutine_op()){
        for(std::shared_ptr<Node>& gate : subroutine_gates){
            if(gate->get_content() == gate_node.value()->get_content()){return;}
        }

        subroutine_gates.push_back(gate_node.value());
    }
}

std::string Qubit_op::resolved_name() const {
    std::string _string = "UNKNOWN";

    if(gate_node.has_value()){
        _string = gate_node.value()->get_content();
    }

    return _string + ", id: " + std::to_string(id);
}
