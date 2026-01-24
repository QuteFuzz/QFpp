#ifndef RULE_H
#define RULE_H

#include <branch.h>
#include "node.h"
#include "utils.h"

class Node;

class Rule {

    public:
        Rule(){}

        Rule(const Token& _token, const U8& _scope) :
            token(_token),
            scope(_scope)
        {}

        Rule(const std::vector<Branch>& _branches) :
            branches(_branches)
        {}

        ~Rule(){}

        std::string get_name() const {return token.value;}

        Token get_token() const {return token;}

        U8 get_scope() const {return scope;}

        bool get_recursive_flag() const {return recursive;}

        std::vector<Branch> get_branches(){return branches;}

        void add(const Branch& b);

        inline size_t size(){return branches.size();}

        inline bool is_empty() const {return branches.empty();}

        inline void clear(){branches.clear();}

        Branch pick_branch(std::shared_ptr<Node> rule_as_node);

        bool contains_rule(const Token_kind& other_rule);

        bool operator==(const Rule& other) const {
            return (token == other.get_token()) && scope_matches(scope, other.get_scope());
        }

        /// Checks that rule name and scope matches
        /// Scope matching is done by == if both are NO_SCOPE, otherwise, & is used
        bool matches(const std::string& name, const U8& _scope) {
            return (token.value == name) && scope_matches(scope, _scope);
        }

        friend std::ostream& operator<<(std::ostream& stream, const Rule& rule){
            stream << rule.get_name() << " = ";

            for(size_t i = 0; i < rule.branches.size(); i++){
                stream << rule.branches[i];
                if(i < rule.branches.size() - 1) stream << " | ";
            }

            stream << " ; " << STR_SCOPE(rule.scope) << std::endl;

            return stream;
        }

    private:
        Token token;
        U8 scope = NO_SCOPE;
        std::vector<Branch> branches;
        bool recursive = false;
};

#endif
