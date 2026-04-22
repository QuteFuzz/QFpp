#ifndef TERM_H
#define TERM_H

#include <utils.h>
#include <lex.h>
#include <rule_utils.h>
#include <expr.h>

class Rule;
class Context;

class Term {
    public:
        Term(){}

        Term(const std::shared_ptr<Rule> rule, const Token_kind& _kind, const Meta_func& _meta_func);

        Term(const std::string& syntax, const Token_kind& _kind);

        ~Term() = default;

        // Term(Term&&) = default;  // generate move constructor
        
        // Term& operator=(Term&&) = default;  // generate move assignment op

        inline void add_term_constraint(std::shared_ptr<Expr> _constraint){
            constraint = _constraint;
        }

        std::shared_ptr<Rule> get_rule() const;

        std::string get_syntax() const;

        std::string get_string() const;

        Scope get_scope() const;

        Meta_func get_meta_func() const;

        bool is_syntax() const;

        bool is_rule() const;

        int eval_constraint(const Context& context) const;

        friend std::ostream& operator<<(std::ostream& stream, const Term& term);

        bool operator==(const Term& other) const;

        Token_kind get_node_kind() const {return kind;}

    private:
        std::variant<std::weak_ptr<Rule>, std::string> value;
        Token_kind kind;
        Meta_func meta_func = Meta_func::NONE;
        std::shared_ptr<Expr> constraint = nullptr;
};

#endif
