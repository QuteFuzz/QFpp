#include <ast_utils.h>
#include <resource.h>
#include <grammar.h>
#include <ast.h>
#include <gate.h>
#include <qubit_op.h>

const std::vector<Token_kind> SELF_INVERSE_PAIRS = {
    H, X, Y, Z, CX, CY, CZ, SWAP, CCX, CSWAP, TOFFOLI
};

const std::vector<std::vector<Token_kind>> INVERSE_PAIRS = {
    {S, SDG},
    {T, TDG},
};

const std::vector<Token_kind> Z_FAMILY = {Z, S, SDG, T, TDG, RZ, U1};

const std::vector<Token_kind> X_FAMILY = {X, RX};  // SX, SXDG

const std::vector<Token_kind> Y_FAMILY = {Y, RY};


Slot_type find_slot_for(const std::shared_ptr<Node>& search_root, const std::shared_ptr<Node>& target) {
    for (auto& child : search_root->get_children()) {
        if (child.get() == target.get()) return &child;
        auto found = find_slot_for(child, target);
        if (found) return found;
    }
    return nullptr;
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
    std::shared_ptr<Node> root, 
    std::shared_ptr<Rule> rule, 
    const Context& context, 
    unsigned int nested_depth,
    unsigned int n_children,
    std::unordered_map<Token_kind, Branch_constraint> descendant_node_branch_constraints
){
    std::shared_ptr<Ast> ast_builder = std::make_shared<Ast>(context, nested_depth);
    const Term& term = make_term_from_rule(rule);
    auto child_expr = std::make_shared<IntExpr>(n_children);
    return ast_builder->term_branch_to_child_nodes(root, term, descendant_node_branch_constraints, child_expr);
}

std::shared_ptr<Gate> gate_from_anscestor(std::shared_ptr<Node> anscestor) {
    std::shared_ptr<Gate> gate;

    if (anscestor->get_node_kind() == QUBIT_OP){
        // qubit ops store their gates at AST build time
        return static_pointer_cast<Qubit_op>(anscestor)->get_gate_node();
    }

    std::shared_ptr<Node> gate_name_primitive = anscestor->find(PRIMITIVE_GATE);
    std::shared_ptr<Node> gate_subroutine = anscestor->find(SUBROUTINE_OP);

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

/// Move qubits from source to dest anscenstor. Assumed both have the same number of qubits
void move_qubits(const std::shared_ptr<Node> source_qubit_anscestor, Slot_type dest_qubit_anscestor) {
    auto source_gate = gate_from_anscestor(source_qubit_anscestor);
    auto dest_gate = gate_from_anscestor(*dest_qubit_anscestor);

    // only move if both have the same number of qubits
    if (source_gate->get_num_external_resources(Resource_kind::QUBIT) == dest_gate->get_num_external_resources(Resource_kind::QUBIT)){
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

std::pair<bool, std::shared_ptr<Qubit_op>> qubit_op_is_interesting(
    std::shared_ptr<Qubit_op> qubit_op, 
    std::function<bool(Token_kind, Token_kind)> func, 
    std::unordered_map<std::string, std::shared_ptr<Qubit_op>>& last_qubit_op_map
){
    std::shared_ptr<Gate> gate = qubit_op->get_gate_node();
    Token_kind gate_kind = gate->get_node_kind();
    auto qubit_names = qubit_op->get_target_qubit_names();

    bool qubits_satisfied = true;
    std::shared_ptr<Qubit_op> prev_qubit_op = nullptr;

    for (const std::string& name : qubit_names){
        auto it = last_qubit_op_map.find(name);

        if (it == last_qubit_op_map.end()){
            qubits_satisfied = false; break;
        }

        if (prev_qubit_op == nullptr){
            prev_qubit_op = it->second;
        } else if (it->second != prev_qubit_op){
            // for multi qubit gates, need to make sure that last qubit op of all qubits is the same in memory
            qubits_satisfied = false; break;
        }
    }

    bool is_intersting = qubits_satisfied && (prev_qubit_op != nullptr) && func(gate_kind, prev_qubit_op->get_gate_node()->get_node_kind());

    if (is_intersting){
        // remove all qubits from last_qubit_op tracker
        for (const auto& name : qubit_names) last_qubit_op_map.erase(name);
    
    } else {
        // set last qubit op for all qubits
        for (const auto& name : qubit_names) last_qubit_op_map[name] = qubit_op;
    }

    return std::make_pair(is_intersting, prev_qubit_op);
}

static bool gate_in_set(const std::vector<Token_kind>& set, Token_kind gate_kind){
    return std::find(set.begin(), set.end(), gate_kind) != set.end();
}

bool is_inverse_pair(Token_kind a, Token_kind b) {
    if (a == b) {
        return gate_in_set(SELF_INVERSE_PAIRS, a);
    } else {
        for (const auto& pair : INVERSE_PAIRS){
            if (gate_in_set(pair, a) && gate_in_set(pair, b)){
                return true;
            }
        }

        return false;
    }
}

bool is_commutative_pair(Token_kind a, Token_kind b){
    if (a == b) return true;

    if (gate_in_set(X_FAMILY, a) && gate_in_set(X_FAMILY, b)) return true;

    if (gate_in_set(Y_FAMILY, a) && gate_in_set(Y_FAMILY, b)) return true;
    
    if (gate_in_set(Z_FAMILY, a) && gate_in_set(Z_FAMILY, b)) return true;

    return false;
}

std::shared_ptr<Node> get_compilation_unit(std::shared_ptr<Node> root){
    std::shared_ptr<Node> comp_unit = nullptr;
    
    for(const auto& slot : Node_gen(*root, CIRCUIT)){
        comp_unit = slot;
    }

    if (comp_unit == nullptr){
        for(const auto& slot : Node_gen(*root, BODY)){
            comp_unit = slot;
        }
    }

    if (comp_unit == nullptr){
        ERROR("Could not find compilation unit for program. Make `circuit` or `body` the \"main_circuit\" of the program");
    } else {
        return comp_unit;
    }
}