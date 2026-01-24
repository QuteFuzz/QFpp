#include <circuit.h>
#include <rule.h>

std::shared_ptr<Qubit> Circuit::get_random_qubit(const U8& scope){
    size_t total_qubits = qubits.get_num_of(scope);
    //Iterate through all qubits and see if there is at least one valid bit to prevent infinite loop
    bool valid_qubit_exists = false;
    for (auto qubit : qubits) {
        if (!qubit->is_used() && scope_matches(qubit->get_scope(), scope)) {
            valid_qubit_exists = true;
            break;
        }
    }

    if(total_qubits && valid_qubit_exists){
        std::shared_ptr<Qubit> qubit = qubits.at(random_uint(total_qubits - 1));

        while(qubit->is_used() || !scope_matches(qubit->get_scope(), scope)){
            qubit = qubits.at(random_uint(total_qubits - 1));
        }

        qubit->set_used();

        return qubit;

    } else {
        ERROR("No more available qubits!");
        return dummy_qubit;
    }
}

std::shared_ptr<Bit> Circuit::get_random_bit(const U8& scope){
    size_t total_bits = bits.get_num_of(scope);
    //Iterate through all bits and see if there is at least one valid bit to prevent infinite loop
    bool valid_bit_exists = false;
    for (auto bit : bits) {
        if (!bit->is_used() && scope_matches(bit->get_scope(), scope)) {
            valid_bit_exists = true;
            break;
        }
    }

    if(total_bits && valid_bit_exists){

        std::shared_ptr<Bit> bit = bits.at(random_uint(total_bits - 1));

        while(bit->is_used() || !scope_matches(bit->get_scope(), scope)){
            bit = bits.at(random_uint(total_bits - 1));
        }

        bit->set_used();

        return bit;

    } else {
        ERROR("No more available bits!");
        return dummy_bit;
    }
}


std::shared_ptr<Qubit_definition> Circuit::get_next_qubit_def(const U8& scope){
    auto maybe_def = qubit_defs.at(qubit_def_pointer++);

    while((maybe_def != nullptr) && !scope_matches(maybe_def->get_scope(), scope)){
        maybe_def = qubit_defs.at(qubit_def_pointer++);
    }

    if(maybe_def == nullptr){
        return dummy_qubit_def;
    } else {
        return maybe_def;
    }
}

std::shared_ptr<Qubit_definition> Circuit::get_next_qubit_def_discard(const U8& scope){
    auto maybe_def = qubit_defs_discard.at(qubit_def_discard_pointer++);

    while((maybe_def != nullptr) && !scope_matches(maybe_def->get_scope(), scope)){
        maybe_def = qubit_defs_discard.at(qubit_def_discard_pointer++);
    }

    if(maybe_def == nullptr){
        return dummy_qubit_def;
    } else {
        return maybe_def;
    }
}

std::shared_ptr<Bit_definition> Circuit::get_next_bit_def(const U8& scope){
    auto maybe_def = bit_defs.at(bit_def_pointer++);

    while((maybe_def != nullptr) && !scope_matches(maybe_def->get_scope(), scope)){
        maybe_def = bit_defs.at(bit_def_pointer++);
    }

    if(maybe_def == nullptr){
        return dummy_bit_def;
    } else {
        return maybe_def;
    }
}

/// @brief Make a register resource definition, whose size is bounded by `max_size`
/// @param max_size
/// @param scope
/// @param rk
/// @param total_definitions
/// @return number of resources created from this definition
unsigned int Circuit::make_register_resource_definition(unsigned int max_size, U8& scope, Resource_kind rk, unsigned int& total_definitions){

    unsigned int size;

    if(max_size > 1) size = random_uint(max_size, 1);
    else size = max_size;

    if (rk == RK_QUBIT) {
        Register_qubit_definition def(
            Variable("qreg" + std::to_string(qubit_defs.get_num_of(ALL_SCOPES))),
            Integer(size)
        );

        def.make_resources(qubits, scope);

        qubit_defs.add(Qubit_definition(def, scope));
        qubit_defs_discard.add(Qubit_definition(def, scope, true)); //Add qubit defs with discard flag set to true for later traversal if needed

    } else {
        Register_bit_definition def(
            Variable("creg" + std::to_string(bit_defs.get_num_of(ALL_SCOPES))),
            Integer(size)
        );

        def.make_resources(bits, scope);

        bit_defs.add(Bit_definition(def, scope));
    }

    total_definitions += 1;

    return size;
}

/// @brief Make singular resource definition
/// @param scope
/// @param rk
/// @param total_definitions
/// @return 1, since there's one qubit created from a singular resource definition
unsigned int Circuit::make_singular_resource_definition(U8& scope, Resource_kind rk, unsigned int& total_definitions){
    if (rk == RK_QUBIT) {
        Singular_qubit_definition def (
            Variable("qubit" + std::to_string(qubit_defs.get_num_of(ALL_SCOPES)))
        );

        def.make_resources(qubits, scope);

        qubit_defs.add(Qubit_definition(def, scope));
        qubit_defs_discard.add(Qubit_definition(def, scope, true));

    } else {
        Singular_bit_definition def (
            Variable("bit" + std::to_string(bit_defs.get_num_of(ALL_SCOPES)))
        );

        def.make_resources(bits, scope);

        bit_defs.add(Bit_definition(def, scope));
    }

    total_definitions += 1;

    return 1;
}

unsigned int Circuit::make_resource_definitions(U8& scope, Resource_kind rk, Control& control, bool discard_defs){

    if (discard_defs) {
        //Simply report back how many qubit defs are there
        if (rk == RK_QUBIT) {
            return qubit_defs.get_num_of(scope);
        } else {
            return bit_defs.get_num_of(scope);
        }

    } else {
        unsigned int total_num_definitions = 0;
        unsigned int target_num_resources = get_resource_target(scope, rk);

        bool can_generate_register_resource = true;
        bool can_generate_singular_resource = true;
        std::shared_ptr<Rule> resource_def_rule;

        if(rk == RK_QUBIT){
            resource_def_rule = control.get_rule("qubit_def", scope);
            can_generate_register_resource = resource_def_rule->contains_rule(REGISTER_QUBIT_DEF);
            can_generate_singular_resource = resource_def_rule->contains_rule(SINGULAR_QUBIT_DEF);

        } else {
            resource_def_rule = control.get_rule("bit_def", scope);
            can_generate_register_resource = resource_def_rule->contains_rule(REGISTER_BIT_DEF);
            can_generate_singular_resource = resource_def_rule->contains_rule(SINGULAR_BIT_DEF);
        }

        // std::cout << "reg re: " << can_generate_register_resource << " sig re: " << can_generate_singular_resource << std::endl;
        // std::cout << owner << std::endl;

        while(target_num_resources > 0){
            /*
                Use singular qubit or qubit register
                Checks whether you have a choice depending on grammar definition
            */
            if(can_generate_register_resource && can_generate_singular_resource) {
                target_num_resources -= (random_uint(1) ? make_singular_resource_definition(scope, rk, total_num_definitions) :
                    make_register_resource_definition(target_num_resources, scope, rk, total_num_definitions));
            } else if (can_generate_register_resource) {
                target_num_resources -= make_register_resource_definition(target_num_resources, scope, rk, total_num_definitions);
            } else if (can_generate_singular_resource) {
                target_num_resources -= make_singular_resource_definition(scope, rk, total_num_definitions);
            } else {
                throw std::runtime_error("Resource def MUST have at least one register or singular resource definition");
            }
        }

        return total_num_definitions;
    }
}

unsigned int Circuit::make_resource_definitions(const Dag& dag, const U8& scope, Resource_kind rk, bool discard_defs){

    if (discard_defs) {
        return qubit_defs.get_num_of(scope);
    } else {
        if(rk == RK_QUBIT){
            qubits = dag.get_qubits();
            qubit_defs = dag.get_qubit_defs();

            return qubit_defs.get_num_of(scope);

        } else {
            bits = dag.get_bits();
            bit_defs = dag.get_bit_defs();

            return bit_defs.get_num_of(scope);
        }
    }
}

void Circuit::print_info() const {

    std::cout << "=======================================" << std::endl;
    std::cout << "              CIRCUIT INFO               " << std::endl;
    std::cout << "=======================================" << std::endl;
    std::cout << "Owner: " << owner << " [can apply subroutines: " << (can_apply_subroutines ? "true" : "false") << "]" << std::endl;

    std::cout << "Target num qubits " << std::endl;
    std::cout << " EXTERNAL: " << target_num_qubits_external << std::endl;
    std::cout << " INTERNAL: " << target_num_qubits_internal << std::endl;
    std::cout << " GLOBAL: " << target_num_qubits_global << std::endl;

    std::cout << "Target num bits " << std::endl;
    std::cout << " EXTERNAL: " << target_num_bits_external << std::endl;
    std::cout << " INTERNAL: " << target_num_bits_internal << std::endl;
    std::cout << " GLOBAL: " << target_num_bits_global << std::endl;

    std::cout << std::endl;
    std::cout << "Qubit definitions " << std::endl;

    if(owner == QuteFuzz::TOP_LEVEL_CIRCUIT_NAME){
        std::cout << YELLOW("Qubit defs may not match target if circuit is built to match DAG") << std::endl;
    }

    for(const auto& qubit_def : qubit_defs){
        std::cout << "name: " << qubit_def->get_name()->get_content() << " " ;

        if(qubit_def->is_register_def()){
            std::cout << "size: " << qubit_def->get_size()->get_content();
        }

        std::cout << STR_SCOPE(qubit_def->get_scope()) << std::endl;
    }

    std::cout << "Bit definitions " << std::endl;

    for(const auto& bit_def : bit_defs){
        std::cout << "name: " << bit_def->get_name()->get_content() << " " ;

        if(bit_def->is_register_def()){
            std::cout << "size: " << bit_def->get_size()->get_content();
        }

        std::cout << STR_SCOPE(bit_def->get_scope()) << std::endl;
    }

    std::cout << "=======================================" << std::endl;
}
