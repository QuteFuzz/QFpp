#ifndef NESTED_BRANCH_H
#define NESTED_BRANCH_H

/*
    elif and else make up this node type
    these need to keep the indentation tracker in order to indent correctly
*/

#include <node.h>

class Nested_branch : public Node {

    public:

        Nested_branch(const std::string& str, const Token_kind& kind, unsigned int target_num_qubit_ops):
            Node(str, kind, indentation_tracker)
        {

            if(kind == ELIF_STMT){
                /*
                    control flow branch with just compound stmts, or control flow branch with compound stmts and control flow branch
                */
                unsigned int n_children = (target_num_qubit_ops == 1) || random_int(1) ? 1 : 2;
                
                make_control_flow_partition(target_num_qubit_ops, n_children);
            
            } else if (kind == ELSE_STMT){
                // only one child we're interested in, which is compound stmts
                make_control_flow_partition(target_num_qubit_ops, 1);
            }
        }

        Nested_branch(const std::string& str, const Token_kind& kind):
            Node(str, kind, indentation_tracker)
        {}

        void print(std::ostream& stream) const override {
            stream << indentation_str;

            for(const std::shared_ptr<Node>& child : children){
                stream << *child;
            } 
        }

    private:

};

#endif
