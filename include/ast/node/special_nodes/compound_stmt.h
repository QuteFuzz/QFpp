#ifndef COMPOUND_STMT_H
#define COMPOUND_STMT_H

#include <node.h>

class Gate;
class Circuit;

class Compound_stmt : public Node {

    public:
        Compound_stmt(unsigned int nested_depth):
            Node("compound_stmt", COMPOUND_STMT)
        {
            if(nested_depth == 0){
                add_branch_constraint(QUBIT_OP, 1);
            }
        }
};

#endif
