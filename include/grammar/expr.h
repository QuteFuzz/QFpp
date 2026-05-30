#ifndef EXPR_H
#define EXPR_H

#include <utils.h>
#include <lex.h>

class Context;
class Rule;

using Rule_list = std::vector<std::shared_ptr<Rule>>;
using Expr_type = std::variant<int, bool, float, std::string, std::shared_ptr<Rule>, Rule_list>;

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

class FloatExpr : public Expr {
    public:
        FloatExpr(float _value):
            value(_value)
        {}

        Expr_type eval(Context&) const override;

        void print(std::ostream& stream) const override;

    private:
        float value;
};

class VarExpr : public Expr {
    public:
        VarExpr(Token_kind _name, std::vector<std::unique_ptr<Expr>> _args = {}):
            name(_name),
            args(std::move(_args))
        {}

        Expr_type eval(Context& context) const override;

        void print(std::ostream& stream) const override;

    private:
        Token_kind name;
        std::vector<std::unique_ptr<Expr>> args;
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
        PropertyAccessExpr(std::string obj, Token_kind prop) :
            obj_name(std::move(obj)), 
            prop_name(std::move(prop))
        {}

        Expr_type eval(Context& context) const override;

        void print(std::ostream& stream) const override;

    private:
        std::string obj_name;
        Token_kind prop_name;
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
        ForExpr(const std::string& var, const Token_kind& iter, std::unique_ptr<Expr> b):
            iter_var(var),
            iterable(iter),
            body(std::move(b)) 
        {}

        Expr_type eval(Context& context) const override;

        void print(std::ostream& stream) const override;

    private:
        std::string iter_var;
        Token_kind iterable; 
        std::unique_ptr<Expr> body;

};

class AssignExpr : public Expr {
    
    public:
        AssignExpr(std::shared_ptr<Rule> _rule) :
            rule(_rule)
        {}

        Expr_type eval(Context& context) const override;

        void print(std::ostream& stream) const override;

    private:
        std::shared_ptr<Rule> rule;

};

class ModExpr : public Expr {

    public:
        ModExpr(std::unique_ptr<Expr> _expr, const Token_kind& _modifier):
            expr(std::move(_expr)),
            modifier(_modifier)
        {}

        Expr_type eval(Context& context) const override;

        void print(std::ostream& stream) const override;

    private:
        std::unique_ptr<Expr> expr;
        Token_kind modifier;

};

#endif
