#ifndef AST_UTILS_H
#define AST_UTILS_H

#include <node.h>
#include <node_gen.h>

class Gate;

class Grammar;

class Resource;

class Qubit_op;

extern const std::vector<Token_kind> SELF_INVERSE_PAIRS;

extern const std::vector<std::vector<Token_kind>> INVERSE_PAIRS;

extern const std::vector<Token_kind> Z_FAMILY;

extern const std::vector<Token_kind> X_FAMILY;

extern const std::vector<Token_kind> Y_FAMILY;

extern const std::vector<Token_kind> CLIFFORDS;

Slot_type find_slot_for(const std::shared_ptr<Node>& search_root, const std::shared_ptr<Node>& target);

std::shared_ptr<Node> build_ast_from_rule(
    std::shared_ptr<Rule> rule,
    const Context& context, 
    std::unordered_map<Token_kind, Branch_constraint> descendant_node_branch_constraints
);

Slot_type build_ast_children(
    std::shared_ptr<Node> root, 
    std::shared_ptr<Rule> rule, 
    const Context& context, 
    unsigned int nested_depth, 
    unsigned int n_children,
    std::unordered_map<Token_kind, Branch_constraint> descendant_node_branch_constraints
);

std::shared_ptr<Gate> gate_from_anscestor(std::shared_ptr<Node> anscestor);

void move_qubits(const std::shared_ptr<Node> source_qubit_anscestor, Slot_type dest_qubit_anscestor);

unsigned int max_control_flow_depth_rec(const std::shared_ptr<Node> node, unsigned int current_depth);

std::vector<std::shared_ptr<Resource>> resources_from_anscestor(Node& anscestor, Token_kind resource_node_kind);

std::pair<bool, std::shared_ptr<Qubit_op>> qubit_op_is_interesting(
    std::shared_ptr<Qubit_op> qubit_op,
    std::function<bool(Token_kind, Token_kind)> func, 
    std::unordered_map<std::string, std::shared_ptr<Qubit_op>>& last_qubit_op_map
);

bool gate_in_set(const std::vector<Token_kind>& set, Token_kind gate_kind);

bool is_inverse_pair(Token_kind a, Token_kind b);

bool is_commutative_pair(Token_kind a, Token_kind b);

std::shared_ptr<Node> get_compilation_unit(std::shared_ptr<Node> root);

std::unordered_map<Token_kind, Branch_constraint> branch_constraints_for_gate(const Token_kind& gate_kind);

#endif
