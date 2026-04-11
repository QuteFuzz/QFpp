#ifndef AST_UTILS_H
#define AST_UTILS_H

#include <node.h>

class Gate;

class Grammar;

class Resource;

Slot_type find_slot_for(std::shared_ptr<Node>& search_root, std::shared_ptr<Node>& target);

Token_kind find_gate_in_same_basis(const Token_kind& gate_kind);

std::shared_ptr<Node> build_ast_from_rule(
    std::shared_ptr<Rule> rule,
    const Context& context, 
    std::unordered_map<Token_kind, Branch_constraint> descendant_node_branch_constraints
);

Slot_type build_ast_children(
    Slot_type root, 
    std::shared_ptr<Rule> rule, 
    const Context& context, 
    unsigned int nested_depth, 
    unsigned int n_children,
    std::unordered_map<Token_kind, Branch_constraint> descendant_node_branch_constraints
);

std::shared_ptr<Gate> gate_from_op(Slot_type gate_op_slot);

void move_qubits(const std::shared_ptr<Node> source_qubit_anscestor, Slot_type dest_qubit_anscestor);

void replace_node(Slot_type old_node, std::shared_ptr<Node> new_node);

#endif
