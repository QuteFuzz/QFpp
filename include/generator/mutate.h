#ifndef MUTATE_H
#define MUTATE_H

#include <node.h>
#include <cassert>
#include <ast.h>

class Mutation_rule {
    public:
        Mutation_rule(Ast_entry& _entry, std::shared_ptr<Grammar> _grammar, Token_kind _block_kind, float _blockwise_rate):
            entry(_entry),
            grammar(_grammar),
            block_kind(_block_kind),
            blockwise_rate(_blockwise_rate)
        {}

        void apply();

        Token_kind get_block_kind() const { return block_kind; }

        unsigned int n_children_across_blocks() const;

        /// function to apply on each block to be mutated by this rule
        virtual void apply_blockwise(Slot_type block_node) const = 0;

        virtual ~Mutation_rule() = default;

    protected:
        Ast_entry entry;
        std::shared_ptr<Grammar> grammar;
        Token_kind block_kind;
        float blockwise_rate;
};

class Add_child : public Mutation_rule {
    public:
        Add_child(Ast_entry& _entry, std::shared_ptr<Grammar> _grammar, Token_kind _block_kind, 
            std::string _block_rule_name,
            float _blockwise_rate,
            unsigned int _nested_depth = 0,
            const std::unordered_map<Token_kind, Branch_constraint>& _descendant_node_branch_constraints = {}
        ):
            Mutation_rule(_entry, _grammar, _block_kind, _blockwise_rate),
            block_rule_name(_block_rule_name),
            nested_depth(_nested_depth),
            descendant_node_branch_constraints(_descendant_node_branch_constraints)
        {}

        void apply_blockwise(Slot_type block) const override;

    private:
        std::string block_rule_name;
        unsigned int nested_depth;
        std::unordered_map<Token_kind, Branch_constraint> descendant_node_branch_constraints;
};

class Erase_child : public Mutation_rule {
    public:
        Erase_child(Ast_entry& _entry, Token_kind _block_kind, float _blockwise_rate = 0.1f):
            Mutation_rule(_entry, nullptr, _block_kind, _blockwise_rate)
        {}

        void apply_blockwise(Slot_type compound_stmts) const override;

    private:

};

class Replace_block : public Mutation_rule {
    public:
        Replace_block(
            Ast_entry& _entry, 
            std::shared_ptr<Grammar> _grammar, 
            Token_kind block_kind, 
            std::string _repl_rule_name,
            float _blockwise_rate,
            const std::unordered_map<Token_kind, Branch_constraint>& _descendant_node_branch_constraints = {}
        ) :
            Mutation_rule(_entry, _grammar, block_kind, _blockwise_rate),
            repl_rule_name(_repl_rule_name),
            descendant_node_branch_constraints(_descendant_node_branch_constraints)
        {}

        void apply_blockwise(Slot_type block) const override;

    private:
        std::string repl_rule_name;
        std::unordered_map<Token_kind, Branch_constraint> descendant_node_branch_constraints;
};

class Mutate_on_condition : public Mutation_rule {
    public:
        // Takes a generic condition based on the block being evaluated
        Mutate_on_condition(std::shared_ptr<Mutation_rule> _mut_rule, std::function<bool(Slot_type)> _cond) :
            Mutation_rule(*_mut_rule),
            mut_rule(_mut_rule),
            cond(_cond)
        {}

        void apply_blockwise(Slot_type block) const override;

    private:
        std::shared_ptr<Mutation_rule> mut_rule;
        std::function<bool(Slot_type)> cond;
};

class Add_gate_chain : public Mutation_rule {
    public:
        Add_gate_chain(Ast_entry& _entry, std::shared_ptr<Grammar> _grammar, float _blockwise_rate, const std::vector<Token_kind>& _chain):
            Mutation_rule(_entry, _grammar, COMPOUND_STMTS, _blockwise_rate),
            chain(_chain)
        {
            assert(chain.size() >= 1);

            unsigned int n_qubits = find_gate_info(chain[0])->n_qubits;

            for (size_t i = 1; i < chain.size(); i++){
                if(find_gate_info(chain[i])->n_qubits != n_qubits){
                    ERROR("All gates in the chain must expect the same number of qubits");
                }
            }
        }

        void apply_blockwise(Slot_type block) const override;

    private:
        std::vector<Token_kind> chain;

};

class Remove_gate_chain : public Mutation_rule {
    public:
        Remove_gate_chain(Ast_entry& _entry, float _blockwise_rate, std::function<bool(Token_kind, Token_kind)> _func):
            Mutation_rule(_entry, nullptr, COMPOUND_STMTS, _blockwise_rate),
            func(_func)
        {}

        void apply_blockwise(Slot_type block) const override;

    private:
        std::function<bool(Token_kind, Token_kind)> func;
};

class Combine : public Mutation_rule {
    public:
        Combine(Ast_entry& _entry, float _blockwise_rate, std::vector<std::unique_ptr<Mutation_rule>> _rules):
            Mutation_rule(_entry, nullptr, COMPOUND_STMTS, _blockwise_rate),
            rules(std::move(_rules))
        {}

        void apply_blockwise(Slot_type type) const override;
        
    private:
        std::vector<std::unique_ptr<Mutation_rule>> rules;

};

#endif
