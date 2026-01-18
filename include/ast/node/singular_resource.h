#ifndef SINGULAR_RESOURCE_H
#define SINGULAR_RESOURCE_H

#include <node.h>
#include <variable.h>

class Singular_resource : public Node {
    public:
        /// @brief Dummy resource
        Singular_resource() :
            Node("Singular_resource", SINGULAR_RESOURCE)
        {}

        Singular_resource(std::string str, Token_kind kind, const Variable& _name) :
            Node(str, kind),
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

class Singular_qubit : public Singular_resource {

    public:
        Singular_qubit(const Variable& _name):
            Singular_resource("singular_qubit", SINGULAR_QUBIT, _name)
        {}

    private:

};

class Singular_bit : public Singular_resource {

    public:
        Singular_bit(const Variable& _name):
            Singular_resource("singular_bit", SINGULAR_BIT, _name)
        {}

    private:

};


#endif
