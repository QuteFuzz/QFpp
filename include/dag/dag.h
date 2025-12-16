#ifndef DAG_H
#define DAG_H

#include <node.h>
#include <collection.h>
#include <qubit_op.h>

namespace Dag {

    class Edge {

        public:
            Edge(){}

            Edge(unsigned int _src_port, unsigned int _dest_port, std::shared_ptr<Qubit_op> _node): 
                src_port(_src_port),
                dest_port(_dest_port),
                node(_node)
            {}

            inline std::shared_ptr<Qubit_op> get_node() const {
                return node;
            }

            inline unsigned int get_dest_port() const {
                return dest_port;
            }

            std::string get_node_resolved_name() const {
                return "\"" + node->resolved_name() + "\"";
            }

            inline int get_node_id() const {
                return node->get_id();
            }

            friend std::ostream& operator<<(std::ostream& stream, const Edge& edge) {
                stream << " -> " << edge.get_node_resolved_name() << "[label=\"" << std::to_string(edge.src_port) << ", " << std::to_string(edge.dest_port) << "\"";
                return stream;
            }

        private:
            unsigned int src_port;
            unsigned int dest_port;
            std::shared_ptr<Qubit_op> node;
    };

    struct Node_data{
        std::shared_ptr<Qubit_op> node;

        std::vector<unsigned int> inputs; // each index is a port into the node, the value at each port is the id for the qubit entering that port
        std::vector<unsigned int> children; // node ids of child nodes

        inline unsigned int in_degree() const {
            return inputs.size();
        }

        inline unsigned int out_degree() const {
            return children.size();
        }
    };

    /// @brief Given a set of qubits, get DAG score using the path taken by each qubit
    class Dag {
        
        public:
            Dag(){}

            void make_dag(const std::shared_ptr<Block> block);

            inline Collection<Qubit_definition> get_qubit_defs() const {
                return qubit_defs;
            }

            inline Collection<Bit_definition> get_bit_defs() const {
                return bit_defs;
            }

            inline Collection<Qubit> get_qubits() const {
                return qubits;
            }

            inline Collection<Bit> get_bits() const {
                return bits;
            }

            void add_edge(const Edge& edge, std::optional<int> maybe_dest_node_id, int qubit_id);

            void render_dag(const fs::path& current_circuit_dir);

            int max_out_degree();

            int score();

            unsigned int n_qubit_ops() const {
                return nodewise_data.size();
            }

            unsigned int n_subroutines() const {
               return subroutine_gates.size();
            }

            inline std::shared_ptr<Qubit_op> get_next_node(){
                if(node_pointer < nodewise_data.size()){
                    return nodewise_data.at(node_pointer++).node;
                } else {
                    return dummy_qubit_op;
                }
            }

            inline std::shared_ptr<Node> get_next_subroutine_gate(){
                if(sub_pointer < subroutine_gates.size()){
                    return subroutine_gates.at(sub_pointer++);
                } else {
                    return dummy_node;
                }
            }

            std::optional<unsigned int> nodewise_data_contains(std::shared_ptr<Qubit_op> node);

            friend std::ostream& operator<<(std::ostream& stream, const Dag& dag){

                stream << "=========================================" << std::endl;
                stream << "                DAG INFO                 " << std::endl; 
                stream << "=========================================" << std::endl;

                stream << "N_NODES (all nodes are Qubit_ops): " << dag.n_qubit_ops() << std::endl;
                stream << "N_SUBROUTINES: " << dag.n_subroutines() << std::endl;

                for(const auto& node_data : dag.nodewise_data){

                    stream << node_data.node->get_id() << " -> children: ";

                    for(const unsigned int& child_id : node_data.children){
                        stream << child_id << ", ";
                    }

                    stream << "in_degree: " << node_data.in_degree() << ", out_degree: " << node_data.out_degree() << std::endl;
                }

                return stream;
            }

            inline void reset(){
                nodewise_data.clear();
                node_pointer = 0;
                subroutine_gates.clear();
                sub_pointer = 0;
            }

        private:
            std::vector<Node_data> nodewise_data;
            unsigned int node_pointer = 0;
            std::vector<std::shared_ptr<Node>> subroutine_gates;
            unsigned int sub_pointer = 0;

            Collection<Qubit> qubits;
            Collection<Qubit_definition> qubit_defs;
            
            Collection<Bit> bits;
            Collection<Bit_definition> bit_defs;

            std::shared_ptr<Node> dummy_node = std::make_shared<Node>("");
            std::shared_ptr<Qubit_op> dummy_qubit_op = std::make_shared<Qubit_op>();
    };

};

#endif
