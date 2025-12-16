#ifndef BRANCH_H
#define BRANCH_H

#include <term.h>

struct Branch_multiply {
    std::vector<Term> remainders;
    std::vector<Term> mults;

    bool is_empty(){
        return remainders.empty() && mults.empty();
    }

    void clear(){
        remainders.clear();
        mults.clear();
    }

};

class Branch {

    public:
        Branch(){}

        ~Branch(){}

        Branch(std::vector<Term> _terms) {
            // collapse terms
            for(Term& t : _terms){
                add(t);
            }
        }

        bool get_recursive_flag() const {return recursive;}

        inline void set_recursive_flag(){recursive = true;}

        void add(const Term& term);

        size_t size() const {return terms.size();}

        const Term& at(size_t index) const {return terms.at(index);}

        Term& at(size_t index) {return terms.at(index);}

        unsigned int count_rule_occurances(const Token_kind& kind) const {

            unsigned int out = 0;

            for(const Term& term : terms){
                out += (term.is_rule()) && (term.get_kind() == kind);
            }

            return out;
        }

        bool is_empty() const {return terms.empty();}

        std::vector<Term> get_terms() const {return terms;} 

        void setup_basis(Branch_multiply& basis, unsigned int nesting_depth) const;

        friend std::ostream& operator<<(std::ostream& stream, const Branch& branch){
            for(const auto& elem : branch.terms){
                stream << elem << " ";
            }

            return stream;
        }

        std::vector<Term>::iterator begin(){
            return terms.begin();
        }

        std::vector<Term>::iterator end(){
            return terms.end();
        }

        std::vector<Term>::const_iterator begin() const {
            return terms.begin();
        }

        std::vector<Term>::const_iterator end() const {
            return terms.end();
        }

    private:
        bool recursive = false;

        std::vector<Term> terms;
};


#endif
