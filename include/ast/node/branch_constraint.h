#ifndef BRANCH_CONSTRAINT_H
#define BRANCH_CONSTRAINT_H

#include <lex.h>
#include <branch.h>

struct Branch_constraint {

    public:
        Branch_constraint(){}

        Branch_constraint(Token_kind _singleton_term_token_kind, unsigned int _n_occurances):
            singleton_term_token_kind(_singleton_term_token_kind),
            n_occurances(_n_occurances)
        {}

        inline bool passed(const Branch& branch){
            return branch.count_rule_occurances(singleton_term_token_kind) == n_occurances;
        }

        friend std::ostream& operator<<(std::ostream& stream, Branch_constraint& nc) {
            stream << "====================================" << std::endl;
            stream << kind_as_str(nc.singleton_term_token_kind) << " " << nc.n_occurances << std::endl;
            stream << "====================================" << std::endl;

            return stream;
        }

    private:
        Token_kind singleton_term_token_kind;
        unsigned int n_occurances;
};

#endif
