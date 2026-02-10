#ifndef VARIABLE_H
#define VARIABLE_H

#include <node.h>

class Variable : public Node {

    public:
        Variable(const std::string& prefix = "var", bool extend_prefix = false) :
            Node(extend_prefix ? prefix + "_" + std::to_string(node_counter) : prefix)
        {}

    private:

};

#endif
