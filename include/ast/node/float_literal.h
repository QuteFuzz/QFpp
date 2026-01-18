#ifndef FLOAT_LITERAL_H
#define FLOAT_LITERAL_H

#include <node.h>

class Float_literal : public Node {

    public:
        using Node::Node;

        Float_literal() :
            Node(std::to_string(random_float(10)))
        {}

        Float_literal(float n) :
            Node(std::to_string(n)),
            num(n)
        {}

        float get_num() const {return num;}

    private:
        float num;

};


#endif
