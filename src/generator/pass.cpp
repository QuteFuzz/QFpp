#include <pass.h>
#include <resource.h>
#include <node_gen.h>
#include <qubit_op.h>
#include <grammar.h>
#include <ast.h>
#include <ast_utils.h>
#include <math.h>
#include <node_gen.h>

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
        {PRIMITIVE_GATE, Branch_constraint(gate_kind, 1)}
    };
}

void Pass::apply(){
    // apply blockwise on collected blocks
    for (auto& block : block_nodes){
        float mut_prob = uniform_float(1.0, 0.0);

        if(mut_prob < blockwise_rate){
            auto slot = find_slot_for(entry.get_root(consider_entire_ast), block);

            if (slot != nullptr) {
                assert(block->get_node_kind() == block_kind);
                apply_blockwise(slot);
            }
        }
    }
}

/// @brief Add child to block
/// @param compound_stmts 
void Add_child::apply_blockwise(Slot_type block) const {
    std::shared_ptr<Rule> rule = rule_from_name(block_rule_name, grammar);
    build_ast_children(*block, rule, *entry.get_context(), nested_depth, 1, descendant_node_branch_constraints);
}

/// @brief Remove random child from block
/// @param compound_stmts 
void Erase_child::apply_blockwise(Slot_type block) const {
    unsigned int n_children = (*block)->size();
    
    if (n_children > 1){
        // only remove random child if there's at least 2
        unsigned int idx = uniform_uint(n_children - 1);
        (*block)->erase_child(idx);
    }
}

void Replace_block::apply_blockwise(Slot_type block) const  {
    std::shared_ptr<Rule> rule = rule_from_name(repl_rule_name, grammar);
    std::shared_ptr<Node> new_node = build_ast_from_rule(rule, *entry.get_context(), descendant_node_branch_constraints);

    new_node->print_mode = (*block)->print_mode;
    *block= new_node;
}

void Mutate_on_condition::apply_blockwise(Slot_type block) const {
    if (cond(*block)) {
        mut_rule->apply_blockwise(block);
    }
}

void Add_gate_chain::apply_blockwise(Slot_type block) const {
    std::shared_ptr<Rule> rule = rule_from_name("compound_stmts", grammar);

    size_t chain_size = chain.size();

    std::unordered_map<Token_kind, Branch_constraint> descendant_node_branch_constraints = branch_constraints_for_gate(chain[0]);

    // need to deref immediately because `build_ast_children` modifies the AST => might lead to child vector reallocs which frees the ptrs in the children vector
    // so derefing later would cause use-after-free error
    std::shared_ptr<Node> last_built_gate_0 = *build_ast_children(*block, rule, *entry.get_context(), 0, 1, descendant_node_branch_constraints);


    #if 0
    std::cout << "gate 0 added: ";
    (*block)->print_program(std::cout);
    std::cout << "=================" << std::endl;
    #endif

    for (size_t i = 1; i < chain_size; i++){
        descendant_node_branch_constraints = branch_constraints_for_gate(chain[i]);
        Slot_type last_built_gate_1 = build_ast_children(*block, rule, *entry.get_context(), 0, 1, descendant_node_branch_constraints);

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
    for (const auto& pass : passes){
        if (pass->get_block_kind() == (*block)->get_node_kind()){
            pass->apply_blockwise(block);
        }
    }
}

void Dead_subs::apply() {
    reachable_subs.clear();
    // here i add the names of any possible top level cirucit, which could include dummy circuit
    reachable_subs.emplace(QuteFuzz::TOP_LEVEL_CIRCUIT_NAME);
    reachable_subs.emplace("dummy_circuit");

    bool new_reachable_found = true;

    while(new_reachable_found){
        new_reachable_found = false;

        // check all qubit ops in the AST
        for (auto& qubit_op : entry.get_qubit_ops(true)){
            auto gate_node = qubit_op->get_gate_node();

            if (gate_node == nullptr){
                // ERROR("Qubit op must have a gate node attached to it");
                continue;
            }

            if (gate_node->get_node_kind() == SUBROUTINE_OP){
                std::string caller = qubit_op->get_caller_name();
                std::string sub_name = gate_node->get_str();

                // if caller is reachable, then op is also reachable
                if (reachable_subs.count(caller)){
                    if (reachable_subs.insert(sub_name).second){
                        // std::cout << "Caller " << caller << " calls " << sub_name << std::endl;
                        new_reachable_found = true;
                    }
                }
            }
        }
    }

    Pass::apply();
}

void Dead_subs::apply_blockwise(Slot_type block) const {
    auto circuit_node = static_pointer_cast<Circuit>(*block);

    for (const std::string& name : reachable_subs){
        if (circuit_node->get_name() == name){
            // this block is reachable
            return;
        }
    }

    // this block is unreachable
    *block = std::make_shared<Node>("");
}
