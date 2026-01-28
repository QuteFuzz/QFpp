#ifndef TERM_H
#define TERM_H

#include <utils.h>
#include <lex.h>

class Rule;
enum class Scope;

class Term {
    public:
        Term(){}

        Term(const std::shared_ptr<Rule> rule, const Token_kind& _kind, unsigned int _branch_nesting_depth = 0);

        Term(const std::string& syntax, const Token_kind& _kind, unsigned int _branch_nesting_depth = 0);

        ~Term() = default;

        std::shared_ptr<Rule> get_rule() const;

        std::string get_syntax() const;

        std::string get_string() const;

        Scope get_scope() const;

        bool is_syntax() const;

        bool is_rule() const;

        friend std::ostream& operator<<(std::ostream& stream, Term term);

        bool operator==(const Term& other) const;

        Token_kind get_kind() const {return kind;}

        unsigned int get_branch_nesting_depth() const { return branch_nesting_depth; }

    private:
        std::variant<std::shared_ptr<Rule>, std::string> value;
        Token_kind kind;

        unsigned int branch_nesting_depth = 0;
};

#endif
