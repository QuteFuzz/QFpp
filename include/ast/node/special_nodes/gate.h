#ifndef GATE_H
#define GATE_H

#include <clone_mixin.h>
#include <coll.h>
#include <supported_gates.h>

class Resource_def;
enum class Resource_kind;
class Variable;

class Gate : public Cloneable<Gate> {

    public:

        Gate() :
            Cloneable<Gate>("dummy")
        {}

        /// @brief Use for predefined gates
        /// @param str
        /// @param kind
        Gate(const std::string& str, const Token_kind& kind);

        /// @brief Use for sub circuits
        /// @param str
        /// @param kind
        /// @param qubit_defs
        Gate(const std::string& str, const Ptr_coll<Resource_def>& _resource_defs);

        /// @brief Use for unitaries
        /// @param str 
        /// @param kind 
        /// @param n_matrix_qubits 
        Gate(const std::string& str, unsigned int n_matrix_qubits);

        std::string get_id_as_str(){
            return std::to_string(id);
        }

        // std::shared_ptr<Variable> get_var_name() const override;

        unsigned int get_num_external_resources(Resource_kind rk) const;

        unsigned int get_num_external_resource_defs(Resource_kind kind) const;

        Ptr_coll<Resource_def> get_resource_defs() const;

        Token_kind get_gate_source() const;

    private:
        Ptr_coll<Resource_def> resource_defs;
        std::shared_ptr<Resource_def> last_qubit_def;
        Gate_info info;   // may need defaults for empty constructor
};


#endif
