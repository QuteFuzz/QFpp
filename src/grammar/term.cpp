#include <qf_term.h>
#include <rule.h>
#include <context.h>

Term::Term(const std::shared_ptr<Rule> rule, const Token_kind& _kind, const Print_mode& _print_mode){
    value = std::weak_ptr<Rule>(rule),
    kind = _kind;
    pm = _print_mode;
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

Print_mode Term::get_print_mode() const {
    return pm;
}

bool Term::is_syntax() const {
    return std::holds_alternative<std::string>(value);
}

bool Term::is_rule() const {
    return std::holds_alternative<std::weak_ptr<Rule>>(value);
}

std::ostream& operator<<(std::ostream& stream, const Term& term){
    if(term.is_syntax()){
        stream << std::quoted(term.get_syntax());

    } else {
        auto rule_ptr = term.get_rule();

        if (rule_ptr == nullptr){
            stream << "[[DELETED RULE]]";
        } else {
            stream << RED(rule_ptr->get_name());
        }
    }

    if (term.expr != nullptr){
        stream << YELLOW("[") << *term.expr << YELLOW("]");
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

std::vector<Term> Term::eval_expr(Context& context) const {
	std::vector<Term> child_terms = {};
	
	if (expr == nullptr){
		child_terms = std::vector<Term>(1, *this);
	
	} else {
		Expr_type expr_eval = expr->eval(context);

		if (std::holds_alternative<int>(expr_eval)){
            int size = std::get<int>(expr_eval);
            
            if (size >= 0){
			    child_terms = std::vector<Term>(size, *this);
            } else {
                ERROR("IntExpr must evaluate to +ve value!");
            }

        } else if (std::holds_alternative<bool>(expr_eval)){
            if (std::get<bool>(expr_eval)){
			    child_terms.push_back(*this);
            }

		} else if (std::holds_alternative<std::string>(expr_eval)){
			child_terms.push_back(Term(std::get<std::string>(expr_eval), STRING));

        } else if (std::holds_alternative<std::shared_ptr<Rule>>(expr_eval)){ 
            auto rule = std::get<std::shared_ptr<Rule>>(expr_eval);
            child_terms.push_back(make_term_from_rule(rule));

		} else if (std::holds_alternative<Rule_list>(expr_eval)){
			auto rules = std::get<Rule_list>(expr_eval);

			for (std::shared_ptr<Rule> rule : rules){
                // std::cout << *rule << std::endl;
				child_terms.push_back(make_term_from_rule(rule));
			}
		
		} else {
			ERROR("Expr is expected to have a return type of INT, STR, RULE or RULE_LIST");
		}
	}

	return child_terms;
}

Term make_term_from_rule(std::shared_ptr<Rule> rule_ptr){
	Token_kind kind = rule_ptr->get_token().kind;
	return Term(rule_ptr, kind, Print_mode::DEFAULT);
}
