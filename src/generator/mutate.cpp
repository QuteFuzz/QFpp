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
    std::shared_ptr<Node> root = entry.get_root();

    // precollect all blocks such that we don't collect added blocks due to mutations
    for(const auto& node : Node_gen(*root, block_kind)){
        block_nodes.push_back(node);
    }

    // apply blockwise on collected blocks
    for (auto& block : block_nodes){
        float mut_prob = random_float(1.0, 0.0) / 1.0;

        if(mut_prob < blockwise_rate){
            Slot_type slot = find_slot_for(root, block);

            if (slot != nullptr) {
                assert((*slot)->get_node_kind() == block_kind);
                apply_blockwise(slot);
            }
        }
    }
}

/// fresh search to count over live number of blocks
unsigned int Mutation_rule::n_children_across_blocks() const {
    unsigned int out = 0;
    auto root = entry.get_root();

    for(const auto& node : Node_gen(*root, block_kind)){
        out += node->size();
    }

    return out;
}


/// @brief Add child to block
/// @param compound_stmts 
void Add_child::apply_blockwise(Slot_type block) const {
    std::shared_ptr<Rule> rule = rule_from_name(block_rule_name, grammar);
    build_ast_children(block, rule, *entry.get_context(), nested_depth, 1, descendant_node_branch_constraints);
}

/// @brief Remove random child from block
/// @param compound_stmts 
void Erase_child::apply_blockwise(Slot_type block) const {
    unsigned int n_children = (*block)->size();
    
    if (n_children > 1){
        // only remove random child if there's at least 2
        unsigned int idx = random_uint(n_children - 1);
        (*block)->erase_child(idx);
    }
}

void Replace_block::apply_blockwise(Slot_type block) const  {
    std::shared_ptr<Rule> rule = rule_from_name(repl_rule_name, grammar);
    std::shared_ptr<Node> new_node = build_ast_from_rule(rule, *entry.get_context(), descendant_node_branch_constraints);
    replace_node(block, new_node);
}

void Mutate_on_condition::apply_blockwise(Slot_type block) const {
    if (cond(block)) {
        mut_rule->apply_blockwise(block);
    }
}

void Add_gate_chain::apply_blockwise(Slot_type block) const {
    std::shared_ptr<Rule> rule = rule_from_name("compound_stmts", grammar);

    size_t chain_size = chain.size();

    std::unordered_map<Token_kind, Branch_constraint> descendant_node_branch_constraints = branch_constraints_for_gate(chain[0]);

    // need to deref immediately because `build_ast_children` modifies the AST => might lead to child vector reallocs which frees the ptrs in the children vector
    // so derefing later would cause use-after-free error
    std::shared_ptr<Node> last_built_gate_0 = *build_ast_children(block, rule, *entry.get_context(), 0, 1, descendant_node_branch_constraints);


    #if 0
    std::cout << "gate 0 added: ";
    (*block)->print_program(std::cout);
    std::cout << "=================" << std::endl;
    #endif

    for (size_t i = 1; i < chain_size; i++){
        descendant_node_branch_constraints = branch_constraints_for_gate(chain[i]);
        Slot_type last_built_gate_1 = build_ast_children(block, rule, *entry.get_context(), 0, 1, descendant_node_branch_constraints);

        # if 0
        std::cout << "gate 1 added: ";
        (*block)->print_program(std::cout);
        std::cout << "=================" << std::endl;
        #endif

        move_qubits(last_built_gate_0, last_built_gate_1);
    }
}

void Remove_gate_chain::apply_blockwise(Slot_type block) const {
    // qubit name -> last qubit op acting on that qubit
    std::unordered_map<std::string, std::shared_ptr<Qubit_op>> last_qubit_op_map;

    auto qubit_ops_gen = Node_gen(**block, QUBIT_OP);

    auto qubit_ops_it = qubit_ops_gen.begin();

    while (qubit_ops_it != qubit_ops_gen.end()){
        auto qubit_op = static_pointer_cast<Qubit_op>(*qubit_ops_it);

        auto [is_interesting, prev_qubit_op] = qubit_op_is_interesting(qubit_op, func, last_qubit_op_map);

        if (is_interesting){
            auto prev_qubit_op_slot = find_slot_for(*block, prev_qubit_op);

            // remove current qubit op and previous qubit op
            *qubit_ops_it = std::make_shared<Node>();
            *prev_qubit_op_slot = std::make_shared<Node>();
        }

        qubit_ops_it++;
    }
}

void Combine::apply_blockwise(Slot_type block) const {
    for (const auto& mut_rule : rules){
        if (mut_rule->get_block_kind() == (*block)->get_node_kind()){
            mut_rule->apply_blockwise(block);
        }
    }
}
