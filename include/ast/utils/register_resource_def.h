#ifndef REGISTER_RESOURCE_DEF_H
#define REGISTER_RESOURCE_DEF_H

#include <register_resource.h>
#include <variable.h>
#include <integer.h>
#include <resource.h>

class Register_resource_def {

    public:

        Register_resource_def(){}

        Register_resource_def(const Name& _name, const Integer& _size):
            name(_name),
            size(_size)
        {}

        std::shared_ptr<Name> get_name() const {
            return std::make_shared<Name>(name);
        }

        std::shared_ptr<Integer> get_size() const {
            return std::make_shared<Integer>(size);
        }

        inline void increase_size(){
            size++;
        }

        void reset(){used = false;}

        void set_used(){used = true;}

        bool is_used(){return used;}

        // void make_resources(Ptr_coll<Resource>& output, const Scope& scope) const {

        //     for(size_t i = 0; i < (size_t)size.get_num(); i++){
        //         Register_resource reg_resource(name, Integer(std::to_string(i)));
        //         output.push_back(std::make_shared<Resource>(reg_resource, scope));
        //     }
        // }

    protected:
        Name name;
        Integer size;
        bool used = false;
};

#endif
