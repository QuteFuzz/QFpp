#ifndef COMPOUND_STMTS_H
#define COMPOUND_STMTS_H

#include <node.h>

class Compound_stmts : public Node {

    public:

        static std::shared_ptr<Compound_stmts> from_num_compound_stmts(unsigned int num_statements){
            Compound_stmts stmts;
            stmts.add_constraint(COMPOUND_STMT, num_statements);

            return std::make_shared<Compound_stmts>(stmts);
        }

        static std::shared_ptr<Compound_stmts> from_num_qubit_ops(unsigned int target_num_qubit_ops){
            Compound_stmts stmts;

            unsigned int n_children = (target_num_qubit_ops >= WILDCARD_MAX) ? WILDCARD_MAX : target_num_qubit_ops;

            stmts.add_constraint(COMPOUND_STMT, n_children);
            stmts.make_partition(target_num_qubit_ops, n_children);
            
            return std::make_shared<Compound_stmts>(stmts);
        }

    private:
        Compound_stmts():
            Node("compound_stmts", COMPOUND_STMTS)
        {}

};

#endif
