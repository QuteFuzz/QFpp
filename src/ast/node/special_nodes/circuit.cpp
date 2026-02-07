#include <circuit.h>
#include <rule.h>

void Circuit::print_info() const {

    std::cout << "=======================================" << std::endl;
    std::cout << "              CIRCUIT INFO               " << std::endl;
    std::cout << "=======================================" << std::endl;
    std::cout << "Owner: " << owner << std::endl;
    std::cout << "---> " << this << std::endl;

    std::cout << "Resource defs" << std::endl;

    for(const auto& rd : resource_defs){
        rd->print_info();
    }

    std::cout << "Resources " << std::endl;

    for(const auto& r : resources){
        r->print_info();
    }

    std::cout << "=======================================" << std::endl;
}
