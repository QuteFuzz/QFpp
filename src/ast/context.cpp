#include <context.h>
#include <generator.h>
#include <params.h>
#include <variable.h>
#include <indent.h>

int Context::ast_counter = -1;

void Context::reset(Reset_level l){

    // fall-throughts are intentional

    switch(l){
        case RL_PROGRAM: {
            subroutine_counter = 0;
            Node::node_counter = 0;

            circuits.clear();

            subroutines_node = std::nullopt;
            [[fallthrough]];
        }

        case RL_CIRCUIT:
            nested_depth = control.get_value("NESTED_MAX_DEPTH");
            [[fallthrough]];

        case RL_QUBITS:
            get_current_circuit()->reset(Resource_kind::QUBIT);
            current_port = 0;
            break;

        case RL_BITS:
            get_current_circuit()->reset(Resource_kind::BIT);
    }
}

/// @brief Check whether current circuit can apply `circuit` as a subroutine
/// @param dest
/// @param circuit
/// @return
bool Context::can_apply_as_subroutine(const std::shared_ptr<Circuit> circuit){

    std::shared_ptr<Circuit> current_circuit = get_current_circuit();

    if(circuit->owned_by(QuteFuzz::TOP_LEVEL_CIRCUIT_NAME) || circuit->owned_by(get_current_circuit()->get_owner())){
        return false;
    }

    auto ext_scope_pred = [](const auto& elem){ return scope_matches(elem->get_scope(), Scope::EXT); };

    auto current_circuit_qubits = current_circuit->get_coll<Resource>(Resource_kind::QUBIT);
    auto dest_circuit_qubits = circuit->get_coll<Resource>(Resource_kind::QUBIT);

    auto current_circuit_bits = current_circuit->get_coll<Resource>(Resource_kind::BIT);
    auto dest_circuit_bits = circuit->get_coll<Resource>(Resource_kind::BIT);

    unsigned int num_required_qubits = size_pred<Resource>(dest_circuit_qubits, ext_scope_pred);
    unsigned int num_required_bits = size_pred<Resource>(dest_circuit_bits, ext_scope_pred);

    unsigned int num_qubits_in_circuit = current_circuit_qubits.size();
    unsigned int num_bits_in_circuit = current_circuit_bits.size();

    bool has_enough_qubits = num_qubits_in_circuit >= 1 && num_qubits_in_circuit >= num_required_qubits;
    bool has_enough_bits = num_bits_in_circuit >= 1 && num_bits_in_circuit >= num_required_bits;

    return has_enough_qubits && has_enough_bits;
}

bool Context::current_circuit_uses_subroutines(){
    for(std::shared_ptr<Circuit> circuit : circuits){
        if (can_apply_as_subroutine(circuit))
        {
            return true;
        }
    }

    return false;
}

/// In normal cases, current circuit is the last added circuit into the circuits vector. The exception is if we are no longer under the `subroutines`
/// node in the AST, but the last added circuit is a subroutine. This implies that after the `subroutines` node, there's no `circuit` node to generate
/// a new, main circuit. As such, qubit and bit definitions, qubits and bits may have been made globally, and therefore stored in the dummy circuit, so we return that
std::shared_ptr<Circuit> Context::get_current_circuit() const {
    if((circuits.size() == 0) || (!under_subroutines_node() && circuits.back()->check_if_subroutine())){
        return dummies.circuit;
    } else {
        return circuits.back();
    }
}

std::shared_ptr<Circuit> Context::get_random_circuit(){

    bool valid_circuit_exists = false;
    for (const auto& circuit : circuits) {
        if (can_apply_as_subroutine(circuit)) {
            valid_circuit_exists = true;
            break;
        }
    }

    if(circuits.size() && valid_circuit_exists){

        std::shared_ptr<Circuit> circuit = circuits.at(random_uint(circuits.size()-1));

        while(!can_apply_as_subroutine(circuit)){
            circuit = circuits.at(random_uint(circuits.size()-1));
        }

        return circuit;

    } else {
        ERROR("No available circuits to use as subroutines!");
        return dummies.circuit;
    }
}

std::shared_ptr<Resource> Context::get_random_resource(Resource_kind rk){
    Ptr_pred_type<Resource> pred = [](const std::shared_ptr<Resource>& elem){ return !elem->is_used(); };
    auto filtered_coll = get_current_circuit()->get_coll<Resource>(rk);
    auto random_resource = get_random_from_coll<Resource>(filtered_coll, pred);

    random_resource->set_used();
    // random_resource->extend_flow_path(current.get<Qubit_op>(), current_port++);

    current.set<Resource>(random_resource);
    return random_resource;
}


std::shared_ptr<Resource_def> Context::nn_resource_def(Scope& scope, Resource_kind rk){
    std::shared_ptr<Resource_def> def;

    // decide whether to treat def as a register or singular definition
    bool can_use_reg, can_use_sing, is_reg;

    if(rk == Resource_kind::QUBIT){
        can_use_reg = !control.get_rule("register_qubit_def", scope)->is_empty();
        can_use_sing = !control.get_rule("singular_qubit_def", scope)->is_empty();
    } else {
        can_use_reg = !control.get_rule("register_bit_def", scope)->is_empty();
        can_use_sing = !control.get_rule("singular_bit_def", scope)->is_empty();
    }

    if (can_use_reg && can_use_sing){
        is_reg = random_uint(1, 0);
    } else if (can_use_reg){
        is_reg = true;
    } else {
        is_reg = false;
    }

    def = std::make_shared<Resource_def>(scope, rk, is_reg, random_uint(control.get_value("MAX_REG_SIZE"), 1));

    current.set<Resource_def>(def);
    get_current_circuit()->store_resource_def(def);

    return def;
}

std::shared_ptr<Circuit> Context::nn_circuit(){
    std::shared_ptr<Circuit> current_circuit;

    reset(RL_CIRCUIT);

    if(under_subroutines_node()){
        current_circuit = std::make_shared<Circuit>("sub" + std::to_string(subroutine_counter++), true);

    } else {
        current_circuit = std::make_shared<Circuit>(QuteFuzz::TOP_LEVEL_CIRCUIT_NAME, false);
        subroutine_counter = 0;
    }

    circuits.push_back(current_circuit);

    return current_circuit;
}

std::shared_ptr<Gate> Context::nn_gate(const std::string& str, Token_kind& kind){
    auto gate = std::make_shared<Gate>(str, kind);

    current.set<Gate>(gate);
    current.get<Qubit_op>()->set_gate_node(gate);

    return gate;
}

std::shared_ptr<Gate> Context::nn_gate_from_subroutine(){
    /*
        1. get random circuit to use as subroutine
        2. get qubit defs inside subroutine, create the gate node
        3. give the gate its name
        4. set current gate to be this new node
        5. give this gate to the current qubit op
    */
    std::shared_ptr<Circuit> subroutine_circuit = get_random_circuit();
    auto gate_name = subroutine_circuit->get_owner();

    auto gate = std::make_shared<Gate>(gate_name, SUBROUTINE, subroutine_circuit->get_coll<Resource_def>());
    gate->add_child(std::make_shared<Variable>(gate_name));

    current.set<Gate>(gate);
    current.get<Qubit_op>()->set_gate_node(gate);

    return gate;
}

std::shared_ptr<UInt> Context::nn_circuit_id() {
    return std::make_shared<UInt>(ast_counter);
}

std::shared_ptr<Compound_stmt> Context::nn_compound_stmt(){
    return Compound_stmt::from_nested_depth(nested_depth);
}

std::shared_ptr<Node> Context::nn_subroutines(){
    std::shared_ptr<Node> node = std::make_shared<Node>("", SUBROUTINE_DEFS);
    subroutines_node = std::make_optional<std::shared_ptr<Node>>(node);
    return node;
}

std::shared_ptr<Qubit_op> Context::nn_qubit_op(){
    reset(RL_QUBITS);
    reset(RL_BITS);

    auto qubit_op = std::make_shared<Qubit_op>();
    current.set<Qubit_op>(qubit_op);
    return qubit_op;
}

std::shared_ptr<Parameter_def> Context::nn_parameter_def(){
    auto def = std::make_shared<Parameter_def>();
    current.set<Parameter_def>(def);
    return def;
}

std::shared_ptr<Node> Context::nn_next(Node& ast_root, const Token_kind& kind){

    if (node_generators.find(kind) == node_generators.end()){
        node_generators[kind] = std::make_unique<Node_gen>(ast_root, kind);
    }

    return *node_generators[kind]->begin()++;
}

unsigned int Context::operator()(Token_kind kind) const {
    auto gate = get_current_node<Gate>();

    if (kind == NUM_QUBITS) {
        return gate->get_num_external_qubits();
    } else if (kind == NUM_BITS) {
        return gate->get_num_external_bits();
    } else if (kind == NUM_FLOATS) {
        return gate->get_num_floats();
    }

    return 0;
}
