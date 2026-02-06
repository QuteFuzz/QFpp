#ifndef CONJUNCTION_H
#define CONJUNCTION_H

#include <node.h>


/*
    Used to override node print to add spaces after each child
*/

class Conjunction : public Node {

    public:
        Conjunction() :
            Node("conjuction", CONJUNCTION)
        {
            add_constraint(INVERSION, 2);
        }

        void print_program(std::ostream& stream) const override {
            for(const std::shared_ptr<Node>& child : children){
                child->print_program(stream);
                stream << " ";
            }
        }

    private:

};


#endif
