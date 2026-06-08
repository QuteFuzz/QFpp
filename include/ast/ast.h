#ifndef AST_H
#define AST_H

#include <optional>
#include <algorithm>
#include "qf_term.h"
#include "grammar.h"
#include "node.h"
#include "context.h"
#include "supported_gates.h"
#include "ast_utils.h"

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

        Slot_type term_branch_to_child_nodes(
            std::shared_ptr<Node> parent, 
            const Term& term,
            std::unordered_map<Token_kind, Branch_constraint>& descendant_node_branch_constraints,
    	    std::shared_ptr<Expr> term_expr_override = nullptr,
            unsigned int recurr_depth = 0
        );

        std::variant<std::shared_ptr<Node>, Term> make_child(const std::shared_ptr<Node> parent, const Term& term);

        std::shared_ptr<Context> get_context() const { return std::make_shared<Context>(context); }

        std::shared_ptr<Node> build(std::shared_ptr<Rule> entry, std::unordered_map<Token_kind, Branch_constraint> descendant_node_branch_constraints = {});

    protected:
        std::shared_ptr<Node> root;
        Context context;
        Control control;
};

struct Ast_entry {

    public:
        Ast_entry(){}

        Ast_entry(std::shared_ptr<Node> _ast, std::shared_ptr<Context> _context) :
            ast(_ast),
            context(_context)
        {
            if (ast == nullptr) {
                ERROR("Cannot pass NULL as AST to entry");
            }

            comp_unit = get_compilation_unit(ast);
            find_qubit_ops();
        }

        void find_qubit_ops(){
            auto _find_qubit_ops = [](Node& root, std::vector<std::shared_ptr<Qubit_op>>& out){
                for (const auto& node : Node_gen(root, QUBIT_OP)){                
                    auto qubit_op = static_pointer_cast<Qubit_op>(node);
                    out.push_back(qubit_op);
                }
            };

            _find_qubit_ops(*ast, ast_qubit_ops);
            _find_qubit_ops(*comp_unit, comp_unit_qubit_ops);
        }

        /// return a clone of this ast entry, by deep cloning the AST, effectively creating a new one, then getting the new compilation unit ptr from that
        Ast_entry clone() {
            std::shared_ptr<Node> new_ast = ast->clone(DEEP);
            return Ast_entry{new_ast, context};
        }

        bool empty() const {return (ast == nullptr);}

        // Get entire AST
        std::shared_ptr<Node> get_ast() const {
            return ast;
        }

        // Get compilation unit root, or AST root
        std::shared_ptr<Node> get_root(bool consider_entire_ast) const {
            return consider_entire_ast ? ast : comp_unit;
        }

        // Qubit ops of entire AST or compilation unit
        std::vector<std::shared_ptr<Qubit_op>> get_qubit_ops(bool consider_entire_ast) const {
            return consider_entire_ast ? ast_qubit_ops : comp_unit_qubit_ops;
        }

        std::shared_ptr<Context> get_context() const {
            return context;
        }

    private:
        std::shared_ptr<Node> ast = nullptr;
        std::shared_ptr<Node> comp_unit;
        std::shared_ptr<Context> context;

        std::vector<std::shared_ptr<Qubit_op>> comp_unit_qubit_ops;
        std::vector<std::shared_ptr<Qubit_op>> ast_qubit_ops;

};

#endif
