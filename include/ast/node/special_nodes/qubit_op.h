#ifndef QUBIT_OP_H
#define QUBIT_OP_H

#include <node.h>
#include <clone_mixin.h>

class Circuit;
class Gate;

class Qubit_op : public Cloneable<Qubit_op> {

    public:
        Qubit_op(std::string _caller):
            Cloneable<Qubit_op>("qubit_op", QUBIT_OP),
            caller(_caller)
        {}

        inline void set_gate_node(std::shared_ptr<Gate> _gate_node){
            gate_node = _gate_node;
        }

        std::shared_ptr<Gate> get_gate_node() const {
            return gate_node;
        }

        std::string get_caller_name() const {
            return caller;
        }

        bool is_subroutine_op() const;

        void add_gate_if_subroutine(std::vector<std::shared_ptr<Node>>& subroutine_gates);

        std::string resolved_name() const override;

        std::vector<std::string> get_target_qubit_names();

    private:
        std::string caller;
        std::shared_ptr<Gate> gate_node = nullptr;

};

#endif
