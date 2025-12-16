#ifndef REGISTER_RESOURCE_DEFINITION_H
#define REGISTER_RESOURCE_DEFINITION_H

#include <node.h>
#include <register_resource.h>
#include <variable.h>
#include <integer.h>
#include <collection.h>
#include <resource.h>

class Register_resource_definition : public Node {

    public:

        /// @brief Dummy resource definition
        Register_resource_definition() : 
            Node("")
        {}

        Register_resource_definition(const Variable& _name, const Integer& _size): 
            Node("register_resource_def", REGISTER_RESOURCE_DEF),
            name(_name),
            size(_size)
        {}

        std::shared_ptr<Variable> get_name() const {
            return std::make_shared<Variable>(name);
        }

        std::shared_ptr<Integer> get_size() const {
            return std::make_shared<Integer>(size);
        }

        inline void increase_size(){
            size++;
        }

    protected:
        Variable name;
        Integer size;
};

class Register_qubit_definition : public Register_resource_definition {

    public:
        Register_qubit_definition(const Variable& _name, const Integer& _size):
            Register_resource_definition(
                _name,
                _size
            )
        {}

        Register_qubit_definition(const Resource::Qubit& qubit) :
            Register_resource_definition(
                *qubit.get_name(),
                Integer(1)
            )
        {}

        void make_resources(Collection<Resource::Qubit>& output, const U8& scope) const {

            for(size_t i = 0; i < (size_t)size.get_num(); i++){
                Register_qubit reg_qubit(name, Integer(std::to_string(i)));

                output.add(Resource::Qubit(reg_qubit, scope));
            }
        }

    private:

};

class Register_bit_definition : public Register_resource_definition {

    public:
        Register_bit_definition(const Variable& _name, const Integer& _size):
            Register_resource_definition(
                _name,
                _size
            )
        {}

        Register_bit_definition(const Resource::Bit& bit) :
            Register_resource_definition(
                *bit.get_name(),
                Integer(1)
            )
        {}

        void make_resources(Collection<Resource::Bit>& output, const U8& scope) const {
            for(size_t i = 0; i < (size_t)size.get_num(); i++){
                Register_bit reg_bit(name, Integer(std::to_string(i)));

                output.add(Resource::Bit(reg_bit, scope));
            }
        }

    private:

};

#endif

