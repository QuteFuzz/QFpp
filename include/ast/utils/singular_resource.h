#ifndef SINGULAR_RESOURCE_H
#define SINGULAR_RESOURCE_H

#include <variable.h>

class Singular_resource : public Node {
    public:
        Singular_resource(){}

        Singular_resource(const Variable& _name) :
            name(_name)
        {}

        void reset(){used = false;}

        void set_used(){used = true;}

        bool is_used(){return used;}

        inline std::shared_ptr<Variable> get_name() const {
            return std::make_shared<Variable>(name);
        }

    private:
        Variable name;
        bool used = false;
};

#endif
