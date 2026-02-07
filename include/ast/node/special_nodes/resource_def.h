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
            scope(Scope::GLOB),
            kind(Resource_kind::QUBIT)
        {}

        Resource_def(const Register_resource_def& def, const Scope& _scope, Resource_kind rk) :
            Node("resource_def", (rk == (Resource_kind::QUBIT) ? QUBIT_DEF : BIT_DEF)),
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

        Resource_kind get_resource_kind() const { return kind; }

        inline std::shared_ptr<Variable> get_name() const override {
            return std::visit([](auto&& val) -> std::shared_ptr<Variable> {
                return val.get_name();
            }, value);
        }

        /// @brief Get size of resource definition if it is a register definition. If not, return 1, or whatever `default_size` is as an `UInt` node
        /// @param default_size
        /// @return
        inline std::shared_ptr<UInt> get_size() const override {
            if (is_register_def()) {
                return std::get<Register_resource_def>(value).get_size();
            } else {
                WARNING("Singular resource def has no size! retuning size 1");
                return std::make_shared<UInt>(1);
            }
        }

        inline bool is_register_def() const {
            return std::holds_alternative<Register_resource_def>(value);
        }

        inline bool defines(const Resource& resource) const {
            return get_name()->get_str() == resource.get_name()->get_str();
        }

        inline std::string resolved_name() const override {
            if(is_register_def()){
                return get_name()->get_str() + " SIZE(" + get_size()->get_str() + ")";
            } else {
                return get_name()->get_str();
            }
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

        std::shared_ptr<Resource_def> clone() const {
            auto new_node = std::make_shared<Resource_def>(*this);
            new_node->clear_children();
            return std::make_shared<Resource_def>(*new_node);
        }

    private:
        std::variant<Register_resource_def, Singular_resource_def> value;
        Scope scope;
        Resource_kind kind;

};

#endif
