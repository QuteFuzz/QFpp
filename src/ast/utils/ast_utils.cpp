#include <ast_utils.h>
#include <resource.h>
#include <grammar.h>
#include <ast.h>
#include <gate.h>

enum class Commutation_basis {
    NONE,
    X_BASIS,
    Y_BASIS,
    Z_BASIS,
};


static const std::unordered_map<Commutation_basis, std::vector<Token_kind>> COMMUTATIVE_GATES = {
    {Commutation_basis::X_BASIS, {X, RX}},
    {Commutation_basis::Y_BASIS, {Y, RY}},
    {Commutation_basis::Z_BASIS, {Z, RZ}},  // S and T removed due to predicate issue in pytket, see grammar
};

Slot_type find_slot_for(std::shared_ptr<Node>& search_root, std::shared_ptr<Node>& target) {
    for (auto& child : search_root->get_children()) {
        if (child.get() == target.get()) return &child;
        auto found = find_slot_for(child, target);
        if (found) return found;
    }
    return nullptr;
}

Token_kind find_gate_in_same_basis(const Token_kind& gate_kind) {
    for (const auto&[basis, gates] : COMMUTATIVE_GATES){
        if (std::find(gates.begin(), gates.end(), gate_kind) != gates.end()) {
            unsigned int n_gates = gates.size();
            Token_kind other_gate_kind = gates.at(random_uint(n_gates - 1));

            while(other_gate_kind == gate_kind) {
                other_gate_kind = gates.at(random_uint(n_gates - 1));
            }

            return other_gate_kind;
        }
    }
    
    return gate_kind;
}

std::shared_ptr<Node> build_ast_from_rule(
    std::shared_ptr<Rule> rule,
    const Context& context, 
    std::unordered_map<Token_kind, Branch_constraint> descendant_node_branch_constraints
) {
    std::shared_ptr<Ast> ast_builder = std::make_shared<Ast>(context, 0);
    return ast_builder->build(rule, descendant_node_branch_constraints);
}

Slot_type build_ast_children(
    Slot_type root, 
    std::shared_ptr<Rule> rule, 
    const Context& context, 
    unsigned int nested_depth,
    unsigned int n_children,
    std::unordered_map<Token_kind, Branch_constraint> descendant_node_branch_constraints
){
    std::shared_ptr<Ast> ast_builder = std::make_shared<Ast>(context, nested_depth);
    const Term& term = make_term_from_rule(rule);
    return ast_builder->term_branch_to_child_nodes(*root, term, n_children, descendant_node_branch_constraints);
}

std::shared_ptr<Gate> gate_from_anscestor(std::shared_ptr<Node> anscestor) {
    std::shared_ptr<Gate> gate;

    std::shared_ptr<Node> gate_name_primitive = anscestor->find(GATE_NAME);
    std::shared_ptr<Node> gate_subroutine = anscestor->find(SUBROUTINE);

    std::shared_ptr<Gate> primitive_gate = 
        (gate_name_primitive == nullptr) ? 
            nullptr : 
            std::dynamic_pointer_cast<Gate>(gate_name_primitive->child_at(0));

    gate = (primitive_gate == nullptr) ? std::dynamic_pointer_cast<Gate>(gate_subroutine) : primitive_gate;

    if (gate == nullptr){
        std::cout << "========================" << std::endl;
        anscestor->print_program(std::cout);
        std::cout << "========================" << std::endl;

        ERROR("`Gate` node not found anywhere under this anscestor");
    }

    return gate;
}

void replace_node(Slot_type old_node, std::shared_ptr<Node> new_node) {
    new_node->print_mode = (*old_node)->print_mode;
    *old_node = new_node;
}

/// Move qubits from source to dest anscenstor. Assumed both have the same number of qubits
void move_qubits(const std::shared_ptr<Node> source_qubit_anscestor, Slot_type dest_qubit_anscestor) {
    auto source_gate = gate_from_anscestor(source_qubit_anscestor);
    auto dest_gate = gate_from_anscestor(*dest_qubit_anscestor);

    // only move if both have the same number of qubits
    if (source_gate->get_num_external_qubits() == dest_gate->get_num_external_qubits()){
        auto source_qubits = Node_gen(*source_qubit_anscestor, QUBIT);
        auto dest_qubits = Node_gen(**dest_qubit_anscestor, QUBIT);

        auto source_it = source_qubits.begin();
        auto dest_it = dest_qubits.begin();

        while((source_it != source_qubits.end()) && (dest_it != dest_qubits.end())){
            *dest_it = (*source_it)->clone(DEEP);

            dest_it++;
            source_it++;
        }
    }
}

unsigned int max_control_flow_depth_rec(const std::shared_ptr<Node> node, unsigned int current_depth) {
    Token_kind kind = node->get_node_kind();

    unsigned int depth = current_depth + (kind == CF_STMT);
    unsigned int max_depth = depth;

    for(const std::shared_ptr<Node>& child : node->get_children()){
        unsigned int child_depth = max_control_flow_depth_rec(child, depth);
        max_depth = std::max(max_depth, child_depth);
    }

    return max_depth;
}

std::vector<std::shared_ptr<Resource>> resources_from_anscestor(Node& anscestor, Token_kind resource_node_kind){
    std::vector<std::shared_ptr<Resource>> out;

    for (const auto& qubit : Node_gen(anscestor, resource_node_kind)) {
        auto resource = std::dynamic_pointer_cast<Resource>(qubit);
        
        if (resource == nullptr) continue;
    
        out.push_back(resource);
    }

    return out;
}
