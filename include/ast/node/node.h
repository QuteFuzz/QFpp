#ifndef NODE_H
#define NODE_H

#include <utils.h>
#include <lex.h>
#include <branch_constraint.h>
#include <rule.h>

enum Node_build_state {
    NB_DONE,
    NB_BUILD,
};

enum Node_kind {
    TERMINAL,
    NON_TERMINAL,
};

enum Clone_type {
    SHALLOW,
    DEEP,
};

enum class Print_mode {
    DEFAULT,
    INDENT_LEVEL,
    CHILD_INDENT,
    SELF_INDENT,
};

class UInt;
class Variable;
class Branch;

class Node;

using Slot_type = std::shared_ptr<Node>*;


/// @brief A node is a term with pointers to other nodes
class Node : public std::enable_shared_from_this<Node> {

    public:
        static int node_counter;
        Print_mode print_mode = Print_mode::DEFAULT;

        Node(){}

        Node(std::string _str, Token_kind _kind = SYNTAX):
            str(_str),
            kind(_kind)
        {
            id = node_counter++;
        }

        Node(const Node& other) = default;

        virtual ~Node() = default;

        inline virtual std::string resolved_name() const {
            return get_str();
        }

        inline Slot_type add_child(const std::shared_ptr<Node> child){
            children.push_back(child);
            return &children.back();
        }

        inline void transition_to_done(){
            state = NB_DONE;
        }

        inline Node_build_state build_state() const {
            return state;
        }

        inline int get_id() const {
            return id;
        }

        inline void incr_id(){
            id++;
        }

        inline std::string get_str() const {
            return (kind == SYNTAX) ? escape_string(str) : str;
        }

        inline Token_kind get_node_kind() const {
            return kind;
        }


        inline std::vector<std::shared_ptr<Node>>& get_children() {
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

        inline void erase_child(size_t index) {
            if(index < size()){
                children.erase(children.begin() + index);
            }
        }

        inline size_t size() const {
            return children.size();
        }

        inline bool branch_satisfies_constraints(const Branch& branch){
            if (branch_constraint.has_value()){
                return branch_constraint.value().passed(branch);
            } else {
                return true;
            }
        }

        inline void add_branch_constraint(const Branch_constraint& _branch_constraint) {
            branch_constraint = std::move(_branch_constraint);
        }

        inline void add_branch_constraint(const Token_kind& singleton_term_token_kind, unsigned int n_occurances){
            branch_constraint = std::make_optional<Branch_constraint>(singleton_term_token_kind, n_occurances);
        }

        inline bool has_branch_constraint(){
            return branch_constraint.has_value();
        }

        inline void print_branch_constraint(std::ostream& stream){
            if(branch_constraint.has_value()){
                stream << branch_constraint.value() << std::endl;
            } else {
                stream << GREEN("NO CONSTRAINTS") << std::endl;
            }
        }

        inline void remove_branch_constraint(){
            branch_constraint = std::nullopt;
        }

        virtual unsigned int get_n_ports() const;

        virtual std::shared_ptr<Variable> get_name() const;

        virtual std::shared_ptr<UInt> get_size() const;

        virtual std::shared_ptr<UInt> get_index() const;

        virtual std::shared_ptr<Node> clone(const Clone_type& ct) const;

        virtual void print_program(std::ostream& stream, unsigned int indent_level = 0) const;

        bool visited(std::vector<Slot_type>& visited_slots, Slot_type slot, bool track_visited) const ;

        Slot_type find_slot(Token_kind node_kind, std::vector<Slot_type>& visited_slots, bool track_visited = true);

        std::shared_ptr<Node> find(Token_kind node_kind);

        void print_ast(std::string indent) const;

        void extend_dot_string(std::ostringstream& ss) const;

        int get_next_child_target();

        void make_partition(int target, int n_children);

        void make_control_flow_partition(int target, int n_children);

        Slot_type get_compilation_unit();

        bool operator==(const Token_kind& other_kind) const {
            return kind == other_kind;
        }

        bool operator==(const Node& other) const {
            return get_str() == other.get_str();
        }

    protected:
        int id = 0;
        std::string str;
        Token_kind kind;

        std::vector<std::shared_ptr<Node>> children;
        Node_build_state state = NB_BUILD;

        std::vector<int> child_partition;
        unsigned int partition_counter = 0;

    private:
        std::optional<Branch_constraint> branch_constraint;
};

#endif
