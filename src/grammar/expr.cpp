#include <context.h>
#include <expr.h>

int BinExpr::eval(const Context& context) const {
    int left_eval = left->eval(context);
    int right_eval = right->eval(context);

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

int VarExpr::eval(const Context& context) const {
    return context.resolve_var(var);
}
