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

/// @brief Add child / children to compound_stmts node
/// @param compound_stmts 
void Statement_insertion::apply_blockwise(std::shared_ptr<Node> compound_stmts) {

    // only insert if compound_stmts node isn't too big
    if(compound_stmts->size() > QuteFuzz::MAX_NODES.at(COMPOUND_STMTS)){
        return;
    }

    auto rule = grammar->get_rule_pointer_if_exists("compound_stmt", Scope::GLOB);

    if (rule == nullptr){
        WARNING("Cannot perform statement insertion! Grammar does not define compound_stmt rule");
    } else {
        // this build setup resets context at RL_CIRCUIT, so resource usages and depths are as if new
        // statement insertions limited to a nested depth of 1, to prevent compound_stmts block growth from going out of hand
        std::shared_ptr<Ast> ast_builder = std::make_shared<Ast>(*entry.context, nested_depth);

        // std::cout << "applying" << std::endl;
        for (unsigned int i = 0; i < n_stmts; i++){
            auto maybe_node = ast_builder->build(rule);

            if (maybe_node.is_ok()){
                std::shared_ptr<Node> node = maybe_node.get_ok();

                assert(node != nullptr);

                #if 0
                std::cout << "new node ====> ";
                node->print_program(std::cout);
                std::cout << std::endl;
                #endif

                compound_stmts->add_child(node);

            } else {
                WARNING("Could not create replacement node");
            }
        }
    }

}

/// @brief Remove random child from compound_stmts node
/// @param compound_stmts 
void Statement_deletion::apply_blockwise(std::shared_ptr<Node> compound_stmts) {
    unsigned int n_children = compound_stmts->size();
    
    if (n_children > 1){
        // only remove children if there's at least 2
        unsigned int idx = random_uint(n_children - 1);
        compound_stmts->erase_child(idx);
    }
}

/// @brief Prefer insertion for small nodes, deletion for big nodes
/// @param compound_stmts 
void Statement_mutation::apply_blockwise(std::shared_ptr<Node> compound_stmts) {
    unsigned upper_bound = QuteFuzz::MAX_NODES.at(COMPOUND_STMTS);

    float random_float = (float)random_uint(upper_bound, 0) / (float)upper_bound;
    float insert_prob = 1.0f - std::pow(std::min(1.0f, (float)block_nodes.size() / upper_bound), 2);

    // std::cout << "insert prob " << insert_prob << " rand f " << random_float << std::endl;

    if (insert_prob > random_float){
        Statement_insertion(entry, grammar, 1).apply_blockwise(compound_stmts);
        Statement_insertion(entry, grammar, 1, 1).apply_blockwise(compound_stmts);

    } else {
        Statement_deletion(entry, grammar).apply_blockwise(compound_stmts);

    }
}
