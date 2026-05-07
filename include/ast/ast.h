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

        Slot_type term_branch_to_child_nodes(
            std::shared_ptr<Node> parent, 
            const Term& term,
            std::optional<unsigned int> term_constraint_max,
            std::unordered_map<Token_kind, Branch_constraint>& descendant_node_branch_constraints,
            unsigned int depth = 0
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

        Ast_entry(std::shared_ptr<Node> _ast, std::shared_ptr<Context> _context, bool _consider_entire_ast = false) :
            ast(_ast),
            context(_context),
            consider_entire_ast(_consider_entire_ast)
        {
            assert(ast != nullptr);

            for (const auto& node : Node_gen(*get_root(), QUBIT_OP)){
                auto qubit_op = std::dynamic_pointer_cast<Qubit_op>(node);

                if (qubit_op == nullptr){
                    node->print_program(std::cout);
                    std::cout << std::endl;
                    
                    node->print_ast("");
                    std::cout << std::endl;

                    ERROR("Nodes of kind `QUBIT_OP` must be of `Qubit_op` type");
                }
            
                qubit_ops.push_back(qubit_op);
            }
        }

        /// return a clone of this ast entry, by deep cloning the AST, effectively creating a new one, then getting the new compilation unit ptr from that
        Ast_entry clone() {
            std::shared_ptr<Node> new_ast = ast->clone(DEEP);
            return Ast_entry{new_ast, context};
        }

        bool empty() const {return (ast == nullptr);}

        std::shared_ptr<Node> get_ast() const {
            return ast;
        }

        std::shared_ptr<Node> get_root() const {
            assert(ast != nullptr);

            return consider_entire_ast ? ast : *ast->get_compilation_unit();
        }

        std::vector<std::shared_ptr<Qubit_op>> get_qubit_ops() const {
            return qubit_ops;
        }

        std::shared_ptr<Context> get_context() const {
            return context;
        }

    private:
        std::shared_ptr<Node> ast;
        std::shared_ptr<Context> context;
        bool consider_entire_ast = false; // or just compilation unit
        std::vector<std::shared_ptr<Qubit_op>> qubit_ops;

};

#endif
