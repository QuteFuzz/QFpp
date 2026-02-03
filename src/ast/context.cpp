#include <context.h>
#include <generator.h>
#include <params.h>
#include <name.h>

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

        case RL_QUBIT_OP: {
            get_current_circuit()->reset();

            current_port = 0;
        }
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

    unsigned int num_dest_qubits = filter<Resource>(dest_circuit_qubits, ext_scope_pred).size();
    unsigned int num_dest_bits = filter<Resource>(dest_circuit_bits, ext_scope_pred).size();

    unsigned int num_circuit_qubits = current_circuit_qubits.size();
    unsigned int num_circuit_bits = current_circuit_bits.size();

    bool has_enough_qubits = (num_circuit_qubits >= 1 && num_circuit_qubits <= num_dest_qubits);
    bool has_enough_bits = num_circuit_bits <= num_dest_bits;

    return has_enough_qubits && has_enough_bits;
}

void Context::set_can_apply_subroutines(){

    for(std::shared_ptr<Circuit> circuit : circuits){
        if (can_apply_as_subroutine(circuit))
        {
            return;
        }
    }

    #ifdef DEBUG
    INFO("Circuit " + get_current_circuit()->get_owner() + " can't apply subroutines");
    #endif

    get_current_circuit()->set_can_apply_subroutines(false);
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

/// @brief This loop is guaranteed to stop. If this function is called, then `set_can_apply_subroutines` must've passed
/// So you just need to return a current circuit that's not the main circuit, or the current circuit and needs <= num qubits in current circuit
/// Qubit comparison needed because `set_can_apply_subroutines` only tells you that there's at least one circuit that can be picked
/// @return
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
    auto random_resource = get_random_from_coll<Resource>(get_current_circuit()->get_coll<Resource>(rk));

    random_resource->extend_flow_path(current.get<Qubit_op>(), current_port++);

    current.set<Resource>(random_resource);
    return random_resource;
}

std::shared_ptr<Resource_def> Context::nn_register_resource_def(Scope& scope, Resource_kind rk){
    std::shared_ptr<Resource_def> def;

    auto reg_def = Register_resource_def(Name(), Integer(random_uint(QuteFuzz::MAX_REG_SIZE)));
    def = std::make_shared<Resource_def>(reg_def, scope, rk);

    current.set<Resource_def>(def);
    get_current_circuit()->make_resources_from_def(def);

    return def;
}

std::shared_ptr<Resource_def> Context::nn_singular_resource_def(Scope& scope, Resource_kind rk){
    std::shared_ptr<Resource_def> def;

    auto sing_def = Singular_resource_def(Name());
    def = std::make_shared<Resource_def>(sing_def, scope, rk);

    current.set<Resource_def>(def);
    get_current_circuit()->make_resources_from_def(def);

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

std::shared_ptr<Subroutine_op_arg> Context::nn_subroutine_op_arg(){
    auto arg = std::make_shared<Subroutine_op_arg>(current.get<Gate>()->get_next_qubit_def());
    current.set<Subroutine_op_arg>(arg);
    return arg;
}

std::shared_ptr<Gate> Context::nn_gate(const std::string& str, Token_kind& kind, int num_qubits, int num_bits, int num_params){
    auto gate = std::make_shared<Gate>(str, kind, num_qubits, num_bits, num_params);

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
    auto qubit_defs = subroutine_circuit->get_coll<Resource_def>(Resource_kind::QUBIT);
    auto gate_name = subroutine_circuit->get_owner();

    auto gate = std::make_shared<Gate>(gate_name, SUBROUTINE, qubit_defs);
    gate->add_child(std::make_shared<Variable>(gate_name));

    current.set<Gate>(gate);
    current.get<Qubit_op>()->set_gate_node(gate);

    return gate;
}

std::shared_ptr<Integer> Context::nn_circuit_id() {
    return std::make_shared<Integer>(ast_counter);
}

/// @brief Any stmt that is nested (if, elif, else) is a nested stmt. Any time such a node is used, reduce nested depth
/// @param str
/// @param kind
/// @param parent
/// @return
std::shared_ptr<Nested_stmt> Context::nn_nested_stmt(const std::string& str, const Token_kind& kind){
    reset(RL_QUBIT_OP);
    nested_depth = (nested_depth == 0) ? 0 : nested_depth - 1;
    return std::make_shared<Nested_stmt>(str, kind);
}

std::shared_ptr<Compound_stmt> Context::nn_compound_stmt(){
    return Compound_stmt::from_nested_depth(nested_depth);
}

std::shared_ptr<Node> Context::nn_subroutines(){
    // unsigned int n_circuits = random_uint(control.get_value("MAX_NUM_SUBROUTINES"));

    std::shared_ptr<Node> node = std::make_shared<Node>("", SUBROUTINE_DEFS);
    subroutines_node = std::make_optional<std::shared_ptr<Node>>(node);
    return node;
}

std::shared_ptr<Qubit_op> Context::nn_qubit_op(){
    reset(RL_QUBIT_OP);

    auto qubit_op = std::make_shared<Qubit_op>(get_current_circuit());
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
