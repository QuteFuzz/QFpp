#ifndef SINGULAR_RESOURCE_H
#define SINGULAR_RESOURCE_H

#include <name.h>

class Singular_resource : public Node {
    public:
        Singular_resource(){}

        Singular_resource(const Name& _name) :
            name(_name)
        {}

        void reset(){used = false;}

        void set_used(){used = true;}

        bool is_used(){return used;}

        inline std::shared_ptr<Name> get_name() const {
            return std::make_shared<Name>(name);
        }

    private:
        Name name;
        bool used = false;
};

#endif
