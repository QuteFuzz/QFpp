#ifndef TERM_H
#define TERM_H

#include <utils.h>
#include <lex.h>
#include <rule_utils.h>
#include <term_constraint.h>

class Rule;

class Term {
    public:
        Term(){}

        Term(const std::shared_ptr<Rule> rule, const Token_kind& _kind, const Meta_func& _meta_func);

        Term(const std::string& syntax, const Token_kind& RULE_KINDS_TOP);

        ~Term() = default;

        inline void add_constraint(const Term_constraint& _constraint){
            constraint = _constraint;
        }

        Term_constraint get_constaint() const {
            return constraint;
        }

        std::shared_ptr<Rule> get_rule() const;

        std::string get_syntax() const;

        std::string get_string() const;

        Scope get_scope() const;

        Meta_func get_meta_func() const;

        bool is_syntax() const;

        bool is_rule() const;

        friend std::ostream& operator<<(std::ostream& stream, Term term);

        bool operator==(const Term& other) const;

        Token_kind get_node_kind() const {return kind;}

    private:
        std::variant<std::weak_ptr<Rule>, std::string> value;
        Token_kind kind;
        Meta_func meta_func = Meta_func::NONE;
        Term_constraint constraint;
};

#endif
