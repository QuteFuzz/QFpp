#ifndef RESOURCE_DEF_H
#define RESOURCE_DEF_H

#include <lex.h>
#include <node.h>
#include <variable.h>
#include <uint.h>
#include <resource.h>

class Resource_def : public Node {

    public:

        /// @brief Dummy definition
        Resource_def() :
            Node("resource_def", QUBIT_DEF),
            scope(Scope::GLOB),
            kind(Resource_kind::QUBIT)
        {}

        Resource_def(const Variable _name, UInt _size, const Scope& _scope, Resource_kind rk, bool is_reg) :
            Node("resource_def", (rk == (Resource_kind::QUBIT) ? QUBIT_DEF : BIT_DEF)),
            name(_name),
            size(_size),
            reg(is_reg),
            scope(_scope),
            kind(rk)
        {}

        Scope get_scope() const { return scope; }

        Resource_kind get_resource_kind() const { return kind; }

        inline std::shared_ptr<Variable> get_name() const override { return std::make_shared<Variable>(name);}

        inline std::shared_ptr<UInt> get_size() const override {
            return std::make_shared<UInt>(size);
        }

        inline bool defines(const Resource& resource) const {
            return get_name()->get_str() == resource.get_name()->get_str();
        }

        inline std::string resolved_name() const override {
            return get_name()->get_str() + " SIZE(" + get_size()->get_str() + ")";
        }

        void reset(){
            used = false;
        }

        bool is_used(){
            return used;
        }

        void set_used(){
            used = true;
        }

        bool is_reg(){
            return reg;
        }

        std::shared_ptr<Resource_def> clone() const {
            auto new_node = std::make_shared<Resource_def>(*this);
            new_node->clear_children();
            return std::make_shared<Resource_def>(*new_node);
        }

    private:
        Variable name;
        UInt size;
        bool used = false;
        bool reg = false;
        Scope scope;
        Resource_kind kind;

};

#endif
