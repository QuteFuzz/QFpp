#ifndef SINGULAR_RESOURCE_DEF_H
#define SINGULAR_RESOURCE_DEF_H

#include <singular_resource.h>
#include <resource.h>

class Singular_resource_def {

    public:

        Singular_resource_def(){}

        Singular_resource_def(const Variable& _name) :
            name(_name)
        {}

        std::shared_ptr<Variable> get_name() const {
            return std::make_shared<Variable>(name);
        }

        void reset(){used = false;}

        void set_used(){used = true;}

        bool is_used(){return used;}

        // void make_resources(Ptr_coll<Resource>& output, Scope& scope) const {
        //     Singular_resource singular_resource(name);
        //     output.push_back(std::make_shared<Resource>(singular_resource, scope));
        // }

    protected:
        Variable name;
        bool used = false;

};


#endif
