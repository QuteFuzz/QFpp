#ifndef NODE_H
#define NODE_H

#include <utils.h>
#include <branch.h>

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
        Node_constraint(Token_kind rule, unsigned int _occurances):
            rule_kinds({rule}),
            occurances({_occurances})
        {}

        Node_constraint(std::vector<Token_kind> _rule_kinds, std::vector<unsigned int> _occurances): 
            rule_kinds(std::move(_rule_kinds)),
            occurances(std::move(_occurances))
        {}

        bool passed(const Branch& branch){
            // Count the number of occurances of each rule in the branch and return true if they match the expected occurances
            for(size_t i = 0; i < rule_kinds.size(); i++){
                if(branch.count_rule_occurances(rule_kinds[i]) != occurances[i]){
                    return false;
                }
            }
            return true;
        }

        Token_kind get_rule_kind_at(unsigned int index) const {
            return rule_kinds[index];
        }

        unsigned int get_occurances_at(unsigned int index) const {
            return occurances[index];
        }

        unsigned int size() const {
            return rule_kinds.size();
        }

        void add(const Token_kind& rule, unsigned int n_occurances){
            rule_kinds.push_back(rule);
            occurances.push_back(n_occurances);    
        }

    private:
        std::vector<Token_kind> rule_kinds;
        std::vector<unsigned int> occurances = {0};

};


/// @brief A node is a term with pointers to other nodes
class Node {

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

        void transition_to_done(){
            state = NB_DONE;
        }

        Node_build_state build_state(){
            return state;
        }

        /// @brief Get node content, stored as a string
        /// @return 
        std::string get_content() const {
            return content;
        }

        int get_id() const {
            return id;
        }

        virtual std::string resolved_name() const {
            return content + ", id: " + std::to_string(id);
        }

        // Node_kind get_node_kind() const {return kind;}

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

        void print_ast(){
            std::cout << content << std::endl;

            for(const std::shared_ptr<Node>& child : children){
                std::cout << "\t";
                child->print_ast();
            }

        }

        std::vector<std::shared_ptr<Node>> get_children() const {
            return children;
        }

        inline std::shared_ptr<Node> child_at(size_t index) const {
            if(index < children.size()){
                return children.at(index);
            } else {
                return nullptr;
            }
        }

        size_t get_num_children() const {
            return children.size();
        }

        bool operator==(const Token_kind& other_kind){
            return kind == other_kind;
        }

        // bool operator==(const std::string& other){
        //     return string == other;
        // }

        bool branch_satisfies_constraint(const Branch& branch){
            return !constraint.has_value() || constraint.value().passed(branch);
        }

        void set_constraint(std::vector<Token_kind> rule_kinds, std::vector<unsigned int> occurances){
            if(rule_kinds.size() != occurances.size()){
                ERROR("Hashes vector must be the same size as occurances vector!");
            }

            constraint = std::make_optional<Node_constraint>(rule_kinds, occurances);
        }

        void add_constraint(const Token_kind& rule_kind, unsigned int n_occurances){
            if(constraint.has_value()){
                constraint.value().add(rule_kind, n_occurances);
            } else {
                constraint = std::make_optional<Node_constraint>(rule_kind, n_occurances);
            }
        }

        #ifdef DEBUG
        std::string get_debug_constraint_string() const;
        #endif

        virtual unsigned int get_n_ports() const {return 1;}

        // std::shared_ptr<Node> find(const U64 _hash) const;

        int get_next_child_target();

        void make_partition(int target, int n_children);

        void make_control_flow_partition(int target, int n_children);

    protected:
        std::string content;
        Token_kind kind;

        int id;

        std::string indentation_str;
        std::vector<std::shared_ptr<Node>> children;
        Node_build_state state = NB_BUILD;

        std::vector<int> child_partition;
        unsigned int partition_counter = 0;
    
    private:
        std::optional<Node_constraint> constraint;
};

#endif
