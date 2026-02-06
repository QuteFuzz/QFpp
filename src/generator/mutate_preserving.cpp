#include <mutate.h>
#include <resource.h>
#include <float_literal.h>
#include <ast_utils.h>
#include <qubit_op.h>

bool stmt_is_qubit_op(const std::shared_ptr<Node>& compound_stmt){
    return *compound_stmt->child_at(0) == QUBIT_OP;
}

bool diagonal_in_same_basis(const std::shared_ptr<Node>& compound_stmt_a, const std::shared_ptr<Node>& compound_stmt_b, std::set<Token_kind> basis){
    std::shared_ptr<Node> qubit_op_a = compound_stmt_a->find(QUBIT_OP)->child_at(0);
    std::shared_ptr<Node> qubit_op_b = compound_stmt_b->find(QUBIT_OP)->child_at(0);

    bool qubit_ops_found = ((qubit_op_a != nullptr) && (qubit_op_b != nullptr));

    if(qubit_ops_found){
        std::shared_ptr<Resource> qubit_a = std::dynamic_pointer_cast<Resource>(qubit_op_a->find(QUBIT));
        std::shared_ptr<Resource> qubit_b = std::dynamic_pointer_cast<Resource>(qubit_op_b->find(QUBIT));

        std::shared_ptr<Node> gate_a = qubit_op_a->find(GATE_NAME)->child_at(0);
        std::shared_ptr<Node> gate_b = qubit_op_b->find(GATE_NAME)->child_at(0);

        bool gates_found = (gate_a != nullptr) && (gate_b != nullptr);
        bool qubits_match = (qubit_a != nullptr) && (qubit_b != nullptr) && (*qubit_a == *qubit_b);

        // std::cout << "gf: " << gates_found << " qm: " << qubits_match << std::endl;
        // std::cout << "Qubits " << std::endl;
        // qubit_a->print_ast("");
        // qubit_b->print_ast("");

        if (gates_found && qubits_match){
            return (basis.count(gate_a->get_node_kind()) && basis.count(gate_b->get_node_kind()));
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

    while((maybe_compound_stmt_a != nullptr) && (maybe_compound_stmt_b != nullptr)){
        if(diagonal_in_same_basis(*maybe_compound_stmt_a, *maybe_compound_stmt_b, basis)){
            std::swap(*maybe_compound_stmt_a, *maybe_compound_stmt_b);
        }

        maybe_compound_stmt_a = compound_stmts->find_slot(COMPOUND_STMT, visited_slots);
        maybe_compound_stmt_b = compound_stmts->find_slot(COMPOUND_STMT, visited_slots, false);
    }
    std::cout << "============================" << std::endl;
}

void Gate_fission::apply(std::shared_ptr<Node>& compound_stmts){

    for(auto _qubit_op : Node_gen(*compound_stmts, QUBIT_OP)){
        std::shared_ptr<Qubit_op> qubit_op = std::dynamic_pointer_cast<Qubit_op>(_qubit_op);

        if(!qubit_op->is_subroutine_op()){
            // std::shared_ptr<Gate> gate = std::dynamic_pointer_cast<Gate>(qubit_op->find(GATE_NAME)->child_at(0));
            // std::shared_ptr<Float_literal> f = std::dynamic_pointer_cast<Float_literal>(qubit_op->find(FLOAT_LITERAL));

        }

    }
}
