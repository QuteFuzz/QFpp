#ifndef NODE_H
#define NODE_H

#include <utils.h>
#include <lex.h>
#include <lex.h>
#include <branch.h>
#include <lex.h>

enum Node_build_state {
    NB_DONE,
    NB_BUILD,
};


enum Node_kind {
    TERMINAL,
    NON_TERMINAL,
};


struct Node_constraints {

    public:

        Node_constraints(){}

        Node_constraints(Token_kind rule, unsigned int _occurances):
            rule_kinds_and_occurances{{rule, _occurances}}
        {}

        Node_constraints(std::unordered_map<Token_kind, unsigned int> _rule_kinds_and_occurances):
            rule_kinds_and_occurances(std::move(_rule_kinds_and_occurances))
        {}

        bool passed(const Branch& branch);

        void set_occurances_for_rule(const Token_kind& rule, unsigned int n_occurances);

        inline unsigned int n_constraints() const {
            return rule_kinds_and_occurances.size();
        }

        void add(const Token_kind& rule, unsigned int n_occurances);

        inline const std::unordered_map<Token_kind, unsigned int>& get_constraints() const {
            return rule_kinds_and_occurances;
        }

        friend std::ostream& operator<<(std::ostream& stream, Node_constraints& nc) {
            stream << "====================================" << std::endl;

            for(const auto& [rule, occurances] : nc.rule_kinds_and_occurances){
                stream << kind_as_str(rule) << " " << occurances << std::endl;
            }

            stream << "====================================" << std::endl;

            return stream;
        }

    private:
        std::unordered_map<Token_kind, unsigned int> rule_kinds_and_occurances;

};

/// @brief A node is a term with pointers to other nodes
class Node : public std::enable_shared_from_this<Node> {

    public:
        static std::string indentation_tracker;
        static int node_counter;

        Node(){}

        Node(std::string _content, Token_kind _kind = SYNTAX, const std::string _indentation_str = ""):
            content(_content),
            kind(_kind),
            indentation_str(_indentation_str)
        {
            id = node_counter++;
        }

        Node(std::string _content, Token_kind _kind, const std::optional<Node_constraints>& _constraints, const std::string _indentation_str = ""):
            content(_content),
            kind(_kind),
            indentation_str(_indentation_str),
            constraints(_constraints)
        {
            id = node_counter++;
        }

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

        std::string get_content() const;

        Token_kind get_kind() const {
            return kind;
        }

        virtual std::string resolved_name() const {
            return get_content();
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

        virtual void print(std::ostream& stream) const {
            if(kind == SYNTAX){
                stream << content;
            } else {

                for(const std::shared_ptr<Node>& child : children){
                    stream << indentation_str << *child;
                }
            }
        }

        friend std::ostream& operator<<(std::ostream& stream, const Node& n) {
            n.print(stream);
            return stream;
        }

        void print_ast(std::string indent) const;

        void extend_dot_string(std::ostringstream& ss) const;

        std::vector<std::shared_ptr<Node>> get_children() const {
            return children;
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
            return get_content() == other.get_content();
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

        virtual unsigned int get_n_ports() const {return 1;}

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

    protected:
        int id = 0;
        std::string content;
        Token_kind kind;

        std::string indentation_str;
        std::vector<std::shared_ptr<Node>> children;
        Node_build_state state = NB_BUILD;

        std::vector<int> child_partition;
        unsigned int partition_counter = 0;

    private:
        std::optional<Node_constraints> constraints;
};

#endif
