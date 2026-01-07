#include <mutate.h>
#include <cassert>

bool Remove_gate::match(const std::shared_ptr<Node> compound_stmts){
    
    std::vector<std::shared_ptr<Node>*> visited_slots;
    std::shared_ptr<Node>* maybe_compound_stmt = compound_stmts->find_slot(COMPOUND_STMT, visited_slots);

    std::cout << "============================" << std::endl;
    std::cout << *compound_stmts << std::endl;

    bool matches = false;

    while(maybe_compound_stmt != nullptr){
        std::shared_ptr<Node> compound_stmt = *maybe_compound_stmt;

        assert(compound_stmt->get_num_children() == 1);

        if((*compound_stmt->child_at(0) == QUBIT_OP) && (compound_stmt->find(kind) != nullptr)){
            slots.push_back(maybe_compound_stmt);
            std::cout << "matches" << std::endl;
            matches = true;
        }

        maybe_compound_stmt = compound_stmts->find_slot(COMPOUND_STMT, visited_slots);
    }
    std::cout << "============================" << std::endl;

    return matches;
}

void Remove_gate::apply(std::shared_ptr<Node>& compound_stmts){
    
    std::shared_ptr<Node>* compound_stmt;
    std::shared_ptr<Node> qubit;

    for(int i = 0; i < slots.size(); i++){
        compound_stmt = slots[i];
        qubit = (*compound_stmt)->find(QUBIT);

        std::cout << *qubit << std::endl;

        *compound_stmt = empty;
    }
}

