#ifndef MUTATE_H
#define MUTATE_H

#include <node.h>
#include <cassert>
#include <ast.h>

namespace QuteFuzz {
    static const std::set<Token_kind> X_BASIS = {X, RX};
    static const std::set<Token_kind> Y_BASIS = {Y, RY};
    static const std::set<Token_kind> Z_BASIS = {Z, RZ, S, T};
};

class Mutation_rule {
    public:
        // Mutation_rule(){}

        Mutation_rule(Ast_entry& _entry, std::shared_ptr<Grammar> _grammar, Token_kind _block_type, bool _on_entire_ast = false):
            block_type(_block_type),
            entry(_entry),
            grammar(_grammar),
            root(_on_entire_ast ? entry.ast : *entry.ast->get_compilation_unit()),
            block_nodes{}
        {}

        void apply(){
            block_nodes.clear();

            for(const auto& node : Node_gen(*root, block_type)){
                block_nodes.push_back(node);
            }

            if (block_nodes.empty()) return;

            // pick one random block rather than mutating all
            unsigned int idx = random_uint(block_nodes.size() - 1);
            apply_blockwise(block_nodes[idx]);
        }

        unsigned int n_children_across_blocks(){
            unsigned int out = 0;

            for(const auto& node : block_nodes){
                out += node->size();
            }

            return out;
        }

        /// function to apply on each block to be mutated by this rule
        virtual void apply_blockwise(std::shared_ptr<Node> block_node) = 0;

        virtual ~Mutation_rule() = default;

    protected:
        Token_kind block_type;
        Ast_entry entry;
        std::shared_ptr<Grammar> grammar;
        std::shared_ptr<Node> root;

        std::vector<std::shared_ptr<Node>> block_nodes;
};

/**
 *          SEMNATICS MODIFYING
 */

class Statement_insertion : public Mutation_rule {

    public:
        Statement_insertion(Ast_entry& _entry, std::shared_ptr<Grammar> _grammar, unsigned int _nested_depth = 0):
            Mutation_rule(_entry, _grammar, COMPOUND_STMTS),
            n_stmts(0),
            nested_depth(_nested_depth)
        {}

        void apply_blockwise(std::shared_ptr<Node> compound_stmts) override;

    private:
        unsigned int n_stmts;
        unsigned int nested_depth;

};

class Statement_deletion : public Mutation_rule {

    public:
        Statement_deletion(Ast_entry& _entry, std::shared_ptr<Grammar> _grammar):
            Mutation_rule(_entry, _grammar, COMPOUND_STMTS)
        {}

        void apply_blockwise(std::shared_ptr<Node> compound_stmts) override;

    private:

};

class Statement_mutation : public Mutation_rule {

    public:
        Statement_mutation(Ast_entry& _entry, std::shared_ptr<Grammar> _grammar):
            Mutation_rule(_entry, _grammar, COMPOUND_STMTS)
        {}

        void apply_blockwise(std::shared_ptr<Node> compound_stmts) override;

    private:

};

#endif
