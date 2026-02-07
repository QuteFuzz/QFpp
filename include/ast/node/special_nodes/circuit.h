#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <node.h>
#include <resource_def.h>
#include <resource.h>
#include <params.h>
#include <run_utils.h>

/*
    Circuits contain external and internal qubits, external and internal bits, which are set targets that must be satisfied
    It is not a guarantee that any circuit will have both internal and external qubits, so the targets are set such that any circuit
    will have external qubits = (MIN_QUBITS, MAX_QUBITS) and internal qubits = (MIN_QUBITS, MAX_QUBITS) separately,
    instead of external qubits + internal qubits = (MIN_QUBITS, MAX_QUBITS)

    See example below, if using  external qubits + internal qubits = (MIN_QUBITS, MAX_QUBITS)

    =======================================
                CIRCUIT INFO
    =======================================
    Owner: main_circuit
    Target num qubits
    EXTERNAL: 3
    INTERNAL: 1
    Target num bits
    EXTERNAL: 1
    INTERNAL: 1

    Qubit definitions
    Qubit defs may not match target if circuit is built to match DAG
    name: qreg0 size: 1 Scope: internal
    =======================================
    @guppy.comptime
    def main_circuit() -> None:
            qreg0 = array(qubit() for _ in range(1))
            if 0 <= 0  and 0 > 0   or 0 <= 0  and 0 == 0   :
                    if 0 <= 0  and 0 == 0   or 0 >= 0  and 0 <= 0   :
                            project_z(qreg0[0])
                            cy(qreg0[0],

    the circuit set a target for internal qubits of 1, and external of 3. But since this is guppy, only internal definitions are created, and therefore this stalls in
    an infinite loop while picking a random qubit
*/

class Circuit : public Node {

    public:

        Circuit() :
            Node("circuit", CIRCUIT),
            owner("dummy_circuit")
        {}

        /// @brief Generating a random circuit from scratch
        Circuit(std::string owner_name, bool _is_subroutine) :
            Node("circuit", CIRCUIT),
            owner(owner_name),
            is_subroutine(_is_subroutine)
        {}

        inline bool owned_by(std::string other){return other == owner;}

        inline std::string get_owner(){return owner;}

        inline bool check_if_subroutine(){return is_subroutine;}

        void set_can_apply_subroutines(bool flag){
            can_apply_subroutines = flag;
        }

        bool get_can_apply_subroutines() const {
            return can_apply_subroutines;
        }

        template <typename T> 
        inline Ptr_coll<T> get_coll() const {
            if constexpr (std::is_same_v<T, Resource>) {
                return resources;

            } else if constexpr (std::is_same_v<T, Resource_def>) {
                return resource_defs;

            } else {
                throw std::runtime_error("Unknown collection type in circuit");
            }
        }

        template <typename T>
        inline Ptr_coll<T> get_coll(Resource_kind rk) const {
            auto pred = [&rk](const auto& elem){ return elem->get_resource_kind() == rk; };
            return filter<T>(get_coll<T>(), pred);
        }

        void store_resource_def(std::shared_ptr<Resource_def> def){
            auto name = def->get_name();
            auto scope = def->get_scope();
            auto rk = def->get_resource_kind();

            if(def->is_register_def()) {
                auto size = def->get_size();

                for(size_t i = 0; i < (size_t)size->get_num(); i++){
                    Register_resource reg_resource(*name, UInt(std::to_string(i)));
                    resources.push_back(std::make_shared<Resource>(reg_resource, scope, rk));
                }
            } else {                
                Singular_resource sing_resource(*name);
                resources.push_back(std::make_shared<Resource>(sing_resource, scope, rk));
            }

            resource_defs.push_back(def);
        }
    
        inline void reset(){
            for (const auto& elem : resources){
                elem->reset();
            }
        }

        void print_info() const;

    private:
        std::string owner;
        bool is_subroutine = false;
        bool can_apply_subroutines = true;
        Ptr_coll<Resource> resources;
        Ptr_coll<Resource_def> resource_defs;
};


#endif
