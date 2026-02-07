// #include <resource.h>
// #include <dag.h>
// #include <circuit.h>

// /// TODO: fix dag stuff, line 45

// Dag::Dag(const std::shared_ptr<Circuit> circuit){
//     reset();

//     qubits = circuit->get_coll<Resource>(Resource_kind::QUBIT);
//     qubit_defs = circuit->get_coll<Resource_def>(Resource_kind::QUBIT);

//     for(const auto& qubit : qubits){
//         qubit->add_path_to_dag(*this);
//     }
// }

// std::optional<unsigned int> Dag::nodewise_data_contains(std::shared_ptr<Qubit_op> node){

//     for(unsigned int i = 0; i < nodewise_data.size(); i++){
//         if(nodewise_data[i].node->get_id() == node->get_id()){
//             return std::make_optional<unsigned int>(i);
//         }
//     }

//     return std::nullopt;
// }


// void Dag::add_edge(const Edge& edge, std::optional<int> maybe_dest_node_id, int qubit_id){

//     unsigned int source_node_input_port = edge.get_dest_port();
//     std::shared_ptr<Qubit_op> source_node = edge.get_node();

//     std::optional<unsigned int> maybe_pos = nodewise_data_contains(source_node);
//     unsigned int pos = maybe_pos.value_or(nodewise_data.size());

//     if(maybe_pos.has_value() == false){
//         nodewise_data.push_back(Node_data{.node = source_node, .inputs = {}, .children = {}});

//         // reserve memory for inputs depending on number of ports this gate has
//         nodewise_data.at(pos).inputs.resize(source_node->get_n_ports() + 2, 0); // +1 is for 0-indexing, +1 is for safety
//     }

//     if(maybe_dest_node_id.has_value()) nodewise_data.at(pos).children.push_back(maybe_dest_node_id.value());

//     nodewise_data.at(pos).inputs[source_node_input_port] = qubit_id;

//     source_node->add_gate_if_subroutine(subroutine_gates);
// }

// int Dag::max_out_degree(){
//     unsigned int curr_max = 0;

//     for(const auto&data : nodewise_data){
//         curr_max = std::max(curr_max, data.out_degree());
//     }

//     return curr_max;
// }

// /// @brief Combine heuristics to get dag score
// /// @return
// int Dag::score(){
//     return max_out_degree();
// }

// void Dag::extend_dot_string(std::ostringstream& dot_string){
//     for(const auto& qubit : qubits){
//         qubit->extend_dot_string(dot_string);
//     }
// }
