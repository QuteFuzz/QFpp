#ifndef RESOURCE_DEF_H
#define RESOURCE_DEF_H

#include <register_resource_def.h>
#include <singular_resource_def.h>
#include <lex.h>

class Resource_def : public Node {

    public:

        /// @brief Dummy definition
        Resource_def() :
            Node("resource_def", RESOURCE_DEF),
            value(Register_resource_def())
        {}

        Resource_def(const Register_resource_def& def, const Scope& _scope, Resource_kind rk) :
            Node("register_resource_def", (rk == (Resource_kind::QUBIT) ? QUBIT_DEF : BIT_DEF)),
            value(def),
            scope(_scope),
            kind(rk)
        {}

        Resource_def(const Singular_resource_def& def, const Scope& _scope, Resource_kind rk) :
            Node("singular_resource_def", (rk == (Resource_kind::QUBIT) ? QUBIT_DEF : BIT_DEF)),
            value(def),
            scope(_scope),
            kind(rk)
        {}

        Scope get_scope() const { return scope; }

        Resource_kind get_kind() const { return kind; }

        inline std::shared_ptr<Name> get_name() const override {
            return std::visit([](auto&& val) -> std::shared_ptr<Name> {
                return val.get_name();
            }, value);
        }

        /// @brief Get size of resource definition if it is a register definition. If not, return 1, or whatever `default_size` is as an `Integer` node
        /// @param default_size
        /// @return
        inline std::shared_ptr<Integer> get_size(unsigned int default_size = 1) const override {
            if(is_register_def()){
                return std::get<Register_resource_def>(value).get_size();
            }

            WARNING("Singular resource definitions do not have sizes! Using default size = " + default_size);

            return std::make_shared<Integer>(default_size);
        }

        inline bool is_register_def() const {
            return std::holds_alternative<Register_resource_def>(value);
        }

        inline bool defines(const Resource& resource) const {
            return get_name()->get_content() == resource.get_name()->get_content();
        }

        inline void increase_size(){
            if(is_register_def()) std::get<Register_resource_def>(value).increase_size();
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
        std::variant<Register_resource_def, Singular_resource_def> value;
        Scope scope;
        Resource_kind kind;

};

// class Qubit_definition : public Resource_def {

//     public:
//         Qubit_definition() : Resource_def() {}

//         Qubit_definition(const Register_resource_definition& def, const Scope& scope, bool def_discard = false):
//             Resource_def(
//                 def,
//                 scope
//             )
//         {
//             if (def_discard) {
//                 add_constraint(REGISTER_QUBIT_DEF_DISCARD, 1);
//             } else {
//                 add_constraint(REGISTER_QUBIT_DEF, 1);
//             }
//         }

//         Qubit_definition(const Singular_resource_definition& def, const Scope& scope, bool def_discard = false):
//             Resource_def(
//                 def,
//                 scope
//             )
//         {
//             if (def_discard) {
//                 add_constraint(SINGULAR_QUBIT_DEF_DISCARD, 1);
//             } else {
//                 add_constraint(SINGULAR_QUBIT_DEF, 1);
//             }
//         }

//     private:

// };

// class Bit_definition : public Resource_def {

//     public:
//         Bit_definition() : Resource_def() {}

//         Bit_definition(const Register_resource_definition& def, const Scope& scope):
//             Resource_def(
//                 def,
//                 scope
//             )
//         {
//             add_constraint(REGISTER_BIT_DEF, 1);
//         }

//         Bit_definition(const Singular_resource_definition& def, const Scope& scope):
//             Resource_def(
//                 def,
//                 scope
//             )
//         {
//             add_constraint(SINGULAR_BIT_DEF, 1);
//         }

//     private:

// };

#endif
