#ifndef BRANCH_H
#define BRANCH_H

#include <qf_term.h>

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

        inline bool get_recursive_flag() const {return recursive;}

        inline void set_recursive_flag(){recursive = true;}

        inline void add(const Term& term){
            terms.push_back(term);
        }

        inline size_t size() const {return terms.size();}

        inline const Term& at(size_t index) const {return terms.at(index);}

        inline Term& at(size_t index) {return terms.at(index);}

        inline unsigned int count_rule_occurances(const Token_kind& kind) const {

            unsigned int out = 0;

            for(const Term& term : terms){
                out += (term.is_rule()) && (term.get_node_kind() == kind);
            }

            return out;
        }

        inline bool is_empty() const {return terms.empty();}

        inline std::vector<Term> get_terms() const {return terms;}

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

        inline void clear(){
            terms.clear();
        }

    private:
        bool recursive = false;
        std::vector<Term> terms;
};


#endif
