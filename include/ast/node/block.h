#ifndef BLOCK_H
#define BLOCK_H

#include <node.h>
#include <resource_definition.h>
#include <resource.h>
#include <collection.h>
#include <params.h>

/*
    Blocks contain external and internal qubits, external and internal bits, which are set targets that must be satisfied
    It is not a guarantee that any block will have both internal and external qubits, so the targets are set such that any block 
    will have external qubits = (MIN_QUBITS, MAX_QUBITS) and internal qubits = (MIN_QUBITS, MAX_QUBITS) separately, 
    instead of external qubits + internal qubits = (MIN_QUBITS, MAX_QUBITS)

    See example below, if using  external qubits + internal qubits = (MIN_QUBITS, MAX_QUBITS)

    =======================================
                BLOCK INFO               
    =======================================
    Owner: main_circuit
    Target num qubits 
    EXTERNAL: 3
    INTERNAL: 1
    Target num bits 
    EXTERNAL: 1
    INTERNAL: 1

    Qubit definitions 
    Qubit defs may not match target if block is built to match DAG
    name: qreg0 size: 1 Scope: internal
    =======================================
    @guppy.comptime
    def main_circuit() -> None:
            qreg0 = array(qubit() for _ in range(1))
            if 0 <= 0  and 0 > 0   or 0 <= 0  and 0 == 0   :
                    if 0 <= 0  and 0 == 0   or 0 >= 0  and 0 <= 0   :
                            project_z(qreg0[0])
                            cy(qreg0[0], 

    the block set a target for internal qubits of 1, and external of 3. But since this is guppy, only internal definitions are created, and therefore this stalls in 
    an infinite loop while picking a random qubit
*/

class Block : public Node {

    public:

        Block() : 
            Node("block", BLOCK),
            owner("kendrick lamar")
        {}

        /// @brief Generating a random block from scratch
        Block(std::string owner_name) :
            Node("block", BLOCK),
            owner(owner_name), 
            target_num_qubits_external(random_int(QuteFuzz::MAX_QUBITS, QuteFuzz::MIN_QUBITS)),
            target_num_qubits_internal(random_int(QuteFuzz::MAX_QUBITS, QuteFuzz::MIN_QUBITS)),
            target_num_bits_external(random_int(QuteFuzz::MAX_BITS, QuteFuzz::MIN_BITS)),
            target_num_bits_internal(random_int(QuteFuzz::MAX_BITS, QuteFuzz::MIN_BITS)) 
        {}

        /// @brief Generating a block with a specific number of external qubits (generating from DAG)
        Block(std::string owner_name, unsigned int num_external_qubits) :
            Node("block", BLOCK),
            owner(owner_name), 
            target_num_qubits_external(num_external_qubits),
            target_num_qubits_internal(random_int(QuteFuzz::MAX_QUBITS, QuteFuzz::MIN_QUBITS)),
            target_num_bits_external(random_int(QuteFuzz::MAX_BITS, QuteFuzz::MIN_BITS)),
            target_num_bits_internal(random_int(QuteFuzz::MAX_BITS, QuteFuzz::MIN_BITS)) 
        {}

        inline bool owned_by(std::string other){return other == owner;}

        inline std::string get_owner(){return owner;}

        void set_can_apply_subroutines(bool flag){
            can_apply_subroutines = flag;
        }

        bool get_can_apply_subroutines() const {
            return can_apply_subroutines;
        }

        /// Cannnot sum internal and external targets because rule to generate using the target may not be in grammar
        inline size_t num_qubits_of(const U8& scope) const {
            return qubits.get_num_of(scope);
        }

        /// Cannnot sum internal and external targets because rule to generate using the target may not be in grammar
        inline size_t num_bits_of(const U8& scope) const {
            return bits.get_num_of(scope);
        }

        inline size_t num_qubit_defs_of(const U8& scope) const {
            return qubit_defs.get_num_of(scope);
        }

        inline size_t num_bit_defs_of(const U8& scope) const {
            return bit_defs.get_num_of(scope);
        }

        void qubit_flag_reset(){
            qubits.reset();
        }

        void bit_flag_reset(){
            bits.reset();
        }

        void qubit_def_pointer_reset(){
            qubit_def_pointer = 0;
        }

        void bit_def_pointer_reset(){
            bit_def_pointer = 0;
        }

        inline std::shared_ptr<Qubit> qubit_at(size_t index){
            if(index < qubits.get_num_of(ALL_SCOPES)){
                return qubits.at(index);
            } else {
                return dummy_qubit;
            }
        }

        inline std::shared_ptr<Bit> bit_at(size_t index){
            if(index < bits.get_num_of(ALL_SCOPES)){
                return bits.at(index);
            } else {
                return dummy_bit;
            }
        }

        inline Collection<Qubit> get_qubits(){
            return qubits;
        }

        inline Collection<Bit> get_bits(){
            return bits;
        }

        inline Collection<Qubit_definition> get_qubit_defs(){
            return qubit_defs;
        }

        inline Collection<Bit_definition> get_bit_defs(){
            return bit_defs;
        }

        std::shared_ptr<Qubit> get_random_qubit(const U8& scope);
        
        std::shared_ptr<Bit> get_random_bit(const U8& scope);

        std::shared_ptr<Qubit_definition> get_next_qubit_def(const U8& scope);

        std::shared_ptr<Qubit_definition> get_next_qubit_def_discard(const U8& scope);

        std::shared_ptr<Bit_definition> get_next_bit_def(const U8& scope);

        unsigned int make_register_resource_definition(unsigned int max_size, U8& scope, Resource_kind rk, unsigned int& total_definitions);

        unsigned int make_singular_resource_definition(U8& scope, Resource_kind rk, unsigned int& total_definitions);

        unsigned int make_resource_definitions(U8& scope, Resource_kind rk, bool discard_defs = false);

        unsigned int make_resource_definitions(const Dag::Dag& dag, const U8& scope, Resource_kind rk, bool discard_defs = false);

        void print_info() const;

    private:
        std::string owner;

        unsigned int target_num_qubits_external = QuteFuzz::MIN_QUBITS;
        unsigned int target_num_qubits_internal = 0;
        unsigned int target_num_bits_external = QuteFuzz::MIN_BITS;
        unsigned int target_num_bits_internal = 0;
        
        bool can_apply_subroutines = true;

        Collection<Qubit> qubits;
        Collection<Qubit_definition> qubit_defs;
        Collection<Qubit_definition> qubit_defs_discard;

        Collection<Bit> bits;
        Collection<Bit_definition> bit_defs;

        unsigned int qubit_def_pointer = 0;
        unsigned int qubit_def_discard_pointer = 0;
        unsigned int bit_def_pointer = 0;

        std::shared_ptr<Qubit> dummy_qubit = std::make_shared<Qubit>();
        std::shared_ptr<Bit> dummy_bit = std::make_shared<Bit>();

        std::shared_ptr<Qubit_definition> dummy_qubit_def = std::make_shared<Qubit_definition>();
        std::shared_ptr<Bit_definition> dummy_bit_def = std::make_shared<Bit_definition>();
};


#endif
