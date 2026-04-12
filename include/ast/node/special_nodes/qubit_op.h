#ifndef QUBIT_OP_H
#define QUBIT_OP_H

#include <node.h>
#include <clone_mixin.h>

class Circuit;
class Gate;

class Qubit_op : public Cloneable<Qubit_op> {

    public:
        Qubit_op():
            Cloneable<Qubit_op>("qubit_op", QUBIT_OP)
        {}

        inline void set_gate_node(std::shared_ptr<Node> node){
            gate_node = std::make_optional<std::shared_ptr<Node>>(node);
        }

        bool is_subroutine_op() const;

        void add_gate_if_subroutine(std::vector<std::shared_ptr<Node>>& subroutine_gates);

        std::string resolved_name() const override;

        std::vector<std::string> get_target_qubit_names();

    private:
        std::optional<std::shared_ptr<Node>> gate_node = std::nullopt;

};

#endif
