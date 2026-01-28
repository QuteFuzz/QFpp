#ifndef RESOURCE_LIST_H
#define RESOURCE_LIST_H

#include <node.h>

class Qubit_list : public Node {

    public:
        Qubit_list(unsigned int num_qubits_in_list):
            Node("qubit_list", QUBIT_LIST)
        {
            add_constraint(QUBIT, num_qubits_in_list);
        }

    private:

};

class Bit_list : public Node {

    public:
        Bit_list(unsigned int num_bits_in_list):
            Node("bit_list", BIT_LIST)
        {
            add_constraint(BIT, num_bits_in_list);
        }

    private:

};

#endif
