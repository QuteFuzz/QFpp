#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <node.h>

/*
    Used to override node print to add a space before each expression
*/

class Expression : public Node {

    public:

        Expression():
            Node("expression", EXPRESSION)
        {}

        void print_program(std::ostream& stream) const override {
            stream << " ";

            for(const std::shared_ptr<Node>& child : children){
                child->print_program(stream);
            }
        }

    private:

};

#endif
