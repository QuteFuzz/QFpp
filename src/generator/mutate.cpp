#include <mutate.h>
#include <resource.h>
#include <float_literal.h>
#include <node_gen.h>
#include <qubit_op.h>
#include <grammar.h>
#include <ast.h>
#include <ast_utils.h>
#include <math.h>

static std::shared_ptr<Rule> rule_from_name(const std::string& rule_name, std::shared_ptr<Grammar> grammar){
    auto rule_ptr = grammar->get_rule_pointer_if_exists(rule_name, Scope::GLOB);

    if(rule_ptr == nullptr){
        ERROR("Grammar does not define rule " + rule_name);
    }

    return rule_ptr;
}

static std::unordered_map<Token_kind, Branch_constraint> branch_constraints_for_gate(const Token_kind& gate_kind) {
    return {
        {COMPOUND_STMT, Branch_constraint(QUBIT_OP, 1)},
        {QUBIT_OP, Branch_constraint(GATE_OP, 1)},
        {GATE_NAME, Branch_constraint(gate_kind, 1)}
    };
}

void Mutation_rule::apply(){

    std::vector<std::shared_ptr<Node>> block_nodes;

    for(const auto& node : Node_gen(*root, block_kind)){
        block_nodes.push_back(node);
    }

    if(block_nodes.size() >= 1) {
        // mutate
        unsigned int total = block_nodes.size();
        std::vector<unsigned int> mutated_idxs = {};
        unsigned int n_blocks_to_mutate = std::max(1, (int)(blockwise_rate * total));

        while(mutated_idxs.size() < n_blocks_to_mutate){
            unsigned int idx = random_uint(total - 1);
            
            while(std::find(mutated_idxs.begin(), mutated_idxs.end(), idx) != mutated_idxs.end()){
                idx = random_uint(total - 1);
            }

            mutated_idxs.push_back(idx);

            Slot_type slot = find_slot_for(root, block_nodes[idx]);
            
            if (slot != nullptr) {
                assert((*slot)->get_node_kind() == block_kind);
                apply_blockwise(slot);
            }
        }
    }
}

/// fresh search to count over live number of blocks
unsigned int Mutation_rule::n_children_across_blocks(){
    unsigned int out = 0;

    for(const auto& node : Node_gen(*root, block_kind)){
        out += node->size();
    }

    return out;
}

/**
 *          SEMANTICS MODIFYING
 */

/// @brief Add child to block
/// @param compound_stmts 
void Add_child::apply_blockwise(Slot_type block) {
    std::shared_ptr<Rule> rule = rule_from_name(block_rule_name, grammar);
    build_ast_children(block, rule, *entry.context, nested_depth, 1, descendant_node_branch_constraints);
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
void Add_or_erase_child::apply_blockwise(Slot_type block) {
    float upper_bound = 60.0f;
    float total_children = (float)n_children_across_blocks();

    float random_float = (float)random_uint(upper_bound, 0) / upper_bound;
    float insert_prob = 1.0f - std::min(1.0f, total_children / upper_bound);

    if (insert_prob > random_float){
        Add_child(entry, grammar, block_kind, block_rule_name, blockwise_rate).apply_blockwise(block);

    } else {
        Erase_child(entry, block_kind, blockwise_rate).apply_blockwise(block);

    }
}

void Replace_block::apply_blockwise(Slot_type block) {
    std::shared_ptr<Rule> rule = rule_from_name(repl_rule_name, grammar);
    std::shared_ptr<Node> new_node = build_ast_from_rule(rule, *entry.context, descendant_node_branch_constraints);
    replace_node(block, new_node);
}

void Mutate_on_condition::apply_blockwise(Slot_type block) {
    if (cond(block)) {
        mut_rule->apply_blockwise(block);
    }
}

void Add_gate_chain::apply_blockwise(Slot_type block) {
    std::shared_ptr<Rule> rule = rule_from_name("compound_stmts", grammar);

    size_t chain_size = gate_kinds.size();

    assert(chain_size >= 1);

    std::unordered_map<Token_kind, Branch_constraint> descendant_node_branch_constraints = branch_constraints_for_gate(gate_kinds[0]);

    // need to deref immediately because `build_ast_children` modifies the AST => might lead to child vector reallocs which frees the ptrs in the children vector
    // so derefing later would cause use-after-free error
    std::shared_ptr<Node> last_built_gate_0 = *build_ast_children(block, rule, *entry.context, 0, 1, descendant_node_branch_constraints);

    for (size_t i = 1; i < chain_size; i++){
        descendant_node_branch_constraints = branch_constraints_for_gate(gate_kinds[i]);
        Slot_type last_built_gate_1 = build_ast_children(block, rule, *entry.context, 0, 1, descendant_node_branch_constraints);

        move_qubits(last_built_gate_0, last_built_gate_1);
    }
}

void CCNOT::apply_blockwise(Slot_type block) {
    std::shared_ptr<Rule> rule = rule_from_name("compound_stmts", grammar);
    
    auto descendant_node_branch_constraints = branch_constraints_for_gate(CX);

    std::shared_ptr<Node> cx_node = build_ast_from_rule(rule, *entry.context, descendant_node_branch_constraints);

    auto resources = resources_from_anscestor(*cx_node, QUBIT);

}
