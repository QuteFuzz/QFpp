#ifndef GATE_H
#define GATE_H

#include <node.h>
#include <coll.h>
#include <supported_gates.h>

class Resource_def;
enum class Resource_kind;

class Gate : public Node {

    public:

        Gate() :
            Node("dummy")
        {}

        /// @brief Use for predefined gates
        /// @param str
        /// @param kind
        Gate(const std::string& str, const Token_kind& kind);

        // Gate(const std::string& str, const Token_kind& kind, unsigned int n_qubits);

        /// @brief Use for subroutines
        /// @param str
        /// @param kind
        /// @param qubit_defs
        Gate(const std::string& str, const Token_kind& kind, const Ptr_coll<Resource_def>& _resource_defs);

        std::string get_id_as_str(){
            return std::to_string(id);
        }

        unsigned int get_n_ports() const override {return get_num_external_qubits();}

        unsigned int get_num_external_resource_defs(Resource_kind kind) const;

        unsigned int get_num_external_qubits() const {return info.n_qubits;}

        unsigned int get_num_external_bits() const { return info.n_bits;}

        unsigned int get_num_floats() const {return info.n_floats;}

    private:
        Ptr_coll<Resource_def> resource_defs;
        std::shared_ptr<Resource_def> last_qubit_def;
        Gate_info info;   // may need defaults for empty constructor
};


#endif
