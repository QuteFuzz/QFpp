#ifndef VARIABLE_H
#define VARIABLE_H

#include <node.h>

class Variable : public Node {

    public:
        Variable(const std::string& prefix = "var", bool count = false) :
            Node(count ? prefix + "_" + std::to_string(node_counter) : prefix)
        {}

    private:

};

#endif
