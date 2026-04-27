#include <context.h>
#include <expr.h>
#include <resource.h>

int IntExpr::eval_int(Context& /*context*/) const {
    return value;
}

void IntExpr::print(std::ostream& stream) const {
    stream << value;
};

int VarExpr::eval_int(Context& context) const {
    return context.resolve_var(var);
}

void VarExpr::print(std::ostream& stream) const {
    stream << var;
};

std::shared_ptr<Rule> RuleExpr::eval_rule(Context& /*context*/) const {
    return rule;
}

void RuleExpr::print(std::ostream& stream) const {
    stream << rule->get_name();
} 

bool PropertyAccessExpr::eval_bool(Context& context) const {
    std::shared_ptr<Resource> resource = context.get_resource_bound_to(obj_name);

    if (resource == nullptr) return false;

    if (prop_name == "is_singular") {
        return resource->get_str() != "register_resource";
    }

    return false;
}

void PropertyAccessExpr::print(std::ostream& stream) const {
    stream << obj_name << "." << prop_name;
}


int BinExpr::eval_int(Context& context) const {
    int left_eval = left->eval_int(context);
    int right_eval = right->eval_int(context);

    if (op == "UNIFORM"){
        return random_uint(right_eval, left_eval);
    } else if (op == "+") {
        return left_eval + right_eval;
    } else if (op == "-"){
        return left_eval - right_eval;
    } else if (op == ">="){
        return left_eval >= right_eval;
    } else if (op == "<="){
        return left_eval <= right_eval;
    } else if (op == "=="){
        return left_eval == right_eval;
    } else if (op == "!="){
        return left_eval != right_eval;
    } else if (op == "*"){
        return left_eval * right_eval;
    } else if (op == "/"){
        return left_eval / right_eval;
    } else if (op == ">"){
        return left_eval > right_eval;
    } else if (op == "<"){
        return left_eval < right_eval;
    } else {
        ERROR("Unknown binary op " + op);
    }
}

void BinExpr::print(std::ostream& stream) const {
    if(paren){ 
        stream << "(" << *left << " " << op << " " << *right << ")";
    } else {
        stream << *left << " " << op << " " << *right;
    }
}

std::shared_ptr<Rule> IfExpr::eval_rule(Context& context) const {
    if (cond->eval_bool(context)) {
        return true_branch->eval_rule(context);
    } else {
        return false_branch->eval_rule(context);
    }
}

void IfExpr::print(std::ostream& stream) const {
    stream << "if (" << *cond << "): \n" << *true_branch << " else: \n" << *false_branch;
}

std::vector<std::shared_ptr<Rule>> ForExpr::eval_rule_list(Context& context) const {
    Resource_kind rk;

    if (iterable == ALL_QUBITS){
        rk = Resource_kind::QUBIT;
    } else {
        rk = Resource_kind::BIT;
    }

    auto items = context.get_current_resources(rk);
    
    std::vector<std::shared_ptr<Rule>> yielded_rules;

    for (const auto res : items) {
        context.push_var(iter_var, res);
        
        yielded_rules.push_back(body->eval_rule(context));
        
        context.pop_var(iter_var);
    }
    
    return yielded_rules;
}

void ForExpr::print(std::ostream& stream) const {
    stream << "for " << iter_var << " in " << iterable << ": \n"
    << *body;
}

