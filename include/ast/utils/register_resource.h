#ifndef REGISTER_RESOURCE_H
#define REGISTER_RESOURCE_H

#include <name.h>
#include <uint.h>

class Register_resource {

    public:

        Register_resource(){}

        Register_resource(const Name& _name, const UInt& _index) :
            name(_name),
            index(_index)
        {}

        void reset(){used = false;}

        void set_used(){used = true;}

        bool is_used(){return used;}

        inline std::shared_ptr<Name> get_name() const {
            return std::make_shared<Name>(name);
        }

        inline std::shared_ptr<UInt> get_index() const {
            return std::make_shared<UInt>(index);
        }

    private:
        Name name;
        UInt index;
        bool used = false;
};

#endif
