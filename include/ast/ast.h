#ifndef AST_H
#define AST_H

#include <optional>
#include <algorithm>
#include <term.h>
#include <grammar.h>
#include <node.h>
#include <context.h>
#include <dag.h>

struct Genome;

class Ast{
    public:
        Ast(){}

        ~Ast() = default;

        inline void set_entry(const std::shared_ptr<Rule> _entry){
            entry = _entry;
        }

        inline bool entry_set(){return entry != nullptr;}

        void write_branch(std::shared_ptr<Node> parent, const Term& term, const Control& control, unsigned int depth = 0);

        std::shared_ptr<Node> get_node(const std::shared_ptr<Node> parent, const Term& term);

        Result<Node> build(const std::optional<Genome>& genome, const std::optional<Node_constraint>& swarm_testing_gateset, const Control& control);

        Genome genome();

        inline void print_ast(){
            root->print_ast("");
        }

        inline void render_dag(const fs::path& current_circuit_dir){
            render([dag = dag](std::ostringstream& dot_string){dag->extend_dot_string(dot_string);},
                current_circuit_dir / "dag.png");
        }

        inline void render_ast(const fs::path& current_circuit_dir){
            render([root = root](std::ostringstream& dot_string){root->extend_dot_string(dot_string);},
                current_circuit_dir / "ast.png");
        }

    protected:
        std::shared_ptr<Rule> entry = nullptr;
        std::shared_ptr<Node> dummy = std::make_shared<Node>("");
        std::shared_ptr<Node> root;
        std::shared_ptr<Dag> dag;
        std::optional<Node_constraint> swarm_testing_gateset = std::nullopt;
        Context context;
};

#endif
