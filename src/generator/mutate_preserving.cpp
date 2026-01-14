#include <mutate.h>
#include <resource.h>

const std::set<Token_kind> X_BASIS = {X, RX};
const std::set<Token_kind> Y_BASIS = {Y, RY};

bool stmt_is_qubit_op(const std::shared_ptr<Node>& compound_stmt){
    return *compound_stmt->child_at(0) == QUBIT_OP;
}

bool diagonal_in_same_basis(const std::shared_ptr<Node>& compound_stmt_a, const std::shared_ptr<Node>& compound_stmt_b){
    std::shared_ptr<Node> qubit_op_a = compound_stmt_a->find(QUBIT_OP)->child_at(0);
    std::shared_ptr<Node> qubit_op_b = compound_stmt_b->find(QUBIT_OP)->child_at(0);

    bool qubit_ops_found = ((qubit_op_a != nullptr) && (qubit_op_b != nullptr)); 

    if(qubit_ops_found){
        std::shared_ptr<Qubit> qubit_a = std::dynamic_pointer_cast<Qubit>(qubit_op_a->find(QUBIT));
        std::shared_ptr<Qubit> qubit_b = std::dynamic_pointer_cast<Qubit>(qubit_op_b->find(QUBIT));

        std::shared_ptr<Node> gate_a = qubit_op_a->find(GATE_MAME)->child_at(0);
        std::shared_ptr<Node> gate_b = qubit_op_b->find(GATE_MAME)->child_at(0);

        bool gates_found = (gate_a != nullptr) && (gate_b != nullptr);
        bool qubits_match = (qubit_a != nullptr) && (qubit_b != nullptr) && (*qubit_a == *qubit_b);

        std::cout << "gf: " << gates_found << " qm: " << qubits_match << " " << qubit_a << " " << qubit_b << std::endl;

        if (gates_found && qubits_match){
            return (X_BASIS.count(gate_a->get_kind()) && X_BASIS.count(gate_b->get_kind())) || 
                (Y_BASIS.count(gate_a->get_kind()) && Y_BASIS.count(gate_b->get_kind()));
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void Commutation_rule::apply(std::shared_ptr<Node>& compound_stmts){
    std::vector<std::shared_ptr<Node>*> visited_slots;
    std::shared_ptr<Node>* maybe_compound_stmt_a = compound_stmts->find_slot(COMPOUND_STMT, visited_slots);
    std::shared_ptr<Node>* maybe_compound_stmt_b = compound_stmts->find_slot(COMPOUND_STMT, visited_slots, false);

    bool matches;

    while((maybe_compound_stmt_a != nullptr) && (maybe_compound_stmt_b != nullptr)){
        matches = false;

        std::shared_ptr<Node> compound_stmt_a = *maybe_compound_stmt_a;
        std::shared_ptr<Node> compound_stmt_b = *maybe_compound_stmt_b;

        matches |= diagonal_in_same_basis(compound_stmt_a, compound_stmt_b);

        if(matches){
            std::shared_ptr<Node> temp = *maybe_compound_stmt_a;

            *maybe_compound_stmt_a = *maybe_compound_stmt_b;
            *maybe_compound_stmt_b = temp;
        }

        maybe_compound_stmt_a = compound_stmts->find_slot(COMPOUND_STMT, visited_slots);
        maybe_compound_stmt_b = compound_stmts->find_slot(COMPOUND_STMT, visited_slots, false);
    }
    std::cout << "============================" << std::endl;
}

