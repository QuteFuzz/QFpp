#ifndef RESOURCE_DEFINITION_H
#define RESOURCE_DEFINITION_H

#include <register_resource_definition.h>
#include <singular_resource_definition.h>
#include <lex.h>

class Resource_definition : public Node {

    public:

        /// @brief Dummy definition
        Resource_definition() :
            Node("resource_def", RESOURCE_DEF),
            value(Register_resource_definition())
        {}

        Resource_definition(const Register_resource_definition& def, const Scope& _scope) :
            Node("register_resource_def", REGISTER_RESOURCE_DEF),
            value(def),
            scope(_scope)
        {}

        Resource_definition(const Singular_resource_definition& def, const Scope& _scope) :
            Node("singular_resource_def", SINGULAR_RESOURCE_DEF),
            value(def),
            scope(_scope)
        {}

        Scope get_scope() const { return scope; }

        inline std::shared_ptr<Variable> get_name() const {
            return std::visit([](auto&& val) -> std::shared_ptr<Variable> {
                return val.get_name();
            }, value);
        }

        /// @brief Get size of resource definition if it is a register definition. If not, return 1, or whatever `default_size` is as an `Integer` node
        /// @param default_size
        /// @return
        inline std::shared_ptr<Integer> get_size(unsigned int default_size = 1) const {
            if(is_register_def()){
                return std::get<Register_resource_definition>(value).get_size();
            }

            WARNING("Singular resource definitions do not have sizes! Using default size = " + default_size);

            return std::make_shared<Integer>(default_size);
        }

        inline bool is_register_def() const {
            return std::holds_alternative<Register_resource_definition>(value);
        }

        inline bool defines(const Resource& resource) const {
            return get_name()->get_content() == resource.get_name()->get_content();
        }

        inline void increase_size(){
            if(is_register_def()) std::get<Register_resource_definition>(value).increase_size();
        }

        void reset(){
            std::visit([](auto&& val){
                val.reset();
            }, value);
        }

        bool is_used(){
            return std::visit([](auto&& val) -> bool {
                return val.is_used();
            }, value);
        }

        void set_used(){
            std::visit([](auto&& val){
                val.set_used();
            }, value);
        }

    private:
        std::variant<Register_resource_definition, Singular_resource_definition> value;
        Scope scope;

};

class Qubit_definition : public Resource_definition {

    public:
        Qubit_definition() : Resource_definition() {}

        Qubit_definition(const Register_resource_definition& def, const Scope& scope, bool def_discard = false):
            Resource_definition(
                def,
                scope
            )
        {
            if (def_discard) {
                add_constraint(REGISTER_QUBIT_DEF_DISCARD, 1);
            } else {
                add_constraint(REGISTER_QUBIT_DEF, 1);
            }
        }

        Qubit_definition(const Singular_resource_definition& def, const Scope& scope, bool def_discard = false):
            Resource_definition(
                def,
                scope
            )
        {
            if (def_discard) {
                add_constraint(SINGULAR_QUBIT_DEF_DISCARD, 1);
            } else {
                add_constraint(SINGULAR_QUBIT_DEF, 1);
            }
        }

    private:

};

class Bit_definition : public Resource_definition {

    public:
        Bit_definition() : Resource_definition() {}

        Bit_definition(const Register_resource_definition& def, const Scope& scope):
            Resource_definition(
                def,
                scope
            )
        {
            add_constraint(REGISTER_BIT_DEF, 1);
        }

        Bit_definition(const Singular_resource_definition& def, const Scope& scope):
            Resource_definition(
                def,
                scope
            )
        {
            add_constraint(SINGULAR_BIT_DEF, 1);
        }

    private:

};

#endif
