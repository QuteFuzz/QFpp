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

using Rule_list = std::vector<std::shared_ptr<Rule>>;
using Expr_type = std::variant<int, bool, std::string, std::shared_ptr<Rule>, Rule_list>;

class Expr {
    public:
        bool paren = false;

        Expr(){}

        virtual ~Expr() = default;

        virtual Expr_type eval(Context&) const { return 0; }

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

        Expr_type eval(Context&) const override;

        void print(std::ostream& stream) const override;

    private:
        int value;

};

class VarExpr : public Expr {
    public:
        VarExpr(Token_kind _var): 
            var(_var)
        {}

        Expr_type eval(Context& context) const override;

        void print(std::ostream& stream) const override;

    private:
        Token_kind var;
};

class RuleExpr : public Expr {
    public:
        RuleExpr(std::string _rule_name, std::shared_ptr<Rule> _rule):
            rule_name(_rule_name), 
            rule(_rule)
        {}

        Expr_type eval(Context& context) const override;

        void print(std::ostream& stream) const override;

    private:
        std::string rule_name;
        std::shared_ptr<Rule> rule;
};

class BlockExpr : public Expr {

    public:
        BlockExpr(std::vector<std::unique_ptr<Expr>> exprs): 
            expressions(std::move(exprs))
        {}

        Expr_type eval(Context& context) const override;

        void print(std::ostream& stream) const override;

    private:
        std::vector<std::unique_ptr<Expr>> expressions;
};

class PropertyAccessExpr : public Expr {
    public:
        PropertyAccessExpr(std::string obj, std::string prop) :
            obj_name(std::move(obj)), 
            prop_name(std::move(prop))
        {}

        Expr_type eval(Context& context) const override;

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
        {}

        Expr_type eval(Context& context) const override;

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
            assert((true_branch != nullptr) && (cond != nullptr));
        }

        Expr_type eval(Context& context) const override;

        void print(std::ostream& stream) const override;

    private:
        std::unique_ptr<Expr> cond;
        std::unique_ptr<Expr> true_branch;
        std::unique_ptr<Expr> false_branch;
};

class ForExpr : public Expr {
    public:
        ForExpr(const std::string& var, const std::string& iter, std::unique_ptr<Expr> b):
            iter_var(var),
            iterable(iter),
            body(std::move(b)) 
        {
            if (iter != "ALL_QUBITS" && iter != "ALL_BITS" && iter != "ALL_QUBIT_DEFS" && iter != "ALL_BIT_DEFS"){
                ERROR("Unknown iterable " + iter);
            }
        }

        Expr_type eval(Context& context) const override;

        void print(std::ostream& stream) const override;

    private:
        std::string iter_var;
        std::string iterable; 
        std::unique_ptr<Expr> body;

};

class AssignExpr : public Expr {
    
    public:
        AssignExpr(std::string _actual_name, std::shared_ptr<Rule> _temp_rule) :
            actual_name(_actual_name),
            temp_rule(_temp_rule)
        {}

        Expr_type eval(Context& context) const override;

        void print(std::ostream& stream) const override;

    private:
        std::string actual_name;
        std::shared_ptr<Rule> temp_rule;

};

#endif
