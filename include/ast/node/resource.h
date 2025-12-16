#ifndef RESOURCE_H
#define RESOURCE_H

#include <gate.h>
#include <register_resource.h>
#include <singular_resource.h>
#include <dag.h>

namespace Resource {

    enum Classification {
        QUBIT,
        BIT
    };

    class Resource : public Node {
        public:

            /// @brief Dummy resource
            Resource() :
                Node("dummy", SINGULAR_RESOURCE),
                value(Singular_resource())
            {}

            Resource(const std::string& str, const Token_kind& kind, const Register_resource& resource, const U8& _scope) :
                Node(str, kind),
                value(resource),
                scope(_scope)
            {}

            Resource(const std::string& str, const Token_kind& kind, const Singular_resource& resource, const U8& _scope) :
                Node(str, kind),
                value(resource),
                scope(_scope)
            {}

            U8 get_scope() const {
                return scope;
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

            inline std::shared_ptr<Variable> get_name() const {
                return std::visit([](auto&& val) -> std::shared_ptr<Variable> {
                    return val.get_name();
                }, value);
            }

            inline bool is_register_def() const {
                return std::holds_alternative<Register_resource>(value);
            }

            inline std::shared_ptr<Integer> get_index() const {
                if(is_register_def()){
                    return std::get<Register_resource>(value).get_index();
                }

                ERROR("Singular resource do not have indices!");

                return std::make_shared<Integer>();
            }

            std::string resolved_name() const override;

            
            
        private:
            std::variant<Register_resource, Singular_resource> value;
            U8 scope;

    };

    class Qubit : public Resource {

        public:
            Qubit() : Resource() {}

            Qubit(const Register_qubit& qubit, const U8& scope) :
                Resource("qubit", QUBIT, qubit, scope)
            {
                add_constraint(REGISTER_QUBIT, 1);
            }

            Qubit(const Singular_qubit& qubit, const U8& scope) :
                Resource("qubit", QUBIT, qubit, scope)
            {
                add_constraint(SINGULAR_QUBIT, 1);
            }

            void extend_flow_path(const std::shared_ptr<Qubit_op> qubit_op, unsigned int current_port);

            void extend_dot_string(std::ostringstream& ss) const;

            void add_path_to_dag(Dag::Dag& dag) const;

            std::vector<Dag::Edge> get_flow_path(){
                return flow_path;
            }


        private:
            std::vector<Dag::Edge> flow_path;
            std::string flow_path_colour = random_hex_colour();
            size_t flow_path_length = 0;

    };

    class Bit : public Resource {

        public:
            Bit() : Resource() {}

            Bit(const Register_bit& bit, const U8& scope) :
                Resource("bit", BIT, bit, scope)
            {
                add_constraint(REGISTER_BIT, 1);
            }

            Bit(const Singular_bit& bit, const U8& scope) :
                Resource("bit", BIT, bit, scope)
            {
                add_constraint(SINGULAR_BIT, 1);
            }

        private:

    };


}




#endif
