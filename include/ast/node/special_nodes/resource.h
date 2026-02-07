#ifndef RESOURCE_H
#define RESOURCE_H

#include <gate.h>
#include <dag.h>
#include <variable.h>
#include <uint.h>

enum class Resource_kind {
    QUBIT,
    BIT
};

#define STR_RESOURCE_KIND(rk) (rk == Resource_kind::QUBIT ? " QUBIT " : " BIT ")

class Resource : public Node {
    public:

        /// @brief Dummy resource
        Resource() :
            Node("dummy", QUBIT),
            resource_kind(Resource_kind::QUBIT)
        {
            add_constraint(SINGULAR_QUBIT, 1);
        }

        Resource(const Variable& _name, const UInt& _index, const Scope& _scope, Resource_kind rk, bool is_reg) :
            Node("register_resource", (rk == (Resource_kind::QUBIT) ? QUBIT : BIT)),
            name(_name),
            index(_index),
            scope(_scope),
            resource_kind(rk)
        {
            if (rk == Resource_kind::QUBIT){
                add_constraint(REGISTER_QUBIT, is_reg);
            } else {
                add_constraint(REGISTER_BIT, is_reg);
            }
        }

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

        void set_used(){
            used = true;
        }

        inline std::shared_ptr<Variable> get_name() const override {
            return std::make_shared<Variable>(name);
        }

        inline std::shared_ptr<UInt> get_index() const override {
            return std::make_shared<UInt>(index);
        }

        inline std::string resolved_name() const override {
            return get_name()->get_str() + "[" + get_index()->get_str() + "]";
        }

        bool operator==(const Resource& other) const {
            bool name_matches = (*get_name() == *other.get_name());
            bool index_matches = (*get_index() == *other.get_index());

            return name_matches && index_matches;
        }

        std::shared_ptr<Resource> clone() const {
            auto new_node = std::make_shared<Resource>(*this);
            new_node->clear_children();
            new_node->incr_id();
            return std::make_shared<Resource>(*new_node);
        }

        inline void print_info() const {
            std::cout << resolved_name() << " " << STR_SCOPE(get_scope()) << STR_RESOURCE_KIND(get_resource_kind()) << " is used: " << used << std::endl;
        }

        // void extend_flow_path(const std::shared_ptr<Qubit_op> qubit_op, unsigned int current_port);

        // void extend_dot_string(std::ostringstream& ss) const;

        // void add_path_to_dag(Dag& dag) const;

        // std::vector<Edge> get_flow_path(){ return flow_path; }

    private:
        Variable name;
        UInt index;
        bool used = false;
        Scope scope = Scope::GLOB;
        Resource_kind resource_kind;

        // std::vector<Edge> flow_path;
        std::string flow_path_colour = random_hex_colour();
        size_t flow_path_length = 0;
};


#endif
