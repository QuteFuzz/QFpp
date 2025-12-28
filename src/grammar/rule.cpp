#include <rule.h>
#include <node.h>

/// @brief need to have this check and store pointers to recursive branches separately
/// @param branch 
void Rule::add(const Branch& branch){
    branches.push_back(branch);

    if(branch.get_recursive_flag()){
        recursive = true; // this rule is recursive
    }
}

/// @brief checks for existing constraint, and adds the new constraint to it if it exists
/// @param rule_kind 
/// @param n_occurances 
void Rule::add_constraint(const Token_kind& rule_kind, unsigned int n_occurances){
    if(constraint.has_value()){
        constraint.value().add(rule_kind, n_occurances);
    } else {
        constraint = std::make_optional<Node_constraint>(rule_kind, n_occurances);
    }
}

Branch Rule::pick_branch(std::shared_ptr<Node> parent){
    
    size_t size = branches.size();
    bool valid_branch_exists = false;
    for (const auto& branch : branches) {
        if (parent->branch_satisfies_constraint(branch)) {
            valid_branch_exists = true;
            break;
        }
    }

    if(size > 0 && valid_branch_exists){
        Branch branch = branches[random_int(size - 1)];

        // #ifdef DEBUG
        // INFO("Picking branch for " + token.value + STR_SCOPE(scope) + " while satisfying constraint " + parent->get_debug_constraint_string());
        // #endif

        while(!parent->branch_satisfies_constraint(branch)){
            branch = branches[random_int(size - 1)];
        }

        return branch;

    } else {
        #ifdef DEBUG
        INFO(token.value + STR_SCOPE(scope) + " is an empty rule");
        #endif

        return Branch();
    }
}

