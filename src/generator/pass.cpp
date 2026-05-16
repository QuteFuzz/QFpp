#include <pass.h>
#include <resource.h>
#include <float_literal.h>
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
        {GATE_NAME, Branch_constraint(gate_kind, 1)}
    };
}

void Pass::apply(){
    // apply blockwise on collected blocks
    for (auto& block_it : block_node_iterators){
        float mut_prob = random_float(1.0, 0.0);

        if(mut_prob < blockwise_rate){
            auto block_ptr = *block_it;

            if (block_ptr != nullptr) {
                assert(block_ptr->get_node_kind() == block_kind);
                apply_blockwise(block_it);
            }
        }
    }
}

/// @brief Add child to block
/// @param compound_stmts 
void Add_child::apply_blockwise(Node_gen::Iterator block_node_it) const {
    std::shared_ptr<Rule> rule = rule_from_name(block_rule_name, grammar);
    build_ast_children(*block_node_it, rule, *entry.get_context(), nested_depth, 1, descendant_node_branch_constraints);
}

/// @brief Remove random child from block
/// @param compound_stmts 
void Erase_child::apply_blockwise(Node_gen::Iterator block_node_it) const {
    unsigned int n_children = (*block_node_it)->size();
    
    if (n_children > 1){
        // only remove random child if there's at least 2
        unsigned int idx = random_uint(n_children - 1);
        (*block_node_it)->erase_child(idx);
    }
}

void Replace_block::apply_blockwise(Node_gen::Iterator block_node_it) const  {
    std::shared_ptr<Rule> rule = rule_from_name(repl_rule_name, grammar);
    std::shared_ptr<Node> new_node = build_ast_from_rule(rule, *entry.get_context(), descendant_node_branch_constraints);

    new_node->print_mode = (*block_node_it)->print_mode;
    *block_node_it= new_node;
}

void Mutate_on_condition::apply_blockwise(Node_gen::Iterator block_node_it) const {
    if (cond(*block_node_it)) {
        mut_rule->apply_blockwise(block_node_it);
    }
}

void Add_gate_chain::apply_blockwise(Node_gen::Iterator block_node_it) const {
    std::shared_ptr<Rule> rule = rule_from_name("compound_stmts", grammar);

    size_t chain_size = chain.size();

    std::unordered_map<Token_kind, Branch_constraint> descendant_node_branch_constraints = branch_constraints_for_gate(chain[0]);

    // need to deref immediately because `build_ast_children` modifies the AST => might lead to child vector reallocs which frees the ptrs in the children vector
    // so derefing later would cause use-after-free error
    std::shared_ptr<Node> last_built_gate_0 = *build_ast_children(*block_node_it, rule, *entry.get_context(), 0, 1, descendant_node_branch_constraints);


    #if 0
    std::cout << "gate 0 added: ";
    (*block_node_it)->print_program(std::cout);
    std::cout << "=================" << std::endl;
    #endif

    for (size_t i = 1; i < chain_size; i++){
        descendant_node_branch_constraints = branch_constraints_for_gate(chain[i]);
        Slot_type last_built_gate_1 = build_ast_children(*block_node_it, rule, *entry.get_context(), 0, 1, descendant_node_branch_constraints);

        # if 0
        std::cout << "gate 1 added: ";
        (*block_node_it)->print_program(std::cout);
        std::cout << "=================" << std::endl;
        #endif

        move_qubits(last_built_gate_0, last_built_gate_1);
    }
}

void Remove_gate_chain::apply_blockwise(Node_gen::Iterator block_node_it) const {
    // qubit name -> last qubit op acting on that qubit
    std::unordered_map<std::string, std::shared_ptr<Qubit_op>> last_qubit_op_map;

    auto qubit_ops_gen = Node_gen(**block_node_it, QUBIT_OP);

    auto qubit_ops_it = qubit_ops_gen.begin();

    while (qubit_ops_it != qubit_ops_gen.end()){
        auto qubit_op = static_pointer_cast<Qubit_op>(*qubit_ops_it);

        auto [is_interesting, prev_qubit_op] = qubit_op_is_interesting(qubit_op, func, last_qubit_op_map);

        if (is_interesting){
            auto prev_qubit_op_slot = find_slot_for(*block_node_it, prev_qubit_op);

            // remove current qubit op and previous qubit op
            *qubit_ops_it = std::make_shared<Node>();
            *prev_qubit_op_slot = std::make_shared<Node>();
        }

        qubit_ops_it++;
    }
}

void Combine::apply_blockwise(Node_gen::Iterator block_node_it) const {
    for (const auto& pass : passes){
        if (pass->get_block_kind() == (*block_node_it)->get_node_kind()){
            pass->apply_blockwise(block_node_it);
        }
    }
}

void Dead_subs::apply() {
    reachable_subs.clear();
    reachable_subs.emplace(QuteFuzz::TOP_LEVEL_CIRCUIT_NAME); // top level circuit is always reachable

    bool new_reachable_found = true;

    while(new_reachable_found){
        new_reachable_found = false;

        // check all qubit ops in the AST
        for (auto& qubit_op : entry.get_qubit_ops(true)){
            if (qubit_op->is_subroutine_op()){
                std::string caller = qubit_op->get_caller_name();
                std::string sub_name = qubit_op->get_gate_node()->get_str();

                // if caller is reachable, then op is also reachable
                if (reachable_subs.count(caller)){
                    if (reachable_subs.insert(sub_name).second){
                        new_reachable_found = true;
                    }
                }
            }
        }
    }

    Pass::apply();
}

void Dead_subs::apply_blockwise(Node_gen::Iterator block_node_it) const {
    auto circuit_node = static_pointer_cast<Circuit>(*block_node_it);

    for (const std::string& name : reachable_subs){
        if (circuit_node->get_name() == name){
            // this block is reachable
            return;
        }
    }

    // this block is unreachable
    *block_node_it = std::make_shared<Node>("");
}
