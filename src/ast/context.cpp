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

            subroutine_defs_node = nullptr;
            [[fallthrough]];
        }

        case RL_CIRCUIT:
            nested_depth = control.get_value("NESTED_MAX_DEPTH");
            [[fallthrough]];

        case RL_QUBITS:
            get_current_circuit()->reset(Resource_kind::QUBIT);
            current_port = 0;
            break;

        case RL_PARAMS:
            get_current_circuit()->reset(Resource_kind::PARAM);
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

    std::string circuit_name = circuit->get_name();

    if((circuit_name == QuteFuzz::TOP_LEVEL_CIRCUIT_NAME) || (circuit_name == current_circuit->get_name())){
        return false;
    }

    static std::vector<Resource_kind> hungry_resources = {
        Resource_kind::QUBIT, Resource_kind::BIT
    };

    for (auto rk : hungry_resources){
        auto current_circuit_resources = current_circuit->get_coll<Resource>(rk);
        unsigned int current_circuit_n_resources = current_circuit_resources.size();

        unsigned int required_n_resources = 0;

        if (current_circuit_n_resources == 0){
            continue;
        }

        if (circuit->get_node_kind() == SUB_CIRCUIT){
            auto ext_scope_pred = [](const auto& elem){ return scope_matches(elem->get_scope(), Scope::EXT); };
            auto circuit_resources = circuit->get_coll<Resource>(rk);
        
            required_n_resources = size_pred<Resource>(circuit_resources, ext_scope_pred);
        
        } else {
            required_n_resources = (rk == Resource_kind::QUBIT) ? circuit->get_n_matrix_qubits() : 0;
        }

        if ((current_circuit_n_resources < 1) || (current_circuit_n_resources < required_n_resources)){
            return false;
        }
    }

    return true;
}

bool Context::current_circuit_uses_subroutines(){
    for(std::shared_ptr<Circuit> circuit : circuits){
        if (can_apply_as_subroutine(circuit)){
            return true;
        }
    }

    return false;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wswitch"
Expr_type Context::resolve_var(const Token_kind name, const std::vector<Expr_type>& args) const {
    auto gate = get_current_node<Gate>();

    switch(name){
        case MAKE_FLOAT:
            return uniform_float(10.0, 0.0);

        case MAKE_VAR:
            return uniform_str(5);

        case MAKE_INTEGER:
            return (int)uniform_uint(10, 0);

        case GET_CIRCUIT_NAME:
            return get_current_circuit()->get_name();

        case GET_GATE_NAME:
            return gate->get_str();

        case GET_DEF_NAME:
            return get_current_node<Resource_def>()->get_var_name();

        case GET_SIZE:
            return (int)get_current_node<Resource_def>()->get_size();

        case GET_DECL_NAME:
            return get_current_node<Resource>()->get_var_name();

        case GET_INDEX:
            return (int)get_current_node<Resource>()->get_index();

        case GET_GATE_SOURCE:
            return gate->get_gate_source();

        case GET_GATE_QUBITS:
            return (int)gate->get_num_external_resources(Resource_kind::QUBIT);

        case GET_GATE_BITS:
            return (int)gate->get_num_external_resources(Resource_kind::BIT);

        case GET_GATE_PARAMS:
            return (int)gate->get_num_external_resources(Resource_kind::PARAM);

        case GET_TOTAL_QUBITS: {
            auto qubits = get_current_circuit()->get_coll<Resource>(Resource_kind::QUBIT);
            return (int)qubits.size();
        }

        case GET_TOTAL_BITS: {
            auto bits = get_current_circuit()->get_coll<Resource>(Resource_kind::BIT);
            return (int)bits.size();
        }

        case GET_MAT_POS:
            if ((args.size() == 2) && 
                std::holds_alternative<int>(args[0]) && std::holds_alternative<int>(args[1])) {
                return get_current_circuit()->get_val_at(std::get<int>(args[0]), std::get<int>(args[1]));
            }
            break;

        case UNIFORM:
            if ((args.size() == 2)) {
                if (std::holds_alternative<int>(args[0]) && std::holds_alternative<int>(args[1])){
                    auto arg0 = std::get<int>(args[0]);
                    auto arg1 = std::get<int>(args[1]);
                    return (int)uniform_uint(std::max(arg0, arg1), std::min(arg0, arg1));

                } else if (std::holds_alternative<float>(args[0]) && std::holds_alternative<float>(args[1])){
                    auto arg0 = std::get<float>(args[0]);
                    auto arg1 = std::get<float>(args[1]);
                    return uniform_float(std::max(arg0, arg1), std::min(arg0, arg1));

                } else {
                    ERROR("ARGS to UNIFORM must resolve to int or float");
                }
            }
            break;

        case AST_HAS_NODE:
            if (args.size() == 1) {
                if (std::holds_alternative<std::shared_ptr<Rule>>(args[0])){
                    std::string node_name = std::get<std::shared_ptr<Rule>>(args[0])->get_name();

                    for (const std::shared_ptr<Circuit>& circ : circuits){
                        if (circ->find(node_name) != nullptr){
                            return true;
                        }
                    }

                    return false;
                } else {
                    ERROR("AST_HAS_NODE expected argument to evaluate to ptr<Rule>");
                }
     
            } else {
                ERROR("AST_HAS_NODE expected 1 argument, got " + std::to_string(args.size()));
            }
            break;

        case GET_CIRCUIT_ID:
            return ast_counter;
    }

    return 0;
}
#pragma GCC diagnostic pop

/// In normal cases, current circuit is the last added circuit into the circuits vector. The exception is if we are no longer under the `subroutines`
/// node in the AST, but the last added circuit is a subroutine. This implies that after the `subroutines` node, there's no `circuit` node to generate
/// a new, main circuit. As such, qubit and bit definitions, qubits and bits may have been made globally, and therefore stored in the dummy circuit, so we return that
std::shared_ptr<Circuit> Context::get_current_circuit() const {
    if((circuits.size() == 0) || (!under_subroutine_defs_node() && circuits.back()->check_if_sub_circuit())){
        return dummy_circuit;
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

        std::shared_ptr<Circuit> circuit = circuits.at(uniform_uint(circuits.size()-1));

        while(!can_apply_as_subroutine(circuit)){
            circuit = circuits.at(uniform_uint(circuits.size()-1));
        }

        return circuit;

    } else {
        ERROR("No available circuits to use as subroutines!");
        return dummy_circuit;
    }
}


std::shared_ptr<Resource> Context::get_random_resource(Resource_kind rk, Scope scope){
    Ptr_pred_type<Resource> pred = [](const std::shared_ptr<Resource>& elem){ return !elem->is_used(); };

    auto filtered_coll = (scope == Scope::GLOB) ?
        dummy_circuit->get_coll<Resource>(rk) :
        get_current_circuit()->get_coll<Resource>(rk);

    auto random_resource = get_random_from_coll<Resource>(filtered_coll, pred);
    random_resource->set_used();

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
    } else if(rk == Resource_kind::BIT){
        can_use_reg = !control.get_rule("register_bit_def", scope)->is_empty();
        can_use_sing = !control.get_rule("singular_bit_def", scope)->is_empty();
    } else if (rk == Resource_kind::PARAM){
        can_use_reg = !control.get_rule("register_param_def", scope)->is_empty();
        can_use_sing = !control.get_rule("singular_param_def", scope)->is_empty();
    } else {
        ERROR("Unknown resource kind!");
    }

    if (can_use_reg && can_use_sing){
        is_reg = uniform_uint(1, 0);
    } else if (can_use_reg){
        is_reg = true;
    } else {
        is_reg = false;
    }

    def = std::make_shared<Resource_def>(scope, rk, is_reg, uniform_uint(control.get_value("MAX_REG_SIZE"), 1));

    current.set<Resource_def>(def);
    get_current_circuit()->store_resource_def(def);

    return def;
}

std::shared_ptr<Circuit> Context::nn_circuit(){
    reset(RL_CIRCUIT);
    std::shared_ptr<Circuit> current_circuit = std::make_shared<Circuit>(QuteFuzz::TOP_LEVEL_CIRCUIT_NAME, CIRCUIT);
    subroutine_counter = 0;
    circuits.push_back(current_circuit);
    return current_circuit;
}

std::shared_ptr<Circuit> Context::nn_sub_circuit(){
    reset(RL_CIRCUIT);
    std::shared_ptr<Circuit> current_circuit = std::make_shared<Circuit>("sub_" + std::to_string(subroutine_counter++), SUB_CIRCUIT);
    circuits.push_back(current_circuit);
    return current_circuit;
}

std::shared_ptr<Circuit> Context::nn_unitary(unsigned int n_qubits){
    reset(RL_CIRCUIT);
    std::shared_ptr<Circuit> current_circuit = std::make_shared<Circuit>("unitary_" + std::to_string(subroutine_counter++), n_qubits);
    circuits.push_back(current_circuit);
    return current_circuit;
}

std::shared_ptr<Gate> Context::nn_gate(const std::string& str, const Token_kind& kind){
    auto gate = std::make_shared<Gate>(str, kind);

    current.set<Gate>(gate);
    current.get<Qubit_op>()->set_gate_node(gate);

    return gate;
}

std::shared_ptr<Gate> Context::nn_subroutine_op(){
    /*
        1. get random circuit to use as subroutine
        2. get qubit defs inside subroutine, create the gate node
        3. give the gate its name
        4. set current gate to be this new node
        5. give this gate to the current qubit op
    */
    std::shared_ptr<Circuit> sub_circuit = get_random_circuit();
    std::string gate_name = sub_circuit->get_name();
    Token_kind circ_kind = sub_circuit->get_node_kind();

    std::shared_ptr<Gate> gate;

    if (circ_kind == SUB_CIRCUIT){
        gate = std::make_shared<Gate>(gate_name, sub_circuit->get_coll<Resource_def>());

    } else if ((circ_kind == UNITARY_1Q_DEF) || (circ_kind == UNITARY_2Q_DEF)){
        gate = std::make_shared<Gate>(gate_name, sub_circuit->get_n_matrix_qubits());

    } else {
        ERROR("Cannot create gate from node of kind " + kind_as_str(circ_kind));
    }

    // gate->add_child(std::make_shared<Variable>(gate_name));

    current.set<Gate>(gate);
    current.get<Qubit_op>()->set_gate_node(gate);

    return gate;
}

std::shared_ptr<Compound_stmt> Context::nn_compound_stmt(){
    return std::make_shared<Compound_stmt>(nested_depth);
}

std::shared_ptr<Node> Context::nn_subroutine_defs(){
    subroutine_defs_node = std::make_shared<Node>("", SUBROUTINE_DEFS);
    return subroutine_defs_node;
}

std::shared_ptr<Qubit_op> Context::nn_qubit_op(){
    reset(RL_QUBITS);
    reset(RL_BITS);

    auto qubit_op = std::make_shared<Qubit_op>(get_current_circuit()->get_name());
    current.set<Qubit_op>(qubit_op);
    return qubit_op;
}


