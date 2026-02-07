#ifndef AST_H
#define AST_H

#include <optional>
#include <algorithm>
#include <qf_term.h>
#include <grammar.h>
#include <node.h>
#include <context.h>
#include <dag.h>
#include <supported_gates.h>

// struct Genome;

class Ast{
    public:
        Ast(const Control& _control) :
            context(_control),
            control(_control)
        {}

        ~Ast() = default;

        inline void set_entry(const std::shared_ptr<Rule> _entry){
            entry = _entry;
        }

        inline bool entry_set(){return entry != nullptr;}

        void term_branch_to_child_nodes(std::shared_ptr<Node> parent, const Term& term, unsigned int depth = 0);

        std::variant<std::shared_ptr<Node>, Term> make_child(const std::shared_ptr<Node> parent, const Term& term);

        Result<Node> build();

        // Genome genome();

        inline void print_ast(){
            root->print_ast("");
        }

        // inline void render_dag(const fs::path& current_circuit_dir){
        //     render([dag = dag](std::ostringstream& dot_string){dag->extend_dot_string(dot_string);},
        //         current_circuit_dir / "dag.png");
        // }

        inline void render_ast(const fs::path& current_circuit_dir){
            render([root = root](std::ostringstream& dot_string){root->extend_dot_string(dot_string);},
                current_circuit_dir / "ast.png");
        }

    protected:
        std::shared_ptr<Rule> entry = nullptr;
        std::shared_ptr<Node> dummy = std::make_shared<Node>("");
        std::shared_ptr<Node> root;
        // std::shared_ptr<Dag> dag;
        std::vector<Gate_info> gateset;
        Context context;
        const Control& control;
};

#endif
