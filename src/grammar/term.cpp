#include "../../include/grammar/qf_term.h"
#include <rule.h>

Term::Term(const std::shared_ptr<Rule> rule, const Token_kind& _kind, const Meta_func& _meta_func){
    value = std::weak_ptr<Rule>(rule),
    kind = _kind;
    meta_func = _meta_func;
}

Term::Term(const std::string& syntax, const Token_kind& _kind){
    value = syntax;
    kind = _kind;
}

std::shared_ptr<Rule> Term::get_rule() const {
    return std::get<std::weak_ptr<Rule>>(value).lock();
}

std::string Term::get_syntax() const {
    return std::get<std::string>(value);
}

std::string Term::get_string() const {
    if(is_rule()){
        auto rule_ptr = get_rule();
        return (rule_ptr == nullptr) ? "[[DELETED RULE]]" : rule_ptr->get_name();
    } else {
        return std::get<std::string>(value);
    }
}

Scope Term::get_scope() const {
    if(is_rule()){
        auto rule_ptr = get_rule();
        return (rule_ptr == nullptr) ? Scope::GLOB : rule_ptr->get_scope();
    } else {
        return Scope::GLOB;
    }
}

Meta_func Term::get_meta_func() const {
    return meta_func;
}

bool Term::is_syntax() const {
    return std::holds_alternative<std::string>(value);
}

bool Term::is_rule() const {
    return std::holds_alternative<std::weak_ptr<Rule>>(value);
}

std::ostream& operator<<(std::ostream& stream, Term term){
    if(term.is_syntax()){
        stream << std::quoted(term.get_syntax());

    } else {
        auto rule_ptr = term.get_rule();

        if (rule_ptr == nullptr){
            stream << "[[DELETED RULE]]";
        } else {
            stream << rule_ptr->get_name() << term.constraint;
        }
    }

    return stream;
}

bool Term::operator==(const Term& other) const {
    if(is_rule() && other.is_rule()){
        auto a = get_rule();
        auto b = other.get_rule();

        return a != nullptr && b != nullptr && *a == *b;

    } else if (is_syntax() && other.is_syntax()){
        return get_syntax() == other.get_syntax();

    } else {
        return false;

    }
}
