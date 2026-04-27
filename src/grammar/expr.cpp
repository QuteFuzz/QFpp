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

    if (auto resource = context.get_value_bound_to<Resource>(obj_name)){
        if (prop_name == "in_ext_scope") {
            return resource->get_scope() == Scope::EXT;
        } else if (prop_name == "in_int_scope") {
            return resource->get_scope() == Scope::INT;
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
    } else if (op == "&&"){
        return left_eval && right_eval;
    } else if (op == "||"){
        return left_eval || right_eval;        
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
    } else if (false_branch != nullptr){
        return false_branch->eval_rule(context);
    } else {
        return nullptr;
    }
}

void IfExpr::print(std::ostream& stream) const {
    if (false_branch == nullptr){
        stream << "if (" << *cond << "): \n" << *true_branch;
    } else {
        stream << "if (" << *cond << "): \n" << *true_branch << " else: \n" << *false_branch;
    }
}

std::vector<std::shared_ptr<Rule>> ForExpr::eval_rule_list(Context& context) const {
    std::vector<std::shared_ptr<Rule>> yielded_rules;

    auto get_rules = [&]<typename T>(Ptr_coll<T> items){
        for (const auto& res : items) {
            context.push_var<T>(iter_var, res);

            auto rule = body->eval_rule(context);

            if (rule != nullptr)
                yielded_rules.push_back(rule);

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

