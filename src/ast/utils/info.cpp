#include <info.h>
#include <node_gen.h>
#include <ast_utils.h>

Info::Info(Slot_type _compilation_unit, std::shared_ptr<Info> info) :
    compilation_unit(_compilation_unit)
{

    if (info == nullptr){
        for (const auto& node : Node_gen(**compilation_unit, QUBIT_OP)){
            auto qubit_op = std::dynamic_pointer_cast<Qubit_op>(node);

            if (qubit_op == nullptr){
                node->print_program(std::cout);
                std::cout << std::endl;
                
                node->print_ast("");
                std::cout << std::endl;

                ERROR("Nodes of kind `QUBIT_OP` must be of `Qubit_op` type");
            }
        
            qubit_ops.push_back(qubit_op);
            gates.push_back(gate_from_qubit_op(qubit_op));
        }

    } else {
        qubit_ops = std::move(info->get_qubit_ops());
        gates = std::move(info->get_gates());
    }

    if (qubit_ops.size() != gates.size()) {
        std::cout << "qubit_ops.size() " << qubit_ops.size() << " n_gates " << gates.size() << std::endl;
        ERROR("Expected the number of `QUBIT_OP` nodes to match the number of nodes of type `Gate`");
    }

    # if 0
    if (qubit_ops.size() == 0) {
		WARNING("Ciruit has no `QUBIT_OP` nodes");
		(*compilation_unit)->print_program(std::cout);

	}
    #endif
}
