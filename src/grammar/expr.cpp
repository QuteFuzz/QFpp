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
    return context.resolve_var(name, args);
}

void VarExpr::print(std::ostream& stream) const {
    stream << name;

    if (args.size()){
        stream << "{";
        for (const auto& arg : args){
            if (std::holds_alternative<int>(arg)){
                stream << std::get<int>(arg) << ", ";
            } else if (std::holds_alternative<std::string>(arg)){
                stream << std::get<std::string>(arg) << ", ";
            }
        }
        stream << "}";
    }
}

Expr_type RuleExpr::eval(Context& context) const {
    if (auto temp_rule = context.get_value_bound_to<Rule>(rule_name)){
        return temp_rule;
    }

    if (rule != nullptr){
        return rule;
    }

    ERROR("RuleExpr evaluation failed: '" + rule_name + "' is not bound in context and is not defined in grammar");
}

void RuleExpr::print(std::ostream& stream) const {
    stream << rule_name;
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
        if (prop_name == "from_reg") {
            return resource->from_reg();
        } else if (prop_name == "from_sing"){
            return !resource->from_reg();
        } else if (prop_name == "in_ext_scope") {
            return resource->get_scope() == Scope::EXT;
        } else if (prop_name == "in_int_scope") {
            return resource->get_scope() == Scope::INT;
        } else if (prop_name == "name") {
            return resource->get_var_name()->get_str();
        } else if (prop_name == "index") {
            return resource->get_index()->get_str();
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
            return resource_def->get_var_name()->get_str();
        } else if (prop_name == "size") {
            return resource_def->get_size()->get_str();
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
    Expr_type left_eval = left->eval(context);
    Expr_type right_eval = right->eval(context);

    static auto resolve_operand = [](const Expr_type& eval)->int{
        if (std::holds_alternative<std::shared_ptr<Rule>>(eval)){
            return (int)std::get<std::shared_ptr<Rule>>(eval)->get_token().kind;
        } else if (std::holds_alternative<int>(eval)){
            return std::get<int>(eval);
        } else if (std::holds_alternative<bool>(eval)){
            return std::get<bool>(eval) ? 1 : 0;
        } else {
            ERROR("Binop operand expected to be int or token!");
        }
    };

    int left = resolve_operand(left_eval);
    int right = resolve_operand(right_eval);

    if (op == "UNIFORM"){
        return (int)uniform_uint(std::max(right, left), std::min(right, left));
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

    bool eval;

    if (std::holds_alternative<bool>(cond_eval)){
        eval = std::get<bool>(cond_eval);
    } else if (std::holds_alternative<int>(cond_eval)){
        eval = std::get<int>(cond_eval) > 0;
    } else {
        ERROR("IfExpr expects cond to return bool or int");
    }

    if (eval) {
        return true_branch->eval(context);
    } else if (false_branch != nullptr) {
        return false_branch->eval(context);
    } else {
        return 0;
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

            auto body_eval = body->eval(context);

            if (std::holds_alternative<std::shared_ptr<Rule>>(body_eval)){
                auto rule = std::get<std::shared_ptr<Rule>>(body_eval);
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
    stream << "for " << iter_var << " in " << iterable << " \n"
    << *body;
}


Expr_type AssignExpr::eval(Context& context) const {
    static int id = 0;
    std::string temp_name = "__temp_assign_" + std::to_string(id++);

    auto dynamic_rule = std::make_shared<Rule>(Token{temp_name, RULE}, Scope::GLOB);

    if (!rule->is_empty()) {
        for (const auto& branch : rule->get_branches()){
            dynamic_rule->add(branch.eval_term_exprs(context));
        }
    }

    context.push_var<Rule>(rule->get_name(), dynamic_rule);

    return 0;
}

void AssignExpr::print(std::ostream& stream) const {
    stream << *rule << std::endl;
}
