#ifndef COMPOUND_STMT_H
#define COMPOUND_STMT_H

#include <node.h>

class Gate;
class Circuit;

class Compound_stmt : public Node {

    public:
        static std::shared_ptr<Compound_stmt> from_nested_depth(unsigned int nested_depth){
            Compound_stmt stmt;

            if(nested_depth == 0){
                stmt.add_constraint(QUBIT_OP, 1);
            }

            return std::make_shared<Compound_stmt>(stmt);
        }

        static std::shared_ptr<Compound_stmt> from_num_qubit_ops(unsigned int target_num_qubit_ops){
            Compound_stmt stmt;

            if(target_num_qubit_ops == 1){
                /*
                    make qubit op
                */
                stmt.add_constraint(QUBIT_OP, 1);

            } else {
                /*
                    use nesting circuit, target is > 1
                */
                stmt.make_partition(target_num_qubit_ops, 1);
                stmt.add_constraint(QUBIT_OP, 0);
            }

            return std::make_shared<Compound_stmt>(stmt);
        }

    private:
        Compound_stmt():
            Node("compound_stmt", COMPOUND_STMT, indentation_tracker)
        {}
};

#endif
