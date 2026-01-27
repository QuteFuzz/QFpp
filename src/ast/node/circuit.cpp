#include <circuit.h>
#include <rule.h>


unsigned int Circuit::make_register_resource_definition(U8& scope, Resource_kind rk, unsigned int max_size, unsigned int& current_num_definitions){

    unsigned int size;

    if(max_size > 1) size = random_uint(max_size, 1);
    else size = max_size;

    if (rk == RK_QUBIT) {

        Register_qubit_definition def(
            Variable("qreg" + std::to_string(current_num_definitions)),
            Integer(size)
        );

        def.make_resources(qubits, scope);
        qubit_defs.push_back(std::make_shared<Qubit_definition>(def, scope));

    } else {

        Register_bit_definition def(
            Variable("creg" + std::to_string(current_num_definitions)),
            Integer(size)
        );

        def.make_resources(bits, scope);
        bit_defs.push_back(std::make_shared<Bit_definition>(def, scope));
    }

    current_num_definitions += 1;

    return size;
}

unsigned int Circuit::make_singular_resource_definition(U8& scope, Resource_kind rk, unsigned int& total_definitions){

    if (rk == RK_QUBIT) {
        Singular_qubit_definition def (
            Variable("qubit" + std::to_string(total_definitions))
        );

        def.make_resources(qubits, scope);
        qubit_defs.push_back(std::make_shared<Qubit_definition>(def, scope));

    } else {

        Singular_bit_definition def (
            Variable("bit" + std::to_string(total_definitions))
        );

        def.make_resources(bits, scope);

        bit_defs.push_back(std::make_shared<Bit_definition>(def, scope));

    }
    total_definitions += 1;

    return 1;
}

unsigned int Circuit::make_resource_definitions(U8& scope, Resource_kind rk, Control& control){

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

    while(target_num_resources > 0){
        /*
            Use singular qubit or qubit register
            Checks whether you have a choice depending on grammar definition
        */
        if(can_generate_register_resource && can_generate_singular_resource) {
            target_num_resources -= (random_uint(1) ?
                make_singular_resource_definition(scope, rk, total_num_definitions) :
                make_register_resource_definition(scope, rk, target_num_resources, total_num_definitions));

        } else if (can_generate_register_resource) {
            target_num_resources -= make_register_resource_definition(scope, rk, target_num_resources, total_num_definitions);

        } else if (can_generate_singular_resource) {
            target_num_resources -= make_singular_resource_definition(scope, rk, total_num_definitions);

        } else {
            throw std::runtime_error("Resource def MUST have at least one register or singular resource definition");
        }
    }

    return total_num_definitions;
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
