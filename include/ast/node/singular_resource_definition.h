#ifndef SINGULAR_RESOURCE_DEFINITION_H
#define SINGULAR_RESOURCE_DEFINITION_H

#include <node.h>
#include <singular_resource.h>
#include <collection.h>
#include <resource.h>

class Singular_resource_definition : public Node {

    public:

        /// @brief Dummy resource definition
        Singular_resource_definition() : 
            Node()
        {}

        Singular_resource_definition(const Variable& _name) : 
            Node("singular_resource_def", SINGULAR_RESOURCE_DEF),
            name(_name)
        {}

        std::shared_ptr<Variable> get_name() const {
            return std::make_shared<Variable>(name);
        }

    protected:
        Variable name;

};

class Singular_qubit_definition : public Singular_resource_definition {
    
    public:
        Singular_qubit_definition(const Variable& _name) : 
            Singular_resource_definition(_name)
        {}

        Singular_qubit_definition(const Qubit& qubit) :
            Singular_resource_definition(*qubit.get_name())
        {}

        void make_resources(Collection<Qubit>& output, U8& scope) const {
            Singular_qubit singular_qubit(name);
            output.add(Qubit(singular_qubit, scope));
        }

    private:

};

class Singular_bit_definition : public Singular_resource_definition {
    
    public:
        Singular_bit_definition(const Variable& _name) : 
            Singular_resource_definition(_name)
        {}

        Singular_bit_definition(const Bit& bit) :
            Singular_resource_definition(*bit.get_name())
        {}

        void make_resources(Collection<Bit>& output, U8& scope) const {
            Singular_bit singular_bit(name);
            output.add(Bit(singular_bit, scope));
        }

    private:

};

#endif

