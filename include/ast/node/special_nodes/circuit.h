#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <node.h>
#include <resource_definition.h>
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
        Circuit(std::string owner_name, const Control& control, bool _is_subroutine) :
            Node("circuit", CIRCUIT),
            owner(owner_name),
            is_subroutine(_is_subroutine),
            target_num_qubits_external(random_uint(control.get_value("MAX_NUM_QUBITS"), control.get_value("MIN_NUM_QUBITS"))),
            target_num_qubits_internal(random_uint(control.get_value("MAX_NUM_QUBITS"), control.get_value("MIN_NUM_QUBITS"))),
            target_num_bits_external(random_uint(control.get_value("MAX_NUM_BITS"), control.get_value("MIN_NUM_BITS"))),
            target_num_bits_internal(random_uint(control.get_value("MAX_NUM_BITS"), control.get_value("MIN_NUM_BITS")))
        {}

        /// @brief Generating a circuit with a specific number of external qubits (generating from DAG)
        Circuit(std::string owner_name, unsigned int num_external_qubits, const Control& control) :
            Node("circuit", CIRCUIT),
            owner(owner_name),
            is_subroutine(false),
            target_num_qubits_external(num_external_qubits),
            target_num_qubits_internal(random_uint(control.get_value("MAX_NUM_QUBITS"), control.get_value("MIN_NUM_QUBITS"))),
            target_num_bits_external(random_uint(control.get_value("MAX_NUM_BITS"), control.get_value("MIN_NUM_BITS"))),
            target_num_bits_internal(random_uint(control.get_value("MAX_NUM_BITS"), control.get_value("MIN_NUM_BITS")))
        {}

        inline void set_global_targets(const Control& control){
            target_num_qubits_global = random_uint(control.get_value("MAX_NUM_QUBITS"), control.get_value("MIN_NUM_QUBITS"));
            target_num_bits_global = random_uint(control.get_value("MAX_NUM_BITS"), control.get_value("MIN_NUM_BITS"));
        }

        inline unsigned int get_resource_target(Scope& scope, Resource_kind& rk){
            switch(scope){
                case Scope::EXT: return (rk == RK_QUBIT) ? target_num_qubits_external : target_num_bits_external;
                case Scope::INT: return (rk == RK_QUBIT) ? target_num_qubits_internal : target_num_bits_internal;
                case Scope::GLOB: return (rk == RK_QUBIT) ? target_num_qubits_global : target_num_bits_global;
                default: return QuteFuzz::MIN_QUBITS;
            }
        }

        inline bool owned_by(std::string other){return other == owner;}

        inline std::string get_owner(){return owner;}

        inline bool check_if_subroutine(){return is_subroutine;}

        void set_can_apply_subroutines(bool flag){
            can_apply_subroutines = flag;
        }

        bool get_can_apply_subroutines() const {
            return can_apply_subroutines;
        }

        template<typename T>
        inline Ptr_coll<T> get_collection() const {
            if constexpr (std::is_same_v<T, Qubit>) {
				return qubits;
			} else if constexpr (std::is_same_v<T, Qubit_definition>) {
				return qubit_defs;
			} else if constexpr (std::is_same_v<T, Bit>) {
				return bits;
			} else if constexpr (std::is_same_v<T, Bit_definition>) {
				return bit_defs;
			} else {
                static_assert(always_false_v<T>, "Given type cannot be in a ptr collection used in `CIRCUIT`");
            }
        }

        template<typename T>
        inline void reset(){
            for (const auto& elem : get_collection<T>()){
                elem->reset();
            }
        }

        unsigned int make_register_resource_definition(Scope& scope, Resource_kind rk, unsigned int max_size, unsigned int& total_definitions);

        unsigned int make_singular_resource_definition(Scope& scope,  Resource_kind rk, unsigned int& total_definitions);

        unsigned int make_resource_definitions(Scope& scope, Resource_kind rk, Control& control);

        void print_info() const;

    private:
        std::string owner;
        bool is_subroutine = false;

        unsigned int target_num_qubits_external = QuteFuzz::MIN_QUBITS;
        unsigned int target_num_qubits_internal = 0;
        unsigned int target_num_qubits_global = 0;

        unsigned int target_num_bits_external = QuteFuzz::MIN_BITS;
        unsigned int target_num_bits_internal = 0;
        unsigned int target_num_bits_global = 0;

        bool can_apply_subroutines = false;

        Ptr_coll<Qubit> qubits;
        Ptr_coll<Qubit_definition> qubit_defs;
        Ptr_coll<Bit> bits;
        Ptr_coll<Bit_definition> bit_defs;
};


#endif
