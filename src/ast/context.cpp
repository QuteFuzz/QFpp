#include <context.h>
#include <generator.h>
#include <params.h>

int Context::ast_counter = -1;

void Context::reset(Reset_level l){

    // fall-throughts are intentional

    switch(l){
        case RL_PROGRAM: {
            subroutine_counter = 0;
            Node::node_counter = 0;

            circuits.clear();

            subroutines_node = std::nullopt;
            genome = std::nullopt;
            [[fallthrough]];
        }

        case RL_CIRCUIT:
            nested_depth = control.get_value("NESTED_MAX_DEPTH");
            [[fallthrough]];

        case RL_QUBIT_OP: {
            get_current_circuit()->reset<Qubit>();
            get_current_circuit()->reset<Bit>();

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

    auto pred = [](const auto& elem){ return scope_matches(elem->get_scope(), EXTERNAL_SCOPE); };

    auto qubits = current_circuit->get_collection<Qubit>();
    auto bits = current_circuit->get_collection<Bit>();

    unsigned int num_dest_qubits = qubits.size();
    unsigned int num_dest_bits = bits.size();

    unsigned int num_circuit_qubits = coll_size<Qubit>(qubits, pred);
    unsigned int num_circuit_bits = coll_size<Bit>(bits, pred);

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
    INFO("Circuit " + current_circuit_owner + " can't apply subroutines");
    #endif

    get_current_circuit()->set_can_apply_subroutines(false);
}

template<typename T>
unsigned int Context::get_max_external_resources(){
    size_t res = QuteFuzz::MIN_BITS;
    auto pred = [](const auto& elem){ return scope_matches(elem->get_scope(), EXTERNAL_SCOPE); };

    for(const std::shared_ptr<Circuit>& circuit : circuits){
        res = std::max(res, coll_size<T>(circuit->get_collection<T>(), pred));
    }

    return res;
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


std::shared_ptr<Qubit> Context::get_random_qubit(){
    auto random_qubit = get_random_from_coll(get_current_circuit()->get_collection<Qubit>(), ALL_SCOPES);

    random_qubit->extend_flow_path(current.get<Qubit_op>(), current_port++);

    current.set<Qubit>(random_qubit);
    return random_qubit;
}

std::shared_ptr<Bit> Context::get_random_bit(){
    auto random_bit = get_random_from_coll(get_current_circuit()->get_collection<Bit>(), ALL_SCOPES);
    current.set<Bit>(random_bit);
    return random_bit;
}

std::shared_ptr<Circuit> Context::nn_circuit(){
    std::shared_ptr<Circuit> current_circuit;

    reset(RL_CIRCUIT);

    if(under_subroutines_node()){

        if(genome.has_value()){
            std::shared_ptr<Node> subroutine = genome.value().dag.get_next_subroutine_gate();

            std::cout << YELLOW("setting circuit from DAG ") << std::endl;
            std::cout << YELLOW("owner: " + subroutine->get_content()) << std::endl;
            std::cout << YELLOW("n ports: " + std::to_string(subroutine->get_n_ports())) << std::endl;

            // current_circuit_owner = subroutine->get_content();
            current_circuit = std::make_shared<Circuit>(subroutine->get_content(), subroutine->get_n_ports(), control);

        } else {
            // current_circuit_owner = "sub"+std::to_string(subroutine_counter++);
            current_circuit = std::make_shared<Circuit>("sub" + std::to_string(subroutine_counter++), control, true);
        }

    } else {
        // current_circuit_owner = QuteFuzz::TOP_LEVEL_CIRCUIT_NAME;
        current_circuit = std::make_shared<Circuit>(QuteFuzz::TOP_LEVEL_CIRCUIT_NAME, control, false);
        subroutine_counter = 0;
    }

    circuits.push_back(current_circuit);

    return current_circuit;
}

std::shared_ptr<Qubit_defs> Context::nn_qubit_defs(U8& scope){
    std::shared_ptr<Circuit> current_circuit = get_current_circuit();

    unsigned int num_defs = current_circuit->make_resource_definitions(scope, RK_QUBIT, control);

    return std::make_shared<Qubit_defs>(num_defs);
}

std::shared_ptr<Bit_defs> Context::nn_bit_defs(U8& scope){
    std::shared_ptr<Circuit> current_circuit = get_current_circuit();

    unsigned int num_defs = current_circuit->make_resource_definitions(scope, RK_BIT, control);
    return std::make_shared<Bit_defs>(num_defs);
}

std::shared_ptr<Subroutine_op_arg> Context::nn_subroutine_op_arg(){
    // if((current_gate != nullptr) && *current_gate == SUBROUTINE){
    //     current_subroutine_op_arg = std::make_shared<Subroutine_op_arg>(current_gate->get_next_qubit_def());
    // }

    auto arg = std::make_shared<Subroutine_op_arg>(current.get<Gate>()->get_next_qubit_def());
    current.set<Subroutine_op_arg>(arg);
    return arg;
}

std::shared_ptr<Qubit_definition> Context::nn_qubit_definition(const U8& scope){
    auto qubit_def = get_next_from_coll(get_current_circuit()->get_collection<Qubit_definition>(), scope);
    current.set<Qubit_definition>(qubit_def);
    return qubit_def;
}

std::shared_ptr<Bit_definition> Context::nn_bit_definition(const U8& scope){
    auto bit_def = get_next_from_coll(get_current_circuit()->get_collection<Bit_definition>(), scope);
    current.set<Bit_definition>(bit_def);
    return bit_def;
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
    auto qubit_defs = subroutine_circuit->get_collection<Qubit_definition>();
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
std::shared_ptr<Nested_stmt> Context::nn_nested_stmt(const std::string& str, const Token_kind& kind, std::shared_ptr<Node> parent){
    reset(RL_QUBIT_OP);
    nested_depth = (nested_depth == 0) ? 0 : nested_depth - 1;
    return std::make_shared<Nested_stmt>(str, kind);
}

std::shared_ptr<Compound_stmt> Context::nn_compound_stmt(std::shared_ptr<Node> parent){
    return Compound_stmt::from_nested_depth(nested_depth);
}

std::shared_ptr<Compound_stmts> Context::nn_compound_stmts(std::shared_ptr<Node> parent){

    /*
        this check is to make sure we only call the function for the compound statements rule in the circuit body, as opposed to each nested
        call within the body in control flow
    */

    if(*parent == BODY){
        set_can_apply_subroutines();
    }

    return Compound_stmts::from_num_compound_stmts(QuteFuzz::WILDCARD_MAX);
}

std::shared_ptr<Subroutine_defs> Context::nn_subroutines(){
    unsigned int n_circuits = random_uint(control.get_value("MAX_NUM_SUBROUTINES"));

    if(genome.has_value()){
        n_circuits = genome.value().dag.n_subroutines();
    }

    std::shared_ptr<Subroutine_defs> node = std::make_shared<Subroutine_defs>(n_circuits);

    subroutines_node = std::make_optional<std::shared_ptr<Subroutine_defs>>(node);

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
