#include <ast_utils.h>
#include <node_gen.h>
#include <resource.h>

std::shared_ptr<Gate> gate_from_op(Slot_type gate_op_slot) {
    if ((*gate_op_slot)->get_node_kind() != GATE_OP) {
        ERROR("Slot must be of kind GATE_OP");
    }

    std::shared_ptr<Node> gate_name = (*gate_op_slot)->find(GATE_NAME);

    if (gate_name == nullptr){
        ERROR("Gate op node must have gate name as a descendant");
    }

    std::shared_ptr<Gate> gate = std::dynamic_pointer_cast<Gate>(gate_name->child_at(0));

    if (gate == nullptr){
        std::cout << "========================" << std::endl;
        (*gate_op_slot)->print_program(std::cout);
        std::cout << "========================" << std::endl;

        ERROR("Child of gate name must have node kind of GATE");
    }

    return gate;
}

void move_qubits(const std::shared_ptr<Node> source_gate_op, Slot_type dest_gate_op) {
    auto source_qubits = Node_gen(*source_gate_op, QUBIT);
    auto dest_qubits = Node_gen(**dest_gate_op, QUBIT);

    auto source_it = source_qubits.begin();
    auto dest_it = dest_qubits.begin();

    while((source_it != source_qubits.end()) && (dest_it != dest_qubits.end())){
        *dest_it = *source_it;

        dest_it++;
        source_it++;
    }
}

void swap_qubits(const Slot_type gate_op) {
    std::shared_ptr<Gate> gate = gate_from_op(gate_op);

    if (gate->get_num_external_qubits() == 2){
        auto node_gen = Node_gen(**gate_op, QUBIT);

        auto qubit_0 = node_gen.begin()++;
        auto qubit_1 = node_gen.begin();

        std::swap(*qubit_0, *qubit_1);
    }
}
