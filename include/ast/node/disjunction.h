#ifndef DISJUNCTION_H
#define DISJUNCTION_H

#include <node.h>

/*
    Used to override node print to add spaces after each child
*/

class Disjunction : public Node {

    public:
        Disjunction() :
            Node("disjunction", DISJUNCTION)
        {
            add_constraint(CONJUNCTION, 2);
        }

        void print(std::ostream& stream) const override {
            for(const std::shared_ptr<Node>& child : children){
                stream << *child << " ";
            }
        }

    private:

};


#endif
