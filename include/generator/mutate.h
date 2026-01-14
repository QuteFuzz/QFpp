#ifndef MUTATE_H
#define MUTATE_H

#include <node.h>
#include <compound_stmts.h>
#include <cassert>

class Mutation_rule {
    public:
        Mutation_rule(){}

        virtual void apply(std::shared_ptr<Node>& compound_stmts) = 0;

    protected:
        std::shared_ptr<Node> empty = std::make_shared<Node>("");
};

class Commutation_rule : public Mutation_rule {

    public:
        Commutation_rule():
            Mutation_rule()
        {}

        void apply(std::shared_ptr<Node>& compound_stmts);

    private:
};

#endif
