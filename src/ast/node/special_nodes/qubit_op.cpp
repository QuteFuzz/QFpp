#include <qubit_op.h>
#include <circuit.h>
#include <node_gen.h>
#include <ast_utils.h>

bool Qubit_op::is_subroutine_op() const {
    if (gate_node == nullptr){
        return false;
    } else {
        return *gate_node == SUBROUTINE_OP; 
    }
}

void Qubit_op::add_gate_if_subroutine(std::vector<std::shared_ptr<Node>>& subroutine_gates){
    assert(gate_node != nullptr);

    if(is_subroutine_op()){
        for(std::shared_ptr<Node>& gate : subroutine_gates){
            if(gate->get_str() == gate_node->get_str()){return;}
        }

        subroutine_gates.push_back(gate_node);
    }
}

std::string Qubit_op::resolved_name() const {
    assert(gate_node != nullptr);
    return gate_node->get_str() + ", id: " + std::to_string(id);
}

/// Need to check for duplication although I use `Node_gen` because it only differentiates nodes
/// by their address, not the content. If I spawed `QUBIT` nodes twice after cloning in the AST below this qubit op, 
/// then I get duplicates. I do this in `ast.cpp` to pass `QUBIT` nodes to specific `REGISTER / SINGULAR` qubits
std::vector<std::string> Qubit_op::get_target_qubit_names() {
    std::vector<std::string> qubit_resolved_names;
    std::set<std::string> seen_names;

    for (const auto& qubit : resources_from_anscestor(*this, QUBIT)) {        
        std::string resolved_name = qubit->resolved_name();

        if (seen_names.insert(resolved_name).second){
            qubit_resolved_names.push_back(resolved_name);
        }
    }
 
    return qubit_resolved_names;
}
