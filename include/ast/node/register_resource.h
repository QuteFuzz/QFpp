#ifndef REGISTER_RESOURCE_H
#define REGISTER_RESOURCE_H

#include <node.h>
#include <variable.h>
#include <integer.h>

class Register_resource : public Node {

    public:

        /// @brief Dummy resource
        Register_resource() :
            Node("register_resource", REGISTER_RESOURCE)
        {}

        Register_resource(const std::string& str, const Token_kind& kind, const Variable& _name, const Integer& _index) : 
            Node(str, kind),
            name(_name),
            index(_index)
        {}

        void reset(){used = false;}

        void set_used(){used = true;}
        
        bool is_used(){return used;}

        inline std::shared_ptr<Variable> get_name() const {
            return std::make_shared<Variable>(name);
        }
        
        inline std::shared_ptr<Integer> get_index() const {
            return std::make_shared<Integer>(index);
        }

    private:
        Variable name;
        Integer index;
        bool used = false;
};

class Register_qubit : public Register_resource {

    public:
        Register_qubit(const Variable& _name, const Integer& _index) : 
            Register_resource("register_qubit", REGISTER_QUBIT, _name, _index)
        {}

    private:

};

class Register_bit : public Register_resource {

    public:
        Register_bit(const Variable& _name, const Integer& _index) : 
            Register_resource("register_bit", REGISTER_BIT, _name, _index)
        {}

    private:

};

#endif
