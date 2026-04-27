#ifndef AST_H
#define AST_H

#include <optional>
#include <algorithm>
#include <qf_term.h>
#include <grammar.h>
#include <node.h>
#include <context.h>
#include <supported_gates.h>

class Ast{
    public:
        Ast(const Control& _control) :
            context(_control),
            control(_control)
        {
            context.reset(RL_PROGRAM);
        }

        /// this build setup resets context resource usages and depths are as if new
        Ast(const Context& _context, unsigned int manual_nested_depth) :
            context(_context),
            control(_context.get_control())
        {
            context.change_nested_depth(manual_nested_depth);
            context.reset(RL_QUBITS);
            context.reset(RL_BITS);
        }

        ~Ast() = default;

        std::vector<Term> resolve_term_expr(const Term& init_child_term, std::optional<unsigned int> term_constraint_max);

        Slot_type term_branch_to_child_nodes(
            std::shared_ptr<Node> parent, 
            const Term& term,
            std::optional<unsigned int> term_constraint_max,
            std::unordered_map<Token_kind, Branch_constraint>& descendant_node_branch_constraints,
            unsigned int depth = 0
        );

        std::variant<std::shared_ptr<Node>, Term> make_child(const std::shared_ptr<Node> parent, const Term& term);

        std::shared_ptr<Context> get_context() const { return std::make_shared<Context>(context); }

        Term make_term_from_rule(std::shared_ptr<Rule> rule_ptr);

        std::shared_ptr<Node> build(std::shared_ptr<Rule> entry, std::unordered_map<Token_kind, Branch_constraint> descendant_node_branch_constraints = {});

    protected:
        std::shared_ptr<Node> root;
        Context context;
        Control control;
};

struct Ast_entry {
    std::shared_ptr<Node> ast;
    std::shared_ptr<Context> context;

    bool empty() const {return (ast == nullptr);}

    /// return a clone of this ast entry, by deep cloning the AST, effectively creating a new one, then getting the new compilation unit ptr from that
    Ast_entry clone() {
        std::shared_ptr<Node> new_ast = ast->clone(DEEP);
        return Ast_entry{new_ast, context};
    }

};

#endif
