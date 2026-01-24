#include <rule.h>
#include <node.h>

void Rule::add(const Branch& branch){
    branches.push_back(branch);

    if(branch.get_recursive_flag()){
        recursive = true; // this rule is recursive
    }
}

bool Rule::contains_rule(const Token_kind& other_rule){
    for(auto& branch : branches){
        if (branch.count_rule_occurances(other_rule) != 0) return true;
    }

    return false;
}


/// @brief Pick a branch from the rule. The parent node here is the node created for this rule, and it contains a constriant which is used to pick
/// the correct branch to make the child nodes
/// @param parent 
/// @return 
Branch Rule::pick_branch(std::shared_ptr<Node> rule_as_node){
    size_t size = branches.size();

    bool valid_branch_exists = false;

    for (const auto& branch : branches) {
        if (rule_as_node->branch_satisfies_constraints(branch)) {
            valid_branch_exists = true;
            break;
        }
    }

    if(valid_branch_exists){
        Branch branch = branches[random_uint(size - 1)];

        while(!rule_as_node->branch_satisfies_constraints(branch)){
            branch = branches[random_uint(size - 1)];
        }

        return branch;

    } else {

        #ifdef DEBUG
        if(rule_as_node->has_constraints()){
            std::cout << "No branch exisits for this rule that satisfies constraints" << std::endl;
            std::cout << *this << std::endl;
            rule_as_node->print_constraints(std::cout);
        }
        #endif

        return Branch();
    }
}
