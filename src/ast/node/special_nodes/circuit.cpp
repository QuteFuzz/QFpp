#include <circuit.h>
#include <rule.h>

// void Circuit::print_info() const {

//     std::cout << "=======================================" << std::endl;
//     std::cout << "              CIRCUIT INFO               " << std::endl;
//     std::cout << "=======================================" << std::endl;
//     std::cout << "Owner: " << owner << " [can apply subroutines: " << (can_apply_subroutines ? "true" : "false") << "]" << std::endl;

//     std::cout << "Target num qubits " << std::endl;
//     std::cout << " EXTERNAL: " << target_num_qubits_external << std::endl;
//     std::cout << " INTERNAL: " << target_num_qubits_internal << std::endl;
//     std::cout << " GLOBAL: " << target_num_qubits_global << std::endl;

//     std::cout << "Target num bits " << std::endl;
//     std::cout << " EXTERNAL: " << target_num_bits_external << std::endl;
//     std::cout << " INTERNAL: " << target_num_bits_internal << std::endl;
//     std::cout << " GLOBAL: " << target_num_bits_global << std::endl;

//     std::cout << std::endl;
//     std::cout << "Qubit definitions " << std::endl;

//     if(owner == QuteFuzz::TOP_LEVEL_CIRCUIT_NAME){
//         std::cout << YELLOW("Qubit defs may not match target if circuit is built to match DAG") << std::endl;
//     }

//     for(const auto& qubit_def : qubit_defs){
//         std::cout << "name: " << qubit_def->get_name()->get_content() << " " ;

//         if(qubit_def->is_register_def()){
//             std::cout << "size: " << qubit_def->get_size()->get_content();
//         }

//         std::cout << STR_SCOPE(qubit_def->get_scope()) << std::endl;
//     }

//     std::cout << "Bit definitions " << std::endl;

//     for(const auto& bit_def : bit_defs){
//         std::cout << "name: " << bit_def->get_name()->get_content() << " " ;

//         if(bit_def->is_register_def()){
//             std::cout << "size: " << bit_def->get_size()->get_content();
//         }

//         std::cout << STR_SCOPE(bit_def->get_scope()) << std::endl;
//     }

//     std::cout << "=======================================" << std::endl;
// }
