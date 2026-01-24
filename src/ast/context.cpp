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
            can_copy_dag = false;

            circuits.clear();

            subroutines_node = std::nullopt;
            genome = std::nullopt;
            [[fallthrough]];
        }

        case RL_CIRCUIT: 
            nested_depth = control.get_value("NESTED_MAX_DEPTH");
            [[fallthrough]];

        case RL_QUBIT_OP: {
            get_current_circuit()->qubit_flag_reset();
            get_current_circuit()->bit_flag_reset();

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

    if(circuit->owned_by(QuteFuzz::TOP_LEVEL_CIRCUIT_NAME) || circuit->owned_by(current_circuit_owner)){
        return false;
    }

    unsigned int num_dest_qubits = current_circuit->num_qubits_of(ALL_SCOPES);
    unsigned int num_dest_bits = current_circuit->num_bits_of(ALL_SCOPES);

    unsigned int num_circuit_qubits = circuit->num_qubits_of(EXTERNAL_SCOPE);
    unsigned int num_circuit_bits = circuit->num_bits_of(EXTERNAL_SCOPE);

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

unsigned int Context::get_max_external_qubits(){
    size_t res = QuteFuzz::MIN_QUBITS;

    for(const std::shared_ptr<Circuit>& circuit : circuits){
        res = std::max(res, circuit->num_qubits_of(EXTERNAL_SCOPE));
    }

    return res;
}

unsigned int Context::get_max_external_bits(){
    size_t res = QuteFuzz::MIN_BITS;

    for(const std::shared_ptr<Circuit>& circuit : circuits){
        res = std::max(res, circuit->num_bits_of(EXTERNAL_SCOPE));
    }

    return res;
}

/// In normal cases, current circuit is the last added circuit into the circuits vector. The exception is if we are no longer under the `subroutines`
/// node in the AST, but the last added circuit is a subroutine. This implies that after the `subroutines` node, there's no `circuit` node to generate
/// a new, main circuit. As such, qubit and bit definitions, qubits and bits may have been made globally, and therefore stored in the dummy circuit, so we return that
std::shared_ptr<Circuit> Context::get_current_circuit() const {
    if((circuits.size() == 0) || (!under_subroutines_node() && circuits.back()->check_if_subroutine())){
        return dummy_circuit;
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
        return dummy_circuit;
    }
}

std::shared_ptr<Circuit> Context::new_circuit_node(){
    std::shared_ptr<Circuit> current_circuit;

    reset(RL_CIRCUIT);

    if(under_subroutines_node()){

        if(genome.has_value()){
            std::shared_ptr<Node> subroutine = genome.value().dag.get_next_subroutine_gate();

            std::cout << YELLOW("setting circuit from DAG ") << std::endl;
            std::cout << YELLOW("owner: " + subroutine->get_content()) << std::endl;
            std::cout << YELLOW("n ports: " + std::to_string(subroutine->get_n_ports())) << std::endl;

            current_circuit_owner = subroutine->get_content();
            current_circuit = std::make_shared<Circuit>(current_circuit_owner, subroutine->get_n_ports(), control);

        } else {
            current_circuit_owner = "sub"+std::to_string(subroutine_counter++);
            current_circuit = std::make_shared<Circuit>(current_circuit_owner, control, true);
        }

    } else {
        current_circuit_owner = QuteFuzz::TOP_LEVEL_CIRCUIT_NAME;
        current_circuit = std::make_shared<Circuit>(QuteFuzz::TOP_LEVEL_CIRCUIT_NAME, control, false);

        subroutine_counter = 0;

        can_copy_dag = genome.has_value();
    }

    circuits.push_back(current_circuit);

    return current_circuit;
}

std::shared_ptr<Qubit_defs> Context::get_qubit_defs_node(U8& scope){
    std::shared_ptr<Circuit> current_circuit = get_current_circuit();

    unsigned int num_defs;

    if(can_copy_dag){
        num_defs = current_circuit->make_resource_definitions(genome->dag, scope, RK_QUBIT);

    } else {
        num_defs = current_circuit->make_resource_definitions(scope, RK_QUBIT, control);
    }

    return std::make_shared<Qubit_defs>(num_defs);
}

std::shared_ptr<Qubit_defs> Context::get_qubit_defs_discard_node(U8& scope){
    std::shared_ptr<Circuit> current_circuit = get_current_circuit();

    unsigned int num_defs;

    if(can_copy_dag){
        num_defs = current_circuit->make_resource_definitions(genome->dag, scope, RK_QUBIT, true);

    } else {
        num_defs = current_circuit->make_resource_definitions(scope, RK_QUBIT, control, true);
    }

    return std::make_shared<Qubit_defs>(num_defs, true);
}

std::shared_ptr<Bit_defs> Context::get_bit_defs_node(U8& scope){
    std::shared_ptr<Circuit> current_circuit = get_current_circuit();

    unsigned int num_defs;

    if(can_copy_dag){
        num_defs = current_circuit->make_resource_definitions(genome->dag, scope, RK_BIT);
    } else {
        num_defs = current_circuit->make_resource_definitions(scope, RK_BIT, control);
    }

    return std::make_shared<Bit_defs>(num_defs);
}

std::optional<std::shared_ptr<Circuit>> Context::get_circuit(std::string owner){
    for(const std::shared_ptr<Circuit>& circuit : circuits){
        if(circuit->owned_by(owner)) return std::make_optional<std::shared_ptr<Circuit>>(circuit);
    }

    INFO("Circuit owner by " + owner + " not defined!");

    return std::nullopt;
}

std::shared_ptr<Qubit> Context::new_qubit(){
    // U8 scope = (*current_gate == QuteFuzz::Measure) ? OWNED_SCOPE : ALL_SCOPES;

    auto random_qubit = get_current_circuit()->get_random_qubit(ALL_SCOPES);

    random_qubit->extend_flow_path(current_qubit_op, current_port++);

    current_qubit = random_qubit;

    return current_qubit;
}

std::shared_ptr<Integer> Context::get_current_qubit_index(){
    if(current_qubit != nullptr){
        return current_qubit->get_index();
    } else {
        WARNING("Current qubit not set but trying to get index! Using dummy instead");
        return std::make_shared<Integer>(dummy_int);
    }
}

std::shared_ptr<Bit> Context::new_bit(){
    auto random_bit = get_current_circuit()->get_random_bit(ALL_SCOPES);
    current_bit = random_bit;

    return current_bit;
}

std::shared_ptr<Integer> Context::get_current_bit_index(){
    if(current_bit != nullptr){
        return current_bit->get_index();
    } else {
        WARNING("Current bit not set but trying to get index! Using dummy instead");
        return std::make_shared<Integer>(dummy_int);
    }
}

std::shared_ptr<Variable> Context::get_current_qubit_name(){
    if(current_qubit != nullptr){
        return current_qubit->get_name();
    } else {
        WARNING("Current qubit not set but trying to get name! Using dummy instead");
        return std::make_shared<Variable>(dummy_var);
    }
}

std::shared_ptr<Variable> Context::get_current_bit_name(){
    if(current_bit != nullptr){
        return current_bit->get_name();
    } else {
        WARNING("Current bit not set but trying to get name! Using dummy instead");
        return std::make_shared<Variable>(dummy_var);
    }
}

std::shared_ptr<Integer> Context::get_current_qubit_definition_size(){
    if(current_qubit_definition != nullptr){
        return current_qubit_definition->get_size();
    } else  {
        WARNING("Current qubit not set but trying to get size! Using dummy instead");
        return std::make_shared<Integer>(dummy_int);
    }
}

std::shared_ptr<Variable> Context::get_current_qubit_definition_name(){
    if(current_qubit_definition != nullptr){
        return current_qubit_definition->get_name();
    } else {
        WARNING("Current qubit not set but trying to get name! Using dummy instead");
        return std::make_shared<Variable>(dummy_var);
    }
}

std::shared_ptr<Integer> Context::get_current_bit_definition_size(){
    if(current_bit_definition != nullptr && current_bit_definition->is_register_def()){
        return current_bit_definition->get_size();
    } else {
        WARNING("Current bit definition not set or is singular but trying to get size! Using dummy instead");
        return std::make_shared<Integer>(dummy_int);
    }
}

std::shared_ptr<Variable> Context::get_current_bit_definition_name(){
    if(current_bit_definition != nullptr){
        return current_bit_definition->get_name();
    } else {
        WARNING("Current bit definition not set but trying to get name! Using dummy instead");
        return std::make_shared<Variable>(dummy_var);
    }
}

/// @brief Any stmt that is nested (if, elif, else) is a nested stmt. Any time such a node is used, reduce nested depth
/// @param str 
/// @param kind 
/// @param parent 
/// @return 
std::shared_ptr<Nested_stmt> Context::get_nested_stmt(const std::string& str, const Token_kind& kind, std::shared_ptr<Node> parent){
    nested_depth = (nested_depth == 0) ? 0 : nested_depth - 1;

    if(can_copy_dag){
        return std::make_shared<Nested_stmt>(str, kind, parent->get_next_child_target());

    } else {
        return std::make_shared<Nested_stmt>(str, kind);
    }
}

std::shared_ptr<Compound_stmt> Context::get_compound_stmt(std::shared_ptr<Node> parent){

    if(can_copy_dag){
        return Compound_stmt::from_num_qubit_ops(parent->get_next_child_target());
    } else {
        return Compound_stmt::from_nested_depth(nested_depth);
    }

}

std::shared_ptr<Compound_stmts> Context::get_compound_stmts(std::shared_ptr<Node> parent){

    /*
        this check is to make sure we only call the function for the compound statements rule in the circuit body, as opposed to each nested
        call within the body in control flow
    */

    if(*parent == BODY){
        set_can_apply_subroutines();

        if(can_copy_dag){
            parent->make_partition(genome.value().dag.n_qubit_ops(), 1);
        }
    }

    if(can_copy_dag){
        return Compound_stmts::from_num_qubit_ops(parent->get_next_child_target());

    } else {
        return Compound_stmts::from_num_compound_stmts(QuteFuzz::WILDCARD_MAX);
    }
}

std::shared_ptr<Subroutine_defs> Context::new_subroutines_node(){
    unsigned int n_circuits = random_uint(control.get_value("MAX_NUM_SUBROUTINES"));

    if(genome.has_value()){
        n_circuits = genome.value().dag.n_subroutines();
    }

    std::shared_ptr<Subroutine_defs> node = std::make_shared<Subroutine_defs>(n_circuits);

    subroutines_node = std::make_optional<std::shared_ptr<Subroutine_defs>>(node);

    return node;
}

std::shared_ptr<Qubit_op> Context::new_qubit_op_node(){
    reset(RL_QUBIT_OP);
    current_qubit_op = can_copy_dag ? genome.value().dag.get_next_node() : std::make_shared<Qubit_op>(get_current_circuit());

    return current_qubit_op;
}
