#include <info.h>

Quality::Quality(Slot_type _compilation_unit, const Features& fv):
    Info(_compilation_unit, std::make_shared<Features>(fv))
{
    for(auto& gate : gates){
        Token_kind kind = gate->get_node_kind();
        if (gate_occurances.find(kind) == gate_occurances.end()){
            gate_occurances[kind] = 1;
        } else {
            gate_occurances[kind] += 1;
        }
    }

    components = {
        Component{"gate_arity_variance", gate_arity_variance(), 1.3f},
        Component{"gate_type_entropy", gate_type_entropy(), 1.2f},
        // Component{"non_adj_gate_pair_density", non_adj_gate_pair_density()},
        Component{"max_control_flow_depth", (float)max_control_flow_depth(), 2.2f},
    };
}

unsigned int Quality::max_control_flow_depth(){
    return max_control_flow_depth_rec(*compilation_unit, 0);
}
