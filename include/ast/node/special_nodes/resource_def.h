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

        inline std::shared_ptr<Variable> get_name() const override {
            return std::visit([](auto&& val) -> std::shared_ptr<Variable> {
                return val.get_name();
            }, value);
        }

        /// @brief Get size of resource definition if it is a register definition. If not, return 1, or whatever `default_size` is as an `UInt` node
        /// @param default_size
        /// @return
        inline std::shared_ptr<UInt> get_size(unsigned int default_size = 1) const override {
            if(is_register_def()){
                return std::get<Register_resource_def>(value).get_size();
            }

            WARNING("Singular resource definitions do not have sizes! Using default size = " + default_size);

            return std::make_shared<UInt>(default_size);
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

        inline std::string resolved_name() const override {
            if(is_register_def()){
                return get_name()->get_content() + "SIZE(" + get_size()->get_content() + ")";
            } else {
                return get_name()->get_content();
            }
        }

        friend std::ostream& operator<<(std::ostream& stream, const Resource_def& rd){
            stream << rd.resolved_name() << " " << STR_SCOPE(rd.scope) << (rd.kind == Resource_kind::QUBIT ? " QUBIT " : " BIT ");
            return stream;
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

#endif
