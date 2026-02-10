#ifndef NODE_H
#define NODE_H

#include <utils.h>
#include <lex.h>
#include <node_constraints.h>
#include <rule.h>

enum Node_build_state {
    NB_DONE,
    NB_BUILD,
};

enum Node_kind {
    TERMINAL,
    NON_TERMINAL,
};

class UInt;
class Variable;
class Branch;

/// @brief A node is a term with pointers to other nodes
class Node : public std::enable_shared_from_this<Node> {

    public:
        static int node_counter;

        Node(){}

        Node(std::string _str, Token_kind _kind = SYNTAX):
            str(_str),
            kind(_kind)
        {
            id = node_counter++;
        }

        Node(const Node& other) = default;

        virtual ~Node() = default;

        inline void add_child(const std::shared_ptr<Node> child){
            children.push_back(child);
        }

        inline void transition_to_done(){
            state = NB_DONE;
        }

        Node_build_state build_state(){
            return state;
        }

        int get_id() const {
            return id;
        }

        void incr_id(){
            id++;
        }

        std::string get_str() const;

        Token_kind get_node_kind() const {
            return kind;
        }

        virtual std::string resolved_name() const {
            return get_str();
        }

        bool visited(std::vector<std::shared_ptr<Node>*>& visited_slots, std::shared_ptr<Node>* slot, bool track_visited);

        std::shared_ptr<Node>* find_slot(Token_kind node_kind, std::vector<std::shared_ptr<Node>*>& visited_slots, bool track_visited = true);

        std::shared_ptr<Node> find(Token_kind node_kind);

        inline int count_nodes() const {
            int res = 1;

            for(auto child : children){
                res += child->count_nodes();
            }

            return res;
        }

        inline int count_nodes(Token_kind _kind) const {
            int res = (kind == _kind);

            for(auto child : children){
                res += child->count_nodes();
            }

            return res;
        }

        /// Used to print the program
        virtual void print_program(std::ostream& stream, unsigned int indent_level = 0) const {
            if(kind == SYNTAX){
                stream << str;
            } else {
                for(const std::shared_ptr<Node>& child : children){
                    child->print_program(stream, indent_level);
                }
            }
        }

        void print_ast(std::string indent) const;

        void extend_dot_string(std::ostringstream& ss) const;

        std::vector<std::shared_ptr<Node>> get_children() const {
            return children;
        }

        inline void clear_children(){
            children.clear();
        }

        inline std::shared_ptr<Node> child_at(size_t index) const {
            if(index < size()){
                return children.at(index);
            } else {
                return nullptr;
            }
        }

        inline void insert_child(size_t index, Node& child) {
            if(index < size()){
                children.insert(children.begin() + index, std::make_shared<Node>(child));
            }
        }

        size_t size() const {
            return children.size();
        }

        bool operator==(const Token_kind& other_kind) const {
            return kind == other_kind;
        }

        bool operator==(const Node& other) const {
            return get_str() == other.get_str();
        }

        bool branch_satisfies_constraints(const Branch& branch){
            return !constraints.has_value() || constraints.value().passed(branch);
        }

        void add_constraint(const Token_kind& rule_kind, unsigned int n_occurances){
            if(constraints.has_value()){
                constraints.value().add(rule_kind, n_occurances);
            } else {
                constraints = std::make_optional<Node_constraints>(rule_kind, n_occurances);
            }
        }

        int get_next_child_target();

        void make_partition(int target, int n_children);

        void make_control_flow_partition(int target, int n_children);

        inline bool has_constraints(){
            return constraints.has_value();
        }

        inline void print_constraints(std::ostream& stream){
            if(constraints.has_value()){
                stream << constraints.value() << std::endl;
            } else {
                stream << GREEN("NO CONSTRAINTS") << std::endl;
            }
        }

        inline void remove_constraints(){
            constraints = std::nullopt;
        }

        virtual unsigned int get_n_ports() const {return 1;}

        virtual std::shared_ptr<Variable> get_name() const;

        virtual std::shared_ptr<UInt> get_size() const;

        virtual std::shared_ptr<UInt> get_index() const;

    protected:
        int id = 0;
        std::string str;
        Token_kind kind;

        std::vector<std::shared_ptr<Node>> children;
        Node_build_state state = NB_BUILD;

        std::vector<int> child_partition;
        unsigned int partition_counter = 0;

        bool indent;

    private:
        std::optional<Node_constraints> constraints;
};

#endif
