#ifndef MUTATE_H
#define MUTATE_H

#include <node.h>
#include <cassert>

namespace QuteFuzz {
    static const std::set<Token_kind> X_BASIS = {X, RX};
    static const std::set<Token_kind> Y_BASIS = {Y, RY};
    static const std::set<Token_kind> Z_BASIS = {Z, RZ, S, T};
};

class Mutation_rule {
    public:
        Mutation_rule(){}

        virtual void apply(std::shared_ptr<Node>& compound_stmts) = 0;

        virtual ~Mutation_rule() = default;

    protected:
        std::shared_ptr<Node> empty = std::make_shared<Node>("");
};

class Sequence_rule : public Mutation_rule {
    public:
        Sequence_rule(){}

        void apply(std::shared_ptr<Node>& compound_stmts) override {
            for(auto rule : rules){
                rule->apply(compound_stmts);
            }
        }

        void add_rule(std::shared_ptr<Mutation_rule> rule){
            rules.push_back(rule);
        }

        std::vector<std::shared_ptr<Mutation_rule>> get_rules() const {
            return rules;
        }

    private:
        std::vector<std::shared_ptr<Mutation_rule>> rules;
};

class Commutation_rule : public Mutation_rule {

    public:
        Commutation_rule(std::set<Token_kind> _basis):
            Mutation_rule(),
            basis(_basis)
        {}

        void apply(std::shared_ptr<Node>& compound_stmts);

    private:
        std::set<Token_kind> basis;
};

class Gate_fission : public Mutation_rule {

    public:
        Gate_fission():
            Mutation_rule()
        {}

        void apply(std::shared_ptr<Node>& compound_stmts);

    private:
};

inline std::shared_ptr<Mutation_rule> operator+(
    std::shared_ptr<Mutation_rule> lhs,
    std::shared_ptr<Mutation_rule> rhs
) {
    auto seq = std::make_shared<Sequence_rule>();

    if (auto lhs_seq = std::dynamic_pointer_cast<Sequence_rule>(lhs)) {
        for (const auto& r : lhs_seq->get_rules()) seq->add_rule(r);
    } else {
        seq->add_rule(lhs);
    }

    if (auto rhs_seq = std::dynamic_pointer_cast<Sequence_rule>(rhs)) {
        for (const auto& r : rhs_seq->get_rules()) seq->add_rule(r);
    } else {
        seq->add_rule(rhs);
    }

    return seq;
}

#endif
