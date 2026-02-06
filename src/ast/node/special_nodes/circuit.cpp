#include <circuit.h>
#include <rule.h>

void Circuit::print_info() const {

    std::cout << "=======================================" << std::endl;
    std::cout << "              CIRCUIT INFO               " << std::endl;
    std::cout << "=======================================" << std::endl;
    std::cout << "Owner: " << owner << " [can apply subroutines: " << (can_apply_subroutines ? "true" : "false") << "]" << std::endl;
    std::cout << "--->" << this << std::endl;

    std::cout << "Resources " << std::endl;

    for(const auto& r : resources){
        std::cout << *r << std::endl;
    }

    std::cout << "Resource defs" << std::endl;

    for(const auto& rd : resource_defs){
        std::cout << *rd << std::endl;
    }
    std::cout << "=======================================" << std::endl;
}
