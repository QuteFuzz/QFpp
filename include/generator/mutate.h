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

static Slot_type find_slot_for(std::shared_ptr<Node>& search_root, std::shared_ptr<Node>& target) {
    for (auto& child : search_root->get_children()) {
        if (child.get() == target.get()) return &child;
        auto found = find_slot_for(child, target);
        if (found) return found;
    }
    return nullptr;
}

class Mutation_rule {
    public:
        // Mutation_rule(){}

        Mutation_rule(Ast_entry& _entry, std::shared_ptr<Grammar> _grammar, Token_kind _block_kind, float _blockwise_rate, bool _on_entire_ast = false):
            entry(_entry),
            grammar(_grammar),
            block_kind(_block_kind),
            blockwise_rate(_blockwise_rate),
            root(_on_entire_ast ? entry.ast : *entry.ast->get_compilation_unit())
        {}

        void apply(){

            std::vector<std::shared_ptr<Node>> block_nodes;

            for(const auto& node : Node_gen(*root, block_kind)){
                block_nodes.push_back(node);
            }

            // mutate
            unsigned int total = block_nodes.size();
            std::vector<unsigned int> mutated_idxs = {};

            while(mutated_idxs.size() < (blockwise_rate * total)){
                unsigned int idx = random_uint(total - 1);
                
                while(std::find(mutated_idxs.begin(), mutated_idxs.end(), idx) != mutated_idxs.end()){
                    idx = random_uint(total - 1);
                }

                mutated_idxs.push_back(idx);

                Slot_type slot = find_slot_for(root, block_nodes[idx]);
                
                if (slot != nullptr) {
                    apply_blockwise(slot);
                }
            }
        }

        /// fresh search to count over live number of blocks
        unsigned int n_children_across_blocks(){
            unsigned int out = 0;

            for(const auto& node : Node_gen(*root, block_kind)){
                out += node->size();
            }

            return out;
        }

        /// function to apply on each block to be mutated by this rule
        virtual void apply_blockwise(Slot_type block_node) = 0;

        virtual ~Mutation_rule() = default;

    protected:
        Ast_entry entry;
        std::shared_ptr<Grammar> grammar;
        Token_kind block_kind;
        float blockwise_rate;
        std::shared_ptr<Node> root;
};

/**
 *          SEMNATICS MODIFYING
 */

class Add_children : public Mutation_rule {

    public:
        Add_children(Ast_entry& _entry, std::shared_ptr<Grammar> _grammar, Token_kind _block_kind, std::string _rule_name, float _blockwise_rate = 0.1f, unsigned int _nested_depth = 0):
            Mutation_rule(_entry, _grammar, _block_kind, _blockwise_rate),
            rule_name(_rule_name),
            nested_depth(_nested_depth)
        {}

        void apply_blockwise(Slot_type block) override;

    private:
        std::string rule_name;
        unsigned int nested_depth;

};

class Erase_child : public Mutation_rule {

    public:
        Erase_child(Ast_entry& _entry, std::shared_ptr<Grammar> _grammar, Token_kind _block_kind, float _blockwise_rate = 0.1f):
            Mutation_rule(_entry, _grammar, _block_kind, _blockwise_rate)
        {}

        void apply_blockwise(Slot_type compound_stmts) override;

    private:

};

class Mutate_children : public Mutation_rule {

    public:
        Mutate_children(Ast_entry& _entry, std::shared_ptr<Grammar> _grammar, Token_kind block_kind, std::string _rule_name, float _blockwise_rate = 0.1f):
            Mutation_rule(_entry, _grammar, block_kind, _blockwise_rate),
            rule_name(_rule_name)
        {}

        void apply_blockwise(Slot_type block) override;

    private:
        std::string rule_name;

};

class Replace_block : public Mutation_rule {

    public:
        Replace_block(Ast_entry& _entry, std::shared_ptr<Grammar> _grammar, Token_kind block_kind, std::string _repl_rule_name, float _blockwise_rate = 0.1f) :
            Mutation_rule(_entry, _grammar, block_kind, _blockwise_rate),
            repl_rule_name(_repl_rule_name)
        {}

        void apply_blockwise(Slot_type block) override;

    private:
        std::string repl_rule_name;
};

class Replace_with_multi_qubit_ops : public Mutation_rule {

    public:
        Replace_with_multi_qubit_ops(Ast_entry& _entry, std::shared_ptr<Grammar> _grammar, float _blockwise_rate = 0.1f):
            Mutation_rule(_entry, _grammar, GATE_OP, _blockwise_rate),
            repl_rule_name("gate_op")
        {}

        void apply_blockwise(Slot_type block) override;

    private:
        std::string repl_rule_name;
};

#endif
