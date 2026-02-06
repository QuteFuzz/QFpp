#ifndef RESOURCE_H
#define RESOURCE_H

#include <gate.h>
#include <register_resource.h>
#include <singular_resource.h>
#include <dag.h>

enum class Resource_kind {
    QUBIT,
    BIT
};

class Resource : public Node {
    public:

        /// @brief Dummy resource
        Resource() :
            Node("dummy", QUBIT),
            value(Singular_resource())
        {}

        Resource(const Register_resource& resource, const Scope& _scope, Resource_kind rk) :
            Node("register_resource", (rk == (Resource_kind::QUBIT) ? QUBIT : BIT)),
            value(resource),
            scope(_scope),
            resource_kind(rk)
        {
            if (rk == Resource_kind::QUBIT){
                add_constraint(REGISTER_QUBIT, 1);
            } else {
                add_constraint(REGISTER_BIT, 1);
            }
        }

        Resource(const Singular_resource& resource, const Scope& _scope, Resource_kind rk) :
            Node("singular_resource", (rk == (Resource_kind::QUBIT) ? QUBIT : BIT)),
            value(resource),
            scope(_scope),
            resource_kind(rk)
        {
            if (rk == Resource_kind::QUBIT){
                add_constraint(SINGULAR_QUBIT, 1);
            } else {
                add_constraint(SINGULAR_BIT, 1);
            }
        }

        Scope get_scope() const {
            return scope;
        }

        Resource_kind get_kind() const {
            return resource_kind;
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

        inline std::shared_ptr<Name> get_name() const override {
            return std::visit([](auto&& val) -> std::shared_ptr<Name> {
                return val.get_name();
            }, value);
        }

        inline std::shared_ptr<UInt> get_index() const override {
            if(is_register_def()){
                return std::get<Register_resource>(value).get_index();
            } else {
                return std::make_shared<UInt>();
            }
        }

        inline bool is_register_def() const {
            return std::holds_alternative<Register_resource>(value);
        }

        inline std::string resolved_name() const override {
            if(is_register_def()){
                return get_name()->get_content() + "[" + get_index()->get_content() + "]";
            } else {
                return get_name()->get_content();
            }
        }

        bool operator==(const Resource& other) const {
            bool name_matches = (*get_name() == *other.get_name());
            bool index_matches = (*get_index() == *other.get_index());

            if(is_register_def()){
                return name_matches && index_matches;
            } else {
                return name_matches;
            }
        }

        friend std::ostream& operator<<(std::ostream& stream, const Resource& r){
            stream << r.resolved_name() << STR_SCOPE(r.scope) << (r.resource_kind == Resource_kind::QUBIT ? " QUBIT " : " BIT ");
            return stream;
        }

        // void extend_flow_path(const std::shared_ptr<Qubit_op> qubit_op, unsigned int current_port);

        // void extend_dot_string(std::ostringstream& ss) const;

        // void add_path_to_dag(Dag& dag) const;

        // std::vector<Edge> get_flow_path(){
        //     return flow_path;
        // }

    private:
        std::variant<Register_resource, Singular_resource> value;
        Scope scope;
        Resource_kind resource_kind;

        // std::vector<Edge> flow_path;
        std::string flow_path_colour = random_hex_colour();
        size_t flow_path_length = 0;
};


#endif
