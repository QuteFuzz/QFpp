#include <mutate.h>
#include <resource.h>
#include <float_literal.h>
#include <node_gen.h>
#include <qubit_op.h>
#include <grammar.h>
#include <ast.h>

/**
 *          SEMANTICS MODIFYING
 */

/// @brief Add child / children to block
/// @param compound_stmts 
void Add_children::apply_blockwise(Slot_type block) {
    std::shared_ptr<Rule> rule = grammar->get_rule_pointer_if_exists(rule_name, Scope::GLOB);

    if (rule == nullptr){
        WARNING("Cannot perform child addition! Grammar does not define rule called " + rule_name);
    
    } else {
        // this build setup resets context at RL_CIRCUIT, so resource usages and depths are as if new
        std::shared_ptr<Ast> ast_builder = std::make_shared<Ast>(*entry.context, nested_depth);
        const Term& term = ast_builder->make_term_from_rule(rule);

        /*
            n_stmts here is not doing what is intended
            already, the branch from compound_stmts contains more than one compound_stmt term

            compound_stmts = (compound_stmt NEWLINE)[UNIFORM(3,5)];

            a solution could be to limit term constraint node additions using node constaints, then i can set
            a node constraint on the compound_stmts node here to be able to control things.

            can also check that n_stmts is <= min possible value from term constraint resolution
        */
        // for (size_t i = 0; i < n_stmts; i++);
        ast_builder->term_branch_to_child_nodes(*block, term);
    }
}

/// @brief Remove random child from block
/// @param compound_stmts 
void Erase_child::apply_blockwise(Slot_type block) {
    unsigned int n_children = (*block)->size();
    
    if (n_children > 1){
        // only remove random child if there's at least 2
        unsigned int idx = random_uint(n_children - 1);
        (*block)->erase_child(idx);
    }
}

/// @brief Prefer insertion for small nodes, deletion for big nodes
/// @param compound_stmts 
void Mutate_children::apply_blockwise(Slot_type block) {
    float upper_bound = 60.0f;
    float total_children = (float)n_children_across_blocks();

    float random_float = (float)random_uint(upper_bound, 0) / upper_bound;
    float insert_prob = 1.0f - std::min(1.0f, total_children / upper_bound);

    // std::cout << "total children " << total_children << std::endl; getchar();

    if (insert_prob > random_float){
        Add_children(entry, grammar, block_kind, rule_name, blockwise_rate, 0).apply_blockwise(block);

    } else {
        Erase_child(entry, grammar, block_kind, blockwise_rate).apply_blockwise(block);

    }
}

void Replace_block::apply_blockwise(Slot_type block) {
    std::shared_ptr<Rule> rule = grammar->get_rule_pointer_if_exists(repl_rule_name, Scope::GLOB);
    std::shared_ptr<Ast> ast_builder = std::make_shared<Ast>(*entry.context, 0);
    Result<std::shared_ptr<Node>> maybe_new_block = ast_builder->build(rule);

    if (maybe_new_block.is_ok()){
        *block = maybe_new_block.get_ok();
    }
}

void Replace_with_multi_qubit_ops::apply_blockwise(Slot_type block) {
    Replace_block(entry, grammar, block_kind, repl_rule_name, blockwise_rate).apply_blockwise(block);
    std::shared_ptr<Node> gate_name = (*block)->find(GATE_NAME);

    if (gate_name == nullptr){
        ERROR("Gate op node must have gate name as a descendant");

    } else {
        std::shared_ptr<Gate> gate = std::dynamic_pointer_cast<Gate>(gate_name->child_at(0));

        if (gate == nullptr){
            ERROR("Child of gate name must have node kind of GATE");
        } else {
            if (gate->get_num_external_qubits() == 1){
                apply_blockwise(block);
            } else {
                // std::cout << "replacement gate " << std::endl;
                // gate->print_program(std::cout);
                // std::cout << "====" << std::endl;
            }
        }
    }
}
