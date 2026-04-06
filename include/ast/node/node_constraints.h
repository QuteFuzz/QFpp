#ifndef NODE_CONSTRAINTS_H
#define NODE_CONSTRAINTS_H

#include <lex.h>
#include <branch.h>

struct Node_constraints {

    public:

        Node_constraints(){}

        Node_constraints(Token_kind rule, unsigned int _occurances):
            constraints{{rule, _occurances}}
        {}

        Node_constraints(std::unordered_map<Token_kind, unsigned int> _constraints):
            constraints(std::move(_constraints))
        {}

        inline bool passed(const Branch& branch){
            // Count the number of occurances of each rule in the branch and return true if they match the expected occurances
            for(const auto& [rule, occurances] : constraints){
                if(branch.count_rule_occurances(rule) != occurances){
                    return false;
                }
            }
            return true;
        }

        inline void set_occurances_for_rule(const Token_kind& rule, unsigned int n_occurances){
            constraints[rule] = n_occurances;
        }

        inline unsigned int n_constraints() const {
            return constraints.size();
        }

        inline void add(const Token_kind& rule, unsigned int n_occurances){
            //Check if rule is in rule_kinds_and_occurances
            if(constraints.find(rule) != constraints.end()){
                constraints[rule] += n_occurances;
            } else {
                constraints[rule] = n_occurances;
            }
        }

        inline const std::unordered_map<Token_kind, unsigned int>& get_constraints() const {
            return constraints;
        }

        friend std::ostream& operator<<(std::ostream& stream, Node_constraints& nc) {
            stream << "====================================" << std::endl;

            for(const auto& [rule, occurances] : nc.constraints){
                stream << kind_as_str(rule) << " " << occurances << std::endl;
            }

            stream << "====================================" << std::endl;

            return stream;
        }

    private:
        std::unordered_map<Token_kind, unsigned int> constraints;

};

#endif
