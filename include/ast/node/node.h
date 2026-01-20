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


struct Node_constraint {

    public:

        Node_constraint(){}

        Node_constraint(Token_kind rule, unsigned int _occurances):
            rule_kinds_and_occurances{{rule, _occurances}}
        {}

        Node_constraint(std::unordered_map<Token_kind, unsigned int> _rule_kinds_and_occurances):
            rule_kinds_and_occurances(std::move(_rule_kinds_and_occurances))
        {}

        bool passed(const Branch& branch){
            // Count the number of occurances of each rule in the branch and return true if they match the expected occurances
            for(const auto& [rule, occurances] : rule_kinds_and_occurances){
                if(branch.count_rule_occurances(rule) != occurances){
                    return false;
                }
            }
            return true;
        }

        void set_occurances_for_rule(const Token_kind& rule, unsigned int n_occurances){
            rule_kinds_and_occurances[rule] = n_occurances;
        }

        unsigned int size() const {
            return rule_kinds_and_occurances.size();
        }

        void add(const Token_kind& rule, unsigned int n_occurances){
            //Check if rule is in rule_kinds_and_occurances
            if(rule_kinds_and_occurances.find(rule) != rule_kinds_and_occurances.end()){
                rule_kinds_and_occurances[rule] += n_occurances;
            } else {
                rule_kinds_and_occurances[rule] = n_occurances;
            }
        }

        const std::unordered_map<Token_kind, unsigned int>& get_constraints() const {
            return rule_kinds_and_occurances;
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

        Node(std::string _content, Token_kind _kind, const std::optional<Node_constraint>& _constraint, const std::string _indentation_str = ""):
            content(_content),
            kind(_kind),
            indentation_str(_indentation_str),
            constraint(_constraint)
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

        std::string get_content() const {
            std::string esc_content = (kind == SYNTAX) ? escape_string(content) : content;
            std::string str_id = " " + std::to_string(id);

            if(content.size() > 10){
                return esc_content.substr(0, 10) + " ... " + str_id;
            } else {
                return esc_content + str_id;
            }
        }

        Token_kind get_kind() const {
            return kind;
        }

        virtual std::string resolved_name() const {
            return get_content();
        }

        inline bool visited(std::vector<std::shared_ptr<Node>*>& visited_slots, std::shared_ptr<Node>* slot, bool track_visited){
            for(auto vslot : visited_slots){
                if(vslot == slot) return true;
            }

            if(track_visited) visited_slots.push_back(slot);
            return false;
        }

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

        bool branch_satisfies_constraint(const Branch& branch){
            return (!constraint.has_value() || constraint.value().passed(branch));
        }

        void set_constraint(std::vector<Token_kind> rule_kinds, std::vector<unsigned int> occurances){
            if(rule_kinds.size() != occurances.size()){
                ERROR("Hashes vector must be the same size as occurances vector!");
            }

            // Convert to unordered map for constructor of Node_constraint
            std::unordered_map<Token_kind, unsigned int> gateset_map;
            for(size_t i = 0; i < rule_kinds.size(); ++i){
                gateset_map[rule_kinds[i]] = occurances[i];
            }

            constraint = std::make_optional<Node_constraint>(gateset_map);
        }

        void add_constraint(const Token_kind& rule_kind, unsigned int n_occurances){
            if(constraint.has_value()){
                constraint.value().add(rule_kind, n_occurances);
            } else {
                constraint = std::make_optional<Node_constraint>(rule_kind, n_occurances);
            }
        }

        virtual unsigned int get_n_ports() const {return 1;}

        int get_next_child_target();

        void make_partition(int target, int n_children);

        void make_control_flow_partition(int target, int n_children);

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
        std::optional<Node_constraint> constraint;
};

#endif
