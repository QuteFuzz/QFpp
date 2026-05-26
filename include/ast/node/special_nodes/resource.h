#ifndef RESOURCE_H
#define RESOURCE_H

#include <gate.h>
#include <variable.h>
#include <uint.h>
#include <resource_kind.h>

class Resource : public Cloneable<Resource> {
    public:

        /// @brief Dummy resource
        Resource() :
            Cloneable<Resource>("dummy", QUBIT),
            resource_kind(Resource_kind::QUBIT)
        {
            add_branch_constraint(SINGULAR_QUBIT, 1);
        }

        Resource(const Variable& _name, const UInt& _index, const Scope& _scope, Resource_kind rk, bool is_reg);

        Scope get_scope() const {
            return scope;
        }

        Resource_kind get_resource_kind() const {
            return resource_kind;
        }

        void reset(){
            used = false;
        }

        bool is_used(){
            return used;
        }

        bool is_used_at_least_once(){
            return used_at_least_once;
        }

        void set_used(){
            used = true;
            used_at_least_once = true;
        }

        bool from_reg(){
            return _from_reg;
        }

        inline std::shared_ptr<Variable> get_var_name() const override {
            return std::make_shared<Variable>(name);
        }

        inline std::shared_ptr<UInt> get_index() const override {
            return std::make_shared<UInt>(index);
        }

        inline std::string resolved_name() const override {
            return name.get_str() + "[" + index.get_str() + "]";
        }

        bool operator==(const Resource& other) const {
            bool name_matches = (*get_var_name() == *other.get_var_name());
            bool index_matches = (*get_index() == *other.get_index());

            return name_matches && index_matches;
        }

        inline void print_info() const {
            std::cout << resolved_name() << " " << STR_SCOPE(get_scope()) << STR_RESOURCE_KIND(get_resource_kind()) << " is used: " << used << std::endl;
        }

    private:
        Variable name;
        UInt index;
        Scope scope = Scope::GLOB;
        Resource_kind resource_kind;
        bool _from_reg;
        bool used = false;
        bool used_at_least_once = false;

};


#endif
