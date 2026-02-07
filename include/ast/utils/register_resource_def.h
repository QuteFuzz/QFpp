#ifndef REGISTER_RESOURCE_DEF_H
#define REGISTER_RESOURCE_DEF_H

#include <register_resource.h>
#include <variable.h>
#include <uint.h>
#include <resource.h>

class Register_resource_def {

    public:

        Register_resource_def(){}

        Register_resource_def(const Variable& _name, const UInt& _size):
            name(_name),
            size(_size)
        {}

        std::shared_ptr<Variable> get_name() const {
            return std::make_shared<Variable>(name);
        }

        std::shared_ptr<UInt> get_size() const {
            return std::make_shared<UInt>(size);
        }

        inline void increase_size(){
            size++;
        }

        void reset(){used = false;}

        void set_used(){used = true;}

        bool is_used(){return used;}

    protected:
        Variable name;
        UInt size;
        bool used = false;
};

#endif
