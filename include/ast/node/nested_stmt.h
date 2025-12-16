#ifndef NESTED_STMT_H
#define NESTED_STMT_H

#include <node.h>

class Nested_stmt : public Node {

    public:
        Nested_stmt(const std::string& str, const Token_kind& kind):
            Node(str, kind)
        {}
    
        Nested_stmt(const std::string& str, const Token_kind& kind, unsigned int target_num_qubit_ops):
            Node(str, kind)
        {
            /*
                control flow with just compound stmts, or control flow with compound stmts and control flow branch
            */
            unsigned int n_children = (target_num_qubit_ops == 1) || random_int(1) ? 1 : 2;

            make_control_flow_partition(target_num_qubit_ops, n_children);
        }

    private:


};


#endif
