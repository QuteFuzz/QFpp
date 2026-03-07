#ifndef GENERATOR_H
#define GENERATOR_H

#include <grammar.h>
#include <ast.h>
#include <qf_term.h>
#include <mutate.h>
#include <info.h>

struct Generator {

    public:

        Generator(Grammar& _grammar):
            grammar(std::make_shared<Grammar>(_grammar))
        {}

        inline void set_grammar_entry(std::string& _entry_name, Scope& _entry_scope){
            entry_name = _entry_name;
            entry_scope = _entry_scope;
        }

        friend std::ostream& operator<<(std::ostream& stream, Generator generator){
            stream << "  . " << generator.grammar->get_name() << ": ";
            generator.grammar->print_rules();

            return stream;
        }

        inline void print_grammar(){ std::cout << *grammar; }

        inline void print_tokens(){ grammar->print_tokens(); }

        inline std::shared_ptr<Grammar> get_grammar() const { return grammar; }

        void ast_parse(const std::vector<Ast_entry>& entries, const fs::path& output_dir, const Control& control);

        std::vector<Ast_entry> generate_n_asts(unsigned int n, const Control& control);

        std::vector<Ast_entry> map_elites(unsigned int n_genomes, const Control& control, const fs::path& output_dir);

        inline void print_ast(const Node& root){
            root.print_ast("");
        }

        inline void render_ast(const Node& root, const fs::path& current_circuit_dir){
            render([root = root](std::ostringstream& dot_string){root.extend_dot_string(dot_string);},
                current_circuit_dir / "ast.png");
        }

    private:
        std::shared_ptr<Grammar> grammar;
        std::string entry_name;
        Scope entry_scope;
};

#endif
