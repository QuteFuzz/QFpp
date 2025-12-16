#include <block.h>

std::shared_ptr<Qubit> Block::get_random_qubit(const U8& scope){
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

        #ifdef DEBUG
        INFO("Getting random qubit");
        #endif

        // std::cout << *this << std::endl;

        std::shared_ptr<Qubit> qubit = qubits.at(random_int(total_qubits - 1));

        while(qubit->is_used() || !scope_matches(qubit->get_scope(), scope)){
            qubit = qubits.at(random_int(total_qubits - 1));
        }

        qubit->set_used();

        return qubit;
    
    } else {
        ERROR("No more available qubits!");
        return dummy_qubit;
    }
}

std::shared_ptr<Bit> Block::get_random_bit(const U8& scope){
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

        #ifdef DEBUG
        INFO("Getting random bit");
        #endif

        std::shared_ptr<Bit> bit = bits.at(random_int(total_bits - 1));

        while(bit->is_used() || !scope_matches(bit->get_scope(), scope)){
            bit = bits.at(random_int(total_bits - 1));
        }

        bit->set_used();

        return bit;
    
    } else {
        ERROR("No more available bits!");
        return dummy_bit;
    }
}


std::shared_ptr<Qubit_definition> Block::get_next_qubit_def(const U8& scope){
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

std::shared_ptr<Qubit_definition> Block::get_next_qubit_def_discard(const U8& scope){
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

std::shared_ptr<Bit_definition> Block::get_next_bit_def(const U8& scope){
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
unsigned int Block::make_register_resource_definition(unsigned int max_size, U8& scope, Resource_kind rk, unsigned int& total_definitions){

    unsigned int size;

    if(max_size > 1) size = random_int(max_size, 1);
    else size = max_size;

    if (rk == QUBIT) {
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
unsigned int Block::make_singular_resource_definition(U8& scope, Resource_kind rk, unsigned int& total_definitions){
    if (rk == QUBIT) {
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

unsigned int Block::make_resource_definitions(U8& scope, Resource_kind rk, bool discard_defs){

    if (discard_defs) {
        //Simply report back how many qubit defs are there
        if (rk == QUBIT) {
            return qubit_defs.get_num_of(scope);
        } else {
            return bit_defs.get_num_of(scope);
        }
    } else {

        unsigned int target_num_resources = 0, total_num_definitions = 0;
        bool scope_is_external = (scope & EXTERNAL_SCOPE);
        bool classificaton_is_qubit = (rk == QUBIT);
        
        switch((scope_is_external << 1) | classificaton_is_qubit){
            case 0b00: target_num_resources = target_num_bits_internal; break;
            case 0b01: target_num_resources = target_num_qubits_internal; break;
            case 0b10: target_num_resources = target_num_bits_external; break;
            case 0b11: target_num_resources = target_num_qubits_external; break;
            default: ERROR("Scope and rk failed to pick target num of resources!");
        }

        while(target_num_resources > 0){
            /*
                Use singular qubit or qubit register
            */
            if(random_int(1)){
                target_num_resources -= make_singular_resource_definition(scope, rk, total_num_definitions);

            } else {
                target_num_resources -= make_register_resource_definition(target_num_resources, scope, rk, total_num_definitions);
            }
        }

        return total_num_definitions;
    }
}

unsigned int Block::make_resource_definitions(const Dag::Dag& dag, const U8& scope, Resource_kind rk, bool discard_defs){

    if (discard_defs) {
        return qubit_defs.get_num_of(scope);
    } else {
        if(rk == QUBIT){
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
 
void Block::print_info() const {

    std::cout << "=======================================" << std::endl;
    std::cout << "              BLOCK INFO               " << std::endl;
    std::cout << "=======================================" << std::endl;
    std::cout << "Owner: " << owner << " [can apply subroutines: " << (can_apply_subroutines ? "true" : "false") << "]" << std::endl;

    std::cout << "Target num qubits " << std::endl;
    std::cout << " EXTERNAL: " << target_num_qubits_external << std::endl;
    std::cout << " INTERNAL: " << target_num_qubits_internal << std::endl;

    std::cout << "Target num bits " << std::endl;
    std::cout << " EXTERNAL: " << target_num_bits_external << std::endl;
    std::cout << " INTERNAL: " << target_num_bits_internal << std::endl;

    std::cout << std::endl;
    std::cout << "Qubit definitions " << std::endl;

    if(owner == Common::TOP_LEVEL_CIRCUIT_NAME){
        std::cout << YELLOW("Qubit defs may not match target if block is built to match DAG") << std::endl;
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
