#ifndef EXPR_H
#define EXPR_H

#include <utils.h>
#include <lex.h>

class Context;

struct Expr {
    public:
        bool paren = false;

        virtual ~Expr() = default;

        virtual int eval(const Context& context) const = 0;

        virtual void print(std::ostream& stream) const = 0;

        friend std::ostream& operator<<(std::ostream& stream, const Expr& expr) {
            expr.print(stream);
            return stream;
        };
};

struct BinExpr : public Expr {
    public:
        BinExpr(std::string _op, std::unique_ptr<Expr> _left, std::unique_ptr<Expr> _right):
            op(_op),
            left(std::move(_left)),
            right(std::move(_right))
        {}

        int eval(const Context& context) const override;

        void print(std::ostream& stream) const override {
            if(paren){ 
                stream << "(" << *left << " " << op << " " << *right << ")";
            } else {
                stream << *left << " " << op << " " << *right;
            }
        };

    private:
        std::string op;
        std::unique_ptr<Expr> left;
        std::unique_ptr<Expr> right;
};

struct IntExpr : public Expr {
    public:
        IntExpr(int _value):
            value(_value)
        {}

        int eval(const Context& /*context*/) const override {
            return value;
        }

        void print(std::ostream& stream) const override {
            stream << value;
        };

    private:
        int value;

};

struct VarExpr : public Expr {
    public:
        VarExpr(Token_kind _var): 
            var(_var)
        {}

        int eval(const Context& context) const override;

        void print(std::ostream& stream) const override {
            stream << var;
        };

    private:
        Token_kind var;
};


#endif
