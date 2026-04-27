#ifndef EXPR_H
#define EXPR_H

#include <utils.h>
#include <lex.h>

/*
    # 1. The Entry Point: Can be a comprehension, an if-statement, or a standard math/logic expression
    expr = for_expr 
        | if_expr 
        | logic_expr ;

    # 2. List Comprehension: Evaluates to a std::vector<std::string>
    for_expr = "[" "for" IDENTIFIER "in" iterable_token ":" expr "]" ;

    # 3. If-Else Expression: Evaluates to a string (rule name) or recursively another expression
    if_expr = "if" logic_expr ":" expr "else" ":" expr ;

    # 4. Logical Comparisons (==, >, <, etc.)
    logic_expr = logic_expr "==" math_expr 
            | logic_expr ">" math_expr 
            | math_expr ;

    # 5. Math (+, -)
    math_expr = math_expr "+" term 
            | math_expr "-" term
            | UNIFORM(math_expr, term)
            | term ;

    # 6. Math (*, /)
    term = term "*" factor 
        | term "/" factor
        | factor ;

    # 7. Base Values / Atoms
    factor = "(" expr ")" 
        | IDENTIFIER
        | property_access 
        | METAFUNC           # e.g., ALL_QUBITS
        | NUMBER ;

    # 8. Property Access (New)
    property_access = IDENTIFIER "." IDENTIFIER ;

    # (Lexer-defined tokens)
    iterable_token = "ALL_QUBITS" | "INTERNAL_QUBITS" | "EXTERNAL_QUBITS" | ... ;

*/

class Context;
class Rule;

enum class Expr_kind {
    UNKNOWN,
    INT,
    BOOL,
    RULE,
    RULE_LIST,
};

class Expr {
    public:
        bool paren = false;

        Expr(){}

        virtual ~Expr() = default;

        virtual Expr_kind get_kind() const { return Expr_kind::UNKNOWN; }
        
        // evaluating maths
        virtual int eval_int(Context& ctx) const { return 0; }
        
        // evaluating conditions (e.g., res.is_singular)
        virtual bool eval_bool(Context& ctx) const { return false; }
        
        virtual std::shared_ptr<Rule> eval_rule(Context& ctx) const { return nullptr; }
        
        // yielding a list of rule names to expand from a loop
        virtual std::vector<std::shared_ptr<Rule>> eval_rule_list(Context& ctx) const { return {}; }

        virtual void print(std::ostream& stream) const = 0;

        friend std::ostream& operator<<(std::ostream& stream, const Expr& expr) {
            expr.print(stream);
            return stream;
        };
    };

class IntExpr : public Expr {
    public:
        IntExpr(int _value):
            value(_value)
        {}

        Expr_kind get_kind() const override { return Expr_kind::INT; }

        int eval_int(Context& /*context*/) const override;

        void print(std::ostream& stream) const override;

    private:
        int value;

};

class VarExpr : public Expr {
    public:
        VarExpr(Token_kind _var): 
            var(_var)
        {}

        Expr_kind get_kind() const override { return Expr_kind::INT; }

        int eval_int(Context& context) const override;

        void print(std::ostream& stream) const override;

    private:
        Token_kind var;
};

class RuleExpr : public Expr {
    public:
        RuleExpr(std::shared_ptr<Rule> _rule): 
            rule(_rule)
        {}

        Expr_kind get_kind() const override { return Expr_kind::RULE; }

        std::shared_ptr<Rule> eval_rule(Context& /*ctx*/) const override;

        void print(std::ostream& stream) const override;

    private:
        std::shared_ptr<Rule> rule;
};

class PropertyAccessExpr : public Expr {
    public:
        PropertyAccessExpr(std::string obj, std::string prop) :
            obj_name(std::move(obj)), 
            prop_name(std::move(prop)) 
        {}

        Expr_kind get_kind() const override { return Expr_kind::BOOL; }

        bool eval_bool(Context& ctx) const override;

        void print(std::ostream& stream) const override;

    private:
        std::string obj_name;
        std::string prop_name;

};

class BinExpr : public Expr {
    public:
        BinExpr(std::string _op, std::unique_ptr<Expr> _left, std::unique_ptr<Expr> _right):
            op(_op),
            left(std::move(_left)),
            right(std::move(_right))
        {
            if (left->get_kind() != Expr_kind::INT || right->get_kind() != Expr_kind::INT){
                ERROR("Binary expr must have lhs and rhs as integers");
            }
        }

        Expr_kind get_kind() const override { return Expr_kind::INT; }

        int eval_int(Context& ctx) const override;

        void print(std::ostream& stream) const override;

    private:
        std::string op;
        std::unique_ptr<Expr> left;
        std::unique_ptr<Expr> right;
};

class IfExpr : public Expr {
    public:
        IfExpr(std::unique_ptr<Expr> c, std::unique_ptr<Expr> t, std::unique_ptr<Expr> f):
            cond(std::move(c)),
            true_branch(std::move(t)),
            false_branch(std::move(f)) 
        {
            if (cond->get_kind() != Expr_kind::BOOL || true_branch->get_kind() != Expr_kind::RULE || false_branch->get_kind() != Expr_kind::RULE){
                ERROR("If expr expeceted conditional to return bool, and true and false branches to return string");
            }
        }

        Expr_kind get_kind() const override { return Expr_kind::RULE; }

        std::shared_ptr<Rule> eval_rule(Context& ctx) const override;

        void print(std::ostream& stream) const override;

    private:
        std::unique_ptr<Expr> cond;
        std::unique_ptr<Expr> true_branch;
        std::unique_ptr<Expr> false_branch;
};

class ForExpr : public Expr {
    public:
        ForExpr(const std::string& var, const Token& iter, std::unique_ptr<Expr> b):
            iter_var(var),
            iterable(iter.kind),
            body(std::move(b)) 
        {
            if (body->get_kind() != Expr_kind::RULE){
                ERROR("For expr body must return RULE type");
            }

            if (iter.kind != ALL_QUBITS && iter.kind != ALL_BITS){
                ERROR("Unknown iterable " + iter.value);
            }
        }

        Expr_kind get_kind() const override { return Expr_kind::RULE_LIST; }

        std::vector<std::shared_ptr<Rule>> eval_rule_list(Context& context) const override;

        void print(std::ostream& stream) const override;

    private:
        std::string iter_var;
        Token_kind iterable; 
        std::unique_ptr<Expr> body;

};


#endif
