#ifndef QUBIT_NAME_H
#define QUBIT_NAME_H

#include <node.h>

class Variable : public Node {

    public:
        using Node::Node;

        Variable() :
            Node("var_" + std::to_string(node_counter))
        {}

    private:

};

#endif
