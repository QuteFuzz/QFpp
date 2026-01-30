#include "../../include/grammar/term.h"
#include <rule.h>

Term::Term(const std::shared_ptr<Rule> rule, const Token_kind& _kind, const Meta_func& _meta_func, unsigned int _branch_nesting_depth){
    value = rule;
    kind = _kind;
    meta_func = _meta_func;
    branch_nesting_depth = _branch_nesting_depth;
}

Term::Term(const std::string& syntax, const Token_kind& _kind, unsigned int _branch_nesting_depth){
    value = syntax;
    kind = _kind;
    branch_nesting_depth = _branch_nesting_depth;
}

std::shared_ptr<Rule> Term::get_rule() const {
    return std::get<std::shared_ptr<Rule>>(value);
}

std::string Term::get_syntax() const {
    return std::get<std::string>(value);
}

std::string Term::get_string() const {
    return is_rule() ? std::get<std::shared_ptr<Rule>>(value)->get_name() : std::get<std::string>(value);
}

Scope Term::get_scope() const {
    return is_rule() ? std::get<std::shared_ptr<Rule>>(value)->get_scope() : Scope::GLOB;
}

Meta_func Term::get_meta_func() const {
    return meta_func;
}

bool Term::is_syntax() const {
    return std::holds_alternative<std::string>(value);
}

bool Term::is_rule() const {
    return std::holds_alternative<std::shared_ptr<Rule>>(value);
}

std::ostream& operator<<(std::ostream& stream, Term term){
    if(term.is_syntax()){
        stream << std::quoted(term.get_syntax());

    } else {
        stream << term.get_rule()->get_name();
    }

    return stream;
}

bool Term::operator==(const Term& other) const {
    if(is_rule() && other.is_rule()){
        return *get_rule() == *other.get_rule();

    } else if (is_syntax() && other.is_syntax()){
        return get_syntax() == other.get_syntax();
    } else {
        return false;
    }
}
