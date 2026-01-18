#include <branch.h>

void Branch::add(const Term& term){
    terms.push_back(term);
}

/// @brief Given the current branch and the nesting depth, work out which terms within a branch should be multiplied. The term's grouping scope tells you that, which is
/// infered by comparing with the nesting depth. Terms with a nesting depth strictly greater than it are multiplied, the rest are left alone. This split is stored inside the
/// `Branch_multiply` struct.
/// @param basis
/// @param nesting_depth
void Branch::setup_basis(Branch_multiply& basis, unsigned int nesting_depth) const {

    for(const Term& t : terms){
        if(t.get_branch_nesting_depth() > nesting_depth){  // only multiply the term if it is in the correct grouping scope
            basis.mults.push_back(t);
        } else {
            basis.remainders.push_back(t);
        }
    }
}
