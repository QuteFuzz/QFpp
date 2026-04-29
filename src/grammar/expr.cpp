#include <context.h>
#include <expr.h>
#include <resource.h>

Expr_type IntExpr::eval(Context&) const {
    return value;
}

void IntExpr::print(std::ostream& stream) const {
    stream << value;
};

Expr_type VarExpr::eval(Context& context) const {
    return (int)context.resolve_var(var);
}

void VarExpr::print(std::ostream& stream) const {
    stream << var;
};

Expr_type RuleExpr::eval(Context&) const {
    return rule;
}

void RuleExpr::print(std::ostream& stream) const {
    stream << rule->get_name();
}

Expr_type BlockExpr::eval(Context& context) const {
    if (expressions.empty()) return 0;

    Expr_type last_val = false; 

    for (const auto& expr : expressions) {
        last_val = expr->eval(context); 
    }

    return last_val; 
}

void BlockExpr::print(std::ostream& stream) const {
    stream << "{ ";
    for(const auto& expr : expressions){
        stream << *expr;
    }
    stream << " }";
}


Expr_type PropertyAccessExpr::eval(Context& context) const {

    if (auto resource = context.get_value_bound_to<Resource>(obj_name)){
        if (prop_name == "in_ext_scope") {
            return resource->get_scope() == Scope::EXT;
        } else if (prop_name == "in_int_scope") {
            return resource->get_scope() == Scope::INT;
        } else if (prop_name == "name") {
            return resource->get_name()->get_str();
        } else {
            ERROR("Unknown resource property " + prop_name);
        }

    } else if (auto resource_def = context.get_value_bound_to<Resource_def>(obj_name)) {
        if (prop_name == "is_singular") {
            return !resource_def->is_reg();
        } else if (prop_name == "is_register"){
            return resource_def->is_reg();
        } else if (prop_name == "in_ext_scope") {
            return resource_def->get_scope() == Scope::EXT;
        } else if (prop_name == "in_int_scope") {
            return resource_def->get_scope() == Scope::INT;
        } else if (prop_name == "name") {
            return resource_def->get_name()->get_str();
        } else {
            ERROR("Unknown resource def property " + prop_name);
        }

    } else {
        ERROR(obj_name + " is not bound to any resource or resource def");
    }
}

void PropertyAccessExpr::print(std::ostream& stream) const {
    stream << obj_name << "." << prop_name;
}


Expr_type BinExpr::eval(Context& context) const {
    auto left_eval = left->eval(context);
    auto right_eval = right->eval(context);

    if (std::holds_alternative<int>(left_eval) && std::holds_alternative<int>(right_eval)){
        int right = std::get<int>(right_eval);
        int left = std::get<int>(left_eval);

        if (op == "UNIFORM"){
            return (int)random_uint(right, left);
        } else if (op == "+") {
            return left + right;
        } else if (op == "-"){
            return left - right;
        } else if (op == ">="){
            return left >= right;
        } else if (op == "<="){
            return left <= right;
        } else if (op == "=="){
            return left == right;
        } else if (op == "!="){
            return left != right;
        } else if (op == "*"){
            return left * right;
        } else if (op == "/"){
            return left / right;
        } else if (op == ">"){
            return left > right;
        } else if (op == "<"){
            return left < right;
        } else if (op == "&&"){
            return left && right;
        } else if (op == "||"){
            return left || right;        
        } else {
            ERROR("Unknown binary op " + op);
        }
    } else {
        ERROR("Binop must have lhs and rhs as int");
    }
}

void BinExpr::print(std::ostream& stream) const {
    if(paren){ 
        stream << "(" << *left << " " << op << " " << *right << ")";
    } else {
        stream << *left << " " << op << " " << *right;
    }
}

Expr_type IfExpr::eval(Context& context) const {
    auto cond_eval = cond->eval(context);
    auto true_branch_eval = true_branch->eval(context);

    if (std::holds_alternative<bool>(cond_eval) && std::holds_alternative<std::shared_ptr<Rule>>(true_branch_eval)){
        
        if (std::get<bool>(cond_eval)){
            return true_branch_eval;
        } else if (false_branch != nullptr) {
            auto false_branch_eval = false_branch->eval(context);

            if (std::holds_alternative<bool>(false_branch_eval)){
                return false_branch_eval;
            } else {
                ERROR("IfExpr expects false branch to return a rule");
            }
        } else {
            return std::make_shared<Rule>();
        }

    } else {
        ERROR("IfExpr expects cond to return bool, and true branch to return a rule");
    }    
}

void IfExpr::print(std::ostream& stream) const {
    if (false_branch == nullptr){
        stream << "if (" << *cond << "): \n" << *true_branch;
    } else {
        stream << "if (" << *cond << "): \n" << *true_branch << " else: \n" << *false_branch;
    }
}

Expr_type ForExpr::eval(Context& context) const {
    std::vector<std::shared_ptr<Rule>> yielded_rules;

    auto get_rules = [&]<typename T>(Ptr_coll<T> items){
        for (const auto& res : items) {
            context.push_var<T>(iter_var, res);

            auto rule_eval = body->eval(context);

            if (std::holds_alternative<std::shared_ptr<Rule>>(rule_eval)){
                auto rule = std::get<std::shared_ptr<Rule>>(rule_eval);
                if (rule != nullptr) yielded_rules.push_back(rule);
            }

            context.pop_var(iter_var);
        }
    };

    if (iterable == "ALL_QUBITS" || iterable == "ALL_BITS"){
        Resource_kind rk = iterable == "ALL_QUBITS" ? Resource_kind::QUBIT : Resource_kind::BIT;
        auto items = context.get_current_coll<Resource>(rk);
        get_rules(items);

    } else if (iterable == "ALL_QUBIT_DEFS" || iterable == "ALL_BIT_DEFS") {
        Resource_kind rk = iterable == "ALL_QUBIT_DEFS" ? Resource_kind::QUBIT : Resource_kind::BIT;
        auto items = context.get_current_coll<Resource_def>(rk);
        get_rules(items);

    } else {
        ERROR("Unknown iterable " + iterable);
    }

    return yielded_rules;
}

void ForExpr::print(std::ostream& stream) const {
    stream << "for " << iter_var << " in " << iterable << ": \n"
    << *body;
}

