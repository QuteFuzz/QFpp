#ifndef NAME_H
#define NAME_H

#include <variable.h>

class Name : public Node {

    public:
        Name() : 
            Node("NAME", NAME)
        {
            add_child(std::make_shared<Variable>());
        }

    private:

};

#endif
