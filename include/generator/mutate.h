#ifndef MUTATE_H
#define MUTATE_H

#include <node.h>
#include <compound_stmts.h>

class Mutation_rule {
    public:
        Mutation_rule(){}

        virtual bool match(const std::shared_ptr<Node> stmts) = 0;

        virtual void apply(std::shared_ptr<Node>& stmts) = 0;

    protected:
        std::vector<std::shared_ptr<Node>*> slots;
        std::shared_ptr<Node> empty = std::make_shared<Node>("");
};

class Remove_gate : public Mutation_rule {
    public:
        Remove_gate(Token_kind gate_kind):
            Mutation_rule(),
            kind(gate_kind)
        {}

        bool match(const std::shared_ptr<Node> compound_stmts);

        void apply(std::shared_ptr<Node>& compound_stmts);

    private:
        Token_kind kind;
};

#endif
