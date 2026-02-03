#ifndef REGISTER_RESOURCE_H
#define REGISTER_RESOURCE_H

#include <name.h>
#include <integer.h>

class Register_resource {

    public:

        Register_resource(){}

        Register_resource(const Name& _name, const Integer& _index) :
            name(_name),
            index(_index)
        {}

        void reset(){used = false;}

        void set_used(){used = true;}

        bool is_used(){return used;}

        inline std::shared_ptr<Name> get_name() const {
            return std::make_shared<Name>(name);
        }

        inline std::shared_ptr<Integer> get_index() const {
            return std::make_shared<Integer>(index);
        }

    private:
        Name name;
        Integer index;
        bool used = false;
};

#endif
