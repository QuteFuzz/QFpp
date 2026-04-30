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

        Term(const std::shared_ptr<Rule> rule, const Token_kind& _kind, const Print_mode& _print_mode);

        Term(const std::string& syntax, const Token_kind& _kind);

        ~Term() = default;

        inline void add_expr(std::shared_ptr<Expr> _expr){
            expr = _expr;
        }

        inline std::shared_ptr<Expr> get_expr() const {
            return expr;
        };

        std::shared_ptr<Rule> get_rule() const;

        std::string get_syntax() const;

        std::string get_string() const;

        Scope get_scope() const;

        Print_mode get_print_mode() const;

        bool is_syntax() const;

        bool is_rule() const;

        friend std::ostream& operator<<(std::ostream& stream, const Term& term);

        bool operator==(const Term& other) const;
        
        std::vector<Term> eval_expr(Context& context, std::optional<unsigned int> term_constraint_max) const;

        Token_kind get_node_kind() const {return kind;}

    private:
        std::variant<std::weak_ptr<Rule>, std::string> value;
        Token_kind kind;
        Print_mode pm = Print_mode::DEFAULT;
        std::shared_ptr<Expr> expr = nullptr;
};

Term make_term_from_rule(std::shared_ptr<Rule> rule_ptr);

#endif
