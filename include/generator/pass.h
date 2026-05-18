#ifndef PASS_H
#define PASS_H

#include <node.h>
#include <cassert>
#include <ast.h>

class Pass {
    public:
        Pass(Ast_entry& _entry, std::shared_ptr<Grammar> _grammar, Token_kind _block_kind, float _blockwise_rate, bool _consider_entire_ast = false):
            entry(_entry),
            grammar(_grammar),
            block_kind(_block_kind),
            blockwise_rate(_blockwise_rate),
            consider_entire_ast(_consider_entire_ast)
        {
            std::shared_ptr<Node> root = entry.get_root(_consider_entire_ast);

            // precollect all blocks such that we don't collect added blocks due to mutations
            for (const auto& block : Node_gen(*root, block_kind)){
                block_nodes.push_back(block);
            }
        }

        Token_kind get_block_kind() const { return block_kind; }

        virtual void apply();

        /// function to apply on each block to be mutated by this rule
        virtual void apply_blockwise(Slot_type block) const = 0;

        virtual ~Pass() = default;

    protected:
        Ast_entry entry;
        std::shared_ptr<Grammar> grammar;
        Token_kind block_kind;
        float blockwise_rate;
        bool consider_entire_ast;
        std::vector<std::shared_ptr<Node>> block_nodes; // collection of all block nodes, not slots, because those may be reallocated during mutation
};

class Add_child : public Pass {
    public:
        Add_child(Ast_entry& _entry, std::shared_ptr<Grammar> _grammar, Token_kind _block_kind, 
            std::string _block_rule_name,
            float _blockwise_rate,
            unsigned int _nested_depth = 0,
            const std::unordered_map<Token_kind, Branch_constraint>& _descendant_node_branch_constraints = {}
        ):
            Pass(_entry, _grammar, _block_kind, _blockwise_rate),
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

class Erase_child : public Pass {
    public:
        Erase_child(Ast_entry& _entry, Token_kind _block_kind, float _blockwise_rate = 0.1f):
            Pass(_entry, nullptr, _block_kind, _blockwise_rate)
        {}

        void apply_blockwise(Slot_type block) const override;

    private:

};

class Replace_block : public Pass {
    public:
        Replace_block(
            Ast_entry& _entry, 
            std::shared_ptr<Grammar> _grammar, 
            Token_kind block_kind, 
            std::string _repl_rule_name,
            float _blockwise_rate,
            const std::unordered_map<Token_kind, Branch_constraint>& _descendant_node_branch_constraints = {}
        ) :
            Pass(_entry, _grammar, block_kind, _blockwise_rate),
            repl_rule_name(_repl_rule_name),
            descendant_node_branch_constraints(_descendant_node_branch_constraints)
        {}

        void apply_blockwise(Slot_type block) const override;

    private:
        std::string repl_rule_name;
        std::unordered_map<Token_kind, Branch_constraint> descendant_node_branch_constraints;
};

class Mutate_on_condition : public Pass {
    public:
        // Takes a generic condition based on the block being evaluated
        Mutate_on_condition(std::shared_ptr<Pass> _mut_rule, std::function<bool(std::shared_ptr<Node>)> _cond) :
            Pass(*_mut_rule),
            mut_rule(_mut_rule),
            cond(_cond)
        {}

        void apply_blockwise(Slot_type block) const override;

    private:
        std::shared_ptr<Pass> mut_rule;
        std::function<bool(std::shared_ptr<Node>)> cond;
};

class Add_gate_chain : public Pass {
    public:
        Add_gate_chain(Ast_entry& _entry, std::shared_ptr<Grammar> _grammar, float _blockwise_rate, const std::vector<Token_kind>& _chain):
            Pass(_entry, _grammar, COMPOUND_STMTS, _blockwise_rate),
            chain(_chain)
        {
            assert(chain.size() >= 1);

            unsigned int n_qubits = find_gate_info(chain[0])->resource_counts.at(Resource_kind::QUBIT);

            for (size_t i = 1; i < chain.size(); i++){
                if(find_gate_info(chain[i])->resource_counts.at(Resource_kind::QUBIT) != n_qubits){
                    ERROR("All gates in the chain must expect the same number of qubits");
                }
            }
        }

        void apply_blockwise(Slot_type block) const override;

    private:
        std::vector<Token_kind> chain;

};

class Remove_gate_chain : public Pass {
    public:
        Remove_gate_chain(Ast_entry& _entry, float _blockwise_rate, std::function<bool(Token_kind, Token_kind)> _func):
            Pass(_entry, nullptr, COMPOUND_STMTS, _blockwise_rate),
            func(_func)
        {}

        void apply_blockwise(Slot_type block) const override;

    private:
        std::function<bool(Token_kind, Token_kind)> func;
};

class Combine : public Pass {
    public:
        Combine(Ast_entry& _entry, float _blockwise_rate, std::vector<std::unique_ptr<Pass>> _passes):
            Pass(_entry, nullptr, COMPOUND_STMTS, _blockwise_rate),
            passes(std::move(_passes))
        {}

        void apply_blockwise(Slot_type block) const override;
        
    private:
        std::vector<std::unique_ptr<Pass>> passes;

};

class Dead_subs : public Pass {
    public:
        Dead_subs(Ast_entry& _entry):
            Pass(_entry, nullptr, CIRCUIT, 1.0, true)
        {}

        void apply_blockwise(Slot_type block) const override;

        void apply() override;

    private:
        std::set<std::string> reachable_subs;
};

#endif
