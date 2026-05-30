#ifndef RESOURCE_DEF_H
#define RESOURCE_DEF_H

#include <lex.h>
#include <node.h>
#include <resource.h>

enum class Resource_kind;
class Resource_def : public Cloneable<Resource_def> {

    public:

        /// @brief Dummy definition
        Resource_def() :
            Cloneable<Resource_def>("resource_def", QUBIT_DEF),
            scope(Scope::GLOB),
            kind(Resource_kind::QUBIT)
        {}

        Resource_def(const Scope& _scope, Resource_kind rk, bool is_reg, unsigned int reg_size);

        Scope get_scope() const { return scope; }

        Resource_kind get_resource_kind() const { return kind; }

        inline std::string get_var_name() const { return name;}

        inline unsigned int get_size() const { return size; }

        inline bool defines(const Resource& resource) const {
            return name == resource.get_var_name();
        }

        inline std::string resolved_name() const override {
            return name + " GET_SIZE(" + std::to_string(size) + ")";
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

        inline void print_info() const {
            std::cout << resolved_name() << " " << STR_SCOPE(get_scope()) << STR_RESOURCE_KIND(get_resource_kind()) << " is used: " << used << " is reg: " << reg << std::endl;
        }

    private:
        std::string name;
        unsigned int size;
        bool used = false;
        bool reg = false;
        Scope scope = Scope::GLOB;
        Resource_kind kind;

};

#endif
